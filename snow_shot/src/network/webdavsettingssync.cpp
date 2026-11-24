// SPDX-License-Identifier: GPL-3.0-or-later
#include "snow_shot/network/webdavsettingssync.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTimer>

#include <cmath>
#include <memory>
#include <utility>

namespace snow_shot::network::webdav {
namespace {
using namespace Qt::StringLiterals;

bool integerIn(const QJsonValue& value, int minimum, int maximum) {
    const double number = value.toDouble(-1);
    return value.isDouble() && std::isfinite(number) && std::floor(number) == number &&
           number >= minimum && number <= maximum;
}

bool allowed(const QString& key, const QJsonValue& value) {
    static const QStringList booleans{
        u"floating_toolbar/enabled"_s, u"floating_toolbar/hide_in_fullscreen"_s,
        u"floating_toolbar/hide_during_capture"_s, u"screenshot/capture_cursor"_s,
        u"pin_to_screen/show_border_by_default"_s, u"pin_to_screen/show_shadow_by_default"_s,
        u"drawing/remember_last_used_tool"_s};
    if (booleans.contains(key))
        return value.isBool();
    if (key == u"interface/theme_mode"_s)
        return QStringList{u"system"_s, u"light"_s, u"dark"_s}.contains(value.toString());
    if (key == u"floating_toolbar/mode"_s)
        return QStringList{u"icon"_s, u"toolbar"_s}.contains(value.toString());
    if (key == u"interface/theme_primary_color"_s) {
        static const QRegularExpression color(u"^#[0-9a-fA-F]{8}$"_s);
        return value.isString() && color.match(value.toString()).hasMatch();
    }
    if (key == u"interface/app_font_size"_s)
        return integerIn(value, 12, 32);
    if (key == u"floating_toolbar/opacity"_s)
        return integerIn(value, 10, 100);
    if (key == u"screenshot/delay_seconds"_s)
        return integerIn(value, 1, 10);
    return false;
}

bool validSettings(const QJsonObject& values) {
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (!allowed(it.key(), it.value()))
            return false;
    }
    return true;
}

bool validEndpoint(const QUrl& url) {
    return url.isValid() && url.scheme() == u"https"_s && !url.host().isEmpty() &&
           !url.authority(QUrl::FullyEncoded).contains(u'@') && !url.hasQuery() &&
           !url.hasFragment() && !url.path().isEmpty() && !url.path().endsWith(u'/');
}

Result resultFor(QNetworkReply& reply, const QByteArray& document, bool write, bool overflow) {
    if (overflow)
        return {Status::TooLarge, {}, {}};
    const int code = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code == 401 || code == 403)
        return {Status::Unauthorized, {}, {}};
    if (code == 409 || code == 412)
        return {Status::Conflict, {}, {}};
    if (!write && code == 404)
        return {Status::Missing, {}, {}};
    if (reply.error() != QNetworkReply::NoError)
        return {Status::NetworkError, {}, {}};
    const QByteArray etag = reply.rawHeader("ETag");
    if (write && (code == 200 || code == 201 || code == 204))
        return {Status::Ok, {}, strongEtag(etag) ? etag : QByteArray{}};
    if (!write && code == 200 && strongEtag(etag) && decodeSettings(document))
        return {Status::Ok, document, etag};
    // Redirects, malformed documents and weak/missing read validators fail closed.
    return {Status::Invalid, {}, {}};
}
} // namespace

QJsonObject projectSettings(const QJsonObject& local) {
    QJsonObject result;
    for (auto it = local.constBegin(); it != local.constEnd(); ++it) {
        if (allowed(it.key(), it.value()))
            result.insert(it.key(), it.value());
    }
    return result;
}

std::optional<QByteArray> encodeSettings(const QJsonObject& values) {
    if (!validSettings(values))
        return std::nullopt;
    const QJsonObject root{{u"format"_s, 1}, {u"settings"_s, values}};
    const auto document = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return document.size() <= maxDocumentBytes ? std::optional(document) : std::nullopt;
}

std::optional<QJsonObject> decodeSettings(const QByteArray& document) {
    if (document.isEmpty() || document.size() > maxDocumentBytes)
        return std::nullopt;
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(document, &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject())
        return std::nullopt;
    const auto root = parsed.object();
    if (root.size() != 2 || root.value(u"format"_s) != QJsonValue(1) ||
        !root.value(u"settings"_s).isObject())
        return std::nullopt;
    const auto values = root.value(u"settings"_s).toObject();
    return validSettings(values) ? std::optional(values) : std::nullopt;
}

