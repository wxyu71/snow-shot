#include "configurationsecretprotection_p.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonValue>
#include <QtGlobal>

#include <initializer_list>
#include <limits>
#include <optional>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <wincrypt.h>
#endif

namespace snow_shot::storage {
namespace {

#ifdef Q_OS_WIN
const QString kProtectionMarker = QStringLiteral("$protected");
const QString kProtectionScheme = QStringLiteral("windows-dpapi-v1");
const QString kCiphertext = QStringLiteral("ciphertext");

std::optional<QString> protectSecret(const QString& value) {
    QByteArray clear = value.toUtf8();
    if (static_cast<quint64>(clear.size()) > std::numeric_limits<DWORD>::max())
        return std::nullopt;
    DATA_BLOB input{static_cast<DWORD>(clear.size()),
                    reinterpret_cast<BYTE*>(clear.data())};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Snow Shot Community configuration credential", nullptr, nullptr,
                          nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return std::nullopt;
    const QByteArray protectedBytes(reinterpret_cast<const char*>(output.pbData),
                                    static_cast<qsizetype>(output.cbData));
    LocalFree(output.pbData);
    return QString::fromLatin1(protectedBytes.toBase64());
}

std::optional<QString> unprotectSecret(const QString& ciphertext) {
    const QByteArray encoded = ciphertext.toLatin1();
    QByteArray protectedBytes = QByteArray::fromBase64(encoded);
    if (protectedBytes.isEmpty() || protectedBytes.toBase64() != encoded ||
        static_cast<quint64>(protectedBytes.size()) > std::numeric_limits<DWORD>::max())
        return std::nullopt;
    DATA_BLOB input{static_cast<DWORD>(protectedBytes.size()),
                    reinterpret_cast<BYTE*>(protectedBytes.data())};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output))
        return std::nullopt;
    const QByteArray clear(reinterpret_cast<const char*>(output.pbData),
                           static_cast<qsizetype>(output.cbData));
    LocalFree(output.pbData);
    return QString::fromUtf8(clear);
}

enum class SecretTransformMode { Protect, Unprotect };

struct SecretTransformResult {
    bool ok = true;
    bool rewrite = false;
    bool decryptionFailed = false;
};

void transformSecretField(QJsonObject* object, const QString& key, SecretTransformMode mode,
                          SecretTransformResult* result) {
    if (!object || !result || !object->contains(key))
        return;
    const QJsonValue stored = object->value(key);
    if (mode == SecretTransformMode::Protect) {
        if (!stored.isString() || stored.toString().isEmpty())
            return;
        const auto ciphertext = protectSecret(stored.toString());
        if (!ciphertext) {
            result->ok = false;
            return;
        }
        object->insert(key, QJsonObject{{kProtectionMarker, kProtectionScheme},
                                        {kCiphertext, *ciphertext}});
        return;
    }

    if (stored.isString()) {
        if (!stored.toString().isEmpty())
            result->rewrite = true;
        return;
    }
    if (!stored.isObject())
        return;
    const QJsonObject envelope = stored.toObject();
    if (envelope.value(kProtectionMarker).toString() != kProtectionScheme)
        return;
    const QJsonValue encoded = envelope.value(kCiphertext);
    const auto clear = encoded.isString() ? unprotectSecret(encoded.toString()) : std::nullopt;
    if (!clear) {
        object->insert(key, QString());
        result->decryptionFailed = true;
        result->rewrite = true;
        return;
    }
    object->insert(key, *clear);
}

void transformArray(QJsonArray* array, std::initializer_list<const char*> fields,
                    SecretTransformMode mode, SecretTransformResult* result) {
    if (!array)
        return;
    for (qsizetype index = 0; index < array->size(); ++index) {
        if (!array->at(index).isObject())
            continue;
        QJsonObject object = array->at(index).toObject();
        for (const char* field : fields)
            transformSecretField(&object, QString::fromLatin1(field), mode, result);
        (*array)[index] = object;
    }
}

