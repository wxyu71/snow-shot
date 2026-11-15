#include "snow_shot/network/communitycloudocrsettings.h"

#include <QByteArray>
#include <QSettings>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

namespace snow_shot::network {
namespace {
QSettings store() {
    return QSettings(QSettings::IniFormat, QSettings::UserScope,
                     QStringLiteral("SnowShot"), QStringLiteral("CloudOcr"));
}
#ifdef Q_OS_WIN
QString protect(const QString& value) {
    QByteArray bytes = value.toUtf8();
    DATA_BLOB input{static_cast<DWORD>(bytes.size()),
                    reinterpret_cast<BYTE*>(bytes.data())};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Snow Shot Community OCR", nullptr, nullptr,
                          nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};
    QByteArray secured(reinterpret_cast<const char*>(output.pbData),
                       static_cast<qsizetype>(output.cbData));
    LocalFree(output.pbData);
    return QString::fromLatin1(secured.toBase64());
}
QString unprotect(const QString& value) {
    QByteArray bytes = QByteArray::fromBase64(value.toLatin1());
    DATA_BLOB input{static_cast<DWORD>(bytes.size()),
                    reinterpret_cast<BYTE*>(bytes.data())};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};
    const QByteArray clear(reinterpret_cast<const char*>(output.pbData),
                           static_cast<qsizetype>(output.cbData));
    LocalFree(output.pbData);
    return QString::fromUtf8(clear);
}
#endif
} // namespace

CommunityCloudOcrSettings readCommunityCloudOcrSettings() {
    auto settings = store();
    CommunityCloudOcrSettings result;
    result.apiUrl = settings.value(QStringLiteral("api_url"), result.apiUrl).toString();
    result.model = settings.value(QStringLiteral("model"), result.model).toString();
#ifdef Q_OS_WIN
    const QString secured = settings.value(QStringLiteral("access_token_dpapi_v1")).toString();
    if (!secured.isEmpty()) {
        // Do not fall back to an unprotected token if decryption fails.
        result.accessToken = unprotect(secured);
    } else
#endif
    {
        // Backwards compatible with the earlier beta; migrate on the next save.
        result.accessToken = settings.value(QStringLiteral("access_token")).toString();
    }
    return result;
}

bool writeCommunityCloudOcrSettings(const CommunityCloudOcrSettings& values) {
    auto settings = store();
#ifdef Q_OS_WIN
    const QString secured = protect(values.accessToken);
    if (!values.accessToken.isEmpty() && secured.isEmpty())
        return false;
    settings.setValue(QStringLiteral("access_token_dpapi_v1"), secured);
    settings.remove(QStringLiteral("access_token"));
#else
    settings.setValue(QStringLiteral("access_token"), values.accessToken);
#endif
    settings.setValue(QStringLiteral("api_url"), values.apiUrl);
    settings.setValue(QStringLiteral("model"), values.model);
    settings.sync();
    return settings.status() == QSettings::NoError;
}
} // namespace snow_shot::network