MergePlan mergeSettings(const QJsonObject& baseline, const QJsonObject& local,
                        const QJsonObject& remote) {
    if (!validSettings(baseline) || !validSettings(local) || !validSettings(remote))
        return {};
    QStringList keys = baseline.keys() + local.keys() + remote.keys();
    keys.removeDuplicates();
    keys.sort();
    MergePlan plan;
    plan.valid = true;
    for (const auto& key : keys) {
        const auto before = baseline.value(key);
        // Absence never clears a preference.
        const auto left = local.contains(key) ? local.value(key) : before;
        const auto right = remote.contains(key) ? remote.value(key) : before;
        QJsonValue selected(QJsonValue::Undefined);
        if (left == right || right.isUndefined() || right == before)
            selected = left;
        else if (left.isUndefined() || left == before)
            selected = right;
        else {
            plan.conflicts.append(key);
            continue;
        }
        if (!selected.isUndefined())
            plan.values.insert(key, selected);
    }
    return plan;
}

bool strongEtag(const QByteArray& etag) {
    static const QRegularExpression syntax(u"^\"[!#-~]*\"$"_s);
    return etag.size() <= 512 && syntax.match(QString::fromLatin1(etag)).hasMatch();
}

std::optional<QNetworkRequest> requestFor(const QUrl& endpoint, const QString& user,
                                         const QString& password) {
    if (!validEndpoint(endpoint) || user.isEmpty() || password.isEmpty() ||
        user.contains(u':') || user.contains(u'\r') || user.contains(u'\n') ||
        password.contains(u'\r') || password.contains(u'\n'))
        return std::nullopt;
    QNetworkRequest request(endpoint);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setTransferTimeout(15000);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", "Basic " + (user + u':' + password).toUtf8().toBase64());
    return request;
}

std::optional<QNetworkRequest> writeRequest(const QUrl& endpoint, const QString& user,
                                           const QString& password, const QByteArray& etag,
                                           bool createOnly) {
    if ((createOnly && !etag.isEmpty()) || (!createOnly && !strongEtag(etag)))
        return std::nullopt;
    auto request = requestFor(endpoint, user, password);
    if (!request)
        return std::nullopt;
    request->setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_s);
    if (createOnly)
        request->setRawHeader("If-None-Match", "*");
    else
        request->setRawHeader("If-Match", etag);
    return request;
}

Client::Client(QObject* parent) : QObject(parent), m_network(this) {}

void Client::read(const QUrl& endpoint, const QString& user, const QString& password,
                  Completion complete) {
    const auto request = requestFor(endpoint, user, password);
    if (!request) {
        complete({});
        return;
    }
    observe(m_network.get(*request), false, std::move(complete));
}

void Client::write(const QUrl& endpoint, const QString& user, const QString& password,
                   const QByteArray& document, const QByteArray& etag, bool createOnly,
                   Completion complete) {
    const auto request = writeRequest(endpoint, user, password, etag, createOnly);
    if (!request || !decodeSettings(document)) {
        complete({});
        return;
    }
    observe(m_network.put(*request, document), true, std::move(complete));
}

void Client::observe(QNetworkReply* reply, bool write, Completion complete) {
    struct Buffer {
        QByteArray document;
        bool overflow = false;
    };
    auto buffer = std::make_shared<Buffer>();
    reply->setReadBufferSize(maxDocumentBytes + 1);
    const auto drain = [reply, buffer] {
        if (buffer->overflow)
            return;
        buffer->document += reply->read(maxDocumentBytes + 1 - buffer->document.size());
        if (buffer->document.size() > maxDocumentBytes) {
            buffer->overflow = true;
            reply->abort();
        }
    };
    QObject::connect(reply, &QNetworkReply::readyRead, this, drain);
    QTimer::singleShot(30000, reply, [reply] {
        if (!reply->isFinished())
            reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [reply, buffer, drain, write, complete = std::move(complete)] {
                         drain();
                         auto result = resultFor(*reply, buffer->document, write, buffer->overflow);
                         reply->deleteLater();
                         complete(std::move(result));
                     });
}

} // namespace snow_shot::network::webdav