SecretTransformResult transformConfigurationSecrets(QJsonObject* document,
                                                     SecretTransformMode mode) {
    SecretTransformResult result;
    if (!document) {
        result.ok = false;
        return result;
    }

    const QJsonValue cloudValue = document->value(QStringLiteral("cloud_upload"));
    if (cloudValue.isObject()) {
        QJsonObject cloud = cloudValue.toObject();
        const QJsonValue configurationValue = cloud.value(QStringLiteral("configuration"));
        if (configurationValue.isObject()) {
            QJsonObject configuration = configurationValue.toObject();
            const QJsonValue profilesValue = configuration.value(QStringLiteral("configurations"));
            if (profilesValue.isArray()) {
                QJsonArray profiles = profilesValue.toArray();
                transformArray(&profiles,
                               {"access_key_id", "secret_access_key", "session_token"},
                               mode, &result);
                configuration.insert(QStringLiteral("configurations"), profiles);
                cloud.insert(QStringLiteral("configuration"), configuration);
                document->insert(QStringLiteral("cloud_upload"), cloud);
            }
        }
    }

    const QJsonValue apiValue = document->value(QStringLiteral("api_configuration"));
    if (apiValue.isObject()) {
        QJsonObject api = apiValue.toObject();
        for (const char* name : {"custom_models", "text_translation"}) {
            const QString key = QString::fromLatin1(name);
            const QJsonValue modelsValue = api.value(key);
            if (!modelsValue.isArray())
                continue;
            QJsonArray models = modelsValue.toArray();
            transformArray(&models, {"api_key"}, mode, &result);
            api.insert(key, models);
        }
        document->insert(QStringLiteral("api_configuration"), api);
    }
    return result;
}
#endif

void redactSecretField(QJsonObject* object, const QString& key) {
    if (!object || !object->contains(key))
        return;
    const QJsonValue stored = object->value(key);
    if (stored.isString() && stored.toString().isEmpty())
        return;
    object->insert(key, QString());
}

void redactSecretArray(QJsonArray* array, std::initializer_list<const char*> fields) {
    if (!array)
        return;
    for (qsizetype index = 0; index < array->size(); ++index) {
        if (!array->at(index).isObject())
            continue;
        QJsonObject object = array->at(index).toObject();
        for (const char* field : fields)
            redactSecretField(&object, QString::fromLatin1(field));
        (*array)[index] = object;
    }
}

void redactSecrets(QJsonObject* document) {
    if (!document)
        return;

    const QJsonValue cloudValue = document->value(QStringLiteral("cloud_upload"));
    if (cloudValue.isObject()) {
        QJsonObject cloud = cloudValue.toObject();
        const QJsonValue configurationValue = cloud.value(QStringLiteral("configuration"));
        if (configurationValue.isObject()) {
            QJsonObject configuration = configurationValue.toObject();
            const QJsonValue profilesValue = configuration.value(QStringLiteral("configurations"));
            if (profilesValue.isArray()) {
                QJsonArray profiles = profilesValue.toArray();
                redactSecretArray(&profiles,
                                  {"access_key_id", "secret_access_key", "session_token"});
                configuration.insert(QStringLiteral("configurations"), profiles);
                cloud.insert(QStringLiteral("configuration"), configuration);
                document->insert(QStringLiteral("cloud_upload"), cloud);
            }
        }
    }

    const QJsonValue apiValue = document->value(QStringLiteral("api_configuration"));
    if (apiValue.isObject()) {
        QJsonObject api = apiValue.toObject();
        for (const char* name : {"custom_models", "text_translation"}) {
            const QString key = QString::fromLatin1(name);
            const QJsonValue modelsValue = api.value(key);
            if (!modelsValue.isArray())
                continue;
            QJsonArray models = modelsValue.toArray();
            redactSecretArray(&models, {"api_key"});
            api.insert(key, models);
        }
        document->insert(QStringLiteral("api_configuration"), api);
    }
}

} // namespace

bool protectConfigurationSecrets(QJsonObject* document, QString* error) {
#ifdef Q_OS_WIN
    const SecretTransformResult result =
        transformConfigurationSecrets(document, SecretTransformMode::Protect);
    if (!result.ok && error)
        *error = QStringLiteral("Unable to protect configuration credentials");
    return result.ok;
#else
    Q_UNUSED(document);
    if (error)
        error->clear();
    return true;
#endif
}

ConfigurationSecretLoadResult unprotectConfigurationSecrets(QJsonObject* document) {
#ifdef Q_OS_WIN
    const SecretTransformResult result =
        transformConfigurationSecrets(document, SecretTransformMode::Unprotect);
    return {result.rewrite, result.decryptionFailed};
#else
    Q_UNUSED(document);
    return {};
#endif
}

void redactConfigurationSecrets(QJsonObject* document) {
    redactSecrets(document);
}

} // namespace snow_shot::storage
