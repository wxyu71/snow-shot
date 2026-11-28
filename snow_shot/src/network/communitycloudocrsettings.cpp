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

QString normalizedModel(const QString& value) {
    if (value == QStringLiteral("PaddleOCR-VL-1.6") ||
        value == QStringLiteral("PP-StructureV3") ||
        value == QStringLiteral("PP-OCRv6"))
        return value;
    return QStringLiteral("auto");
}

QString normalizedOutput(const QString& value, const QString& model) {
    if (value == QStringLiteral("text") || value == QStringLiteral("jsonl"))
        return value;
    if (model == QStringLiteral("PP-OCRv6"))
        return QStringLiteral("text");
    return value == QStringLiteral("html") ? value : QStringLiteral("markdown");
}

} // namespace

CommunityCloudOcrSettings readCommunityCloudOcrSettings() {
    auto settings = store();
    CommunityCloudOcrSettings result;
    result.apiUrl = settings.value(QStringLiteral("api_url"), result.apiUrl).toString();
    result.model = normalizedModel(settings.value(QStringLiteral("model"), result.model).toString());
    // Legacy Vision preferences are not PaddleOCR preferences. The versioned
    // output key prevents an old HTML/LaTeX selection from changing the new route.
    result.engine = QStringLiteral("paddleocr");
    result.outputFormat = normalizedOutput(
        settings.value(QStringLiteral("paddle_output_format_v2"), result.outputFormat).toString(),
        result.model);
#ifdef Q_OS_WIN
    const QString secured = settings.value(QStringLiteral("access_token_dpapi_v1")).toString();
    if (!secured.isEmpty())
        result.accessToken = unprotect(secured);
#else
    result.accessToken = settings.value(QStringLiteral("access_token")).toString();
#endif
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
    const QString model = normalizedModel(values.model);
    const QString output = normalizedOutput(values.outputFormat, model);
    settings.setValue(QStringLiteral("model"), model);
    settings.setValue(QStringLiteral("engine"), QStringLiteral("paddleocr"));
    settings.setValue(QStringLiteral("output_format"), output);
    settings.setValue(QStringLiteral("paddle_output_format_v2"), output);
    settings.sync();
    return settings.status() == QSettings::NoError;
}
} // namespace snow_shot::network

