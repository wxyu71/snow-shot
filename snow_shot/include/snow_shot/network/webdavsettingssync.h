// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QStringList>
#include <QUrl>

#include <functional>
#include <optional>

namespace snow_shot::network::webdav {

// Deliberately narrow initial scope: scalar, non-sensitive preferences only.
// Missing keys are not deletions; explicit resets are represented by defaults.
inline constexpr qsizetype maxDocumentBytes = 1024 * 1024;
QJsonObject projectSettings(const QJsonObject& local);
std::optional<QByteArray> encodeSettings(const QJsonObject& values);
std::optional<QJsonObject> decodeSettings(const QByteArray& document);

struct MergePlan {
    QJsonObject values;
    QStringList conflicts;
    bool valid = false;
};

// No side effects. Do not persist or upload a plan with conflicts.
MergePlan mergeSettings(const QJsonObject& baseline, const QJsonObject& local,
                        const QJsonObject& remote);

enum class Status { Ok, Missing, Conflict, Unauthorized, Invalid, NetworkError, TooLarge };
struct Result {
    Status status = Status::Invalid;
    QByteArray document;
    QByteArray etag;
};

bool strongEtag(const QByteArray& etag);
// Endpoint is an exact HTTPS file URL inside a user-provisioned WebDAV folder.
// Credentials are supplied in memory; neither URL nor response body is logged.
std::optional<QNetworkRequest> requestFor(const QUrl& endpoint, const QString& user,
                                         const QString& password);
std::optional<QNetworkRequest> writeRequest(const QUrl& endpoint, const QString& user,
                                           const QString& password, const QByteArray& etag,
                                           bool createOnly);

class Client final : public QObject {
  public:
    using Completion = std::function<void(Result)>;
    explicit Client(QObject* parent = nullptr);
    void read(const QUrl& endpoint, const QString& user, const QString& password,
              Completion complete);
    void write(const QUrl& endpoint, const QString& user, const QString& password,
               const QByteArray& document, const QByteArray& etag, bool createOnly,
               Completion complete);

  private:
    void observe(QNetworkReply* reply, bool write, Completion complete);
    QNetworkAccessManager m_network;
};

} // namespace snow_shot::network::webdav
