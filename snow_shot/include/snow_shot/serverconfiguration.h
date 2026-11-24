#ifndef SNOW_SHOT_SERVERCONFIGURATION_H
#define SNOW_SHOT_SERVERCONFIGURATION_H

#include <QString>
#include <QUrl>
#include <optional>

namespace snow_shot {
[[nodiscard]] inline bool secureServiceUrl(const QUrl& url) {
    if (url.scheme() == QStringLiteral("https"))
        return true;
    if (url.scheme() != QStringLiteral("http"))
        return false;
    const QString host = url.host().toLower();
    return host == QStringLiteral("localhost") || host == QStringLiteral("127.0.0.1") ||
           host == QStringLiteral("::1");
}

// Empty means no configured Community service.
[[nodiscard]] inline std::optional<QString> normalizedServerUrl(const QString& value) {
    QString normalized = value.trimmed();
    if (normalized.isEmpty())
        return normalized;
    const QUrl url(normalized, QUrl::StrictMode);
    if (!url.isValid() || url.isRelative() || url.host().isEmpty() ||
        !secureServiceUrl(url) || url.authority().contains(u'@') || url.hasQuery() ||
        url.hasFragment())
        return std::nullopt;
    normalized = url.toString(QUrl::FullyEncoded);
    while (normalized.endsWith(u'/'))
        normalized.chop(1);
    return normalized;
}
} // namespace snow_shot

#endif // SNOW_SHOT_SERVERCONFIGURATION_H
