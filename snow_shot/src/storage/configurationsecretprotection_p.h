#pragma once

#include <QJsonObject>
#include <QString>

namespace snow_shot::storage {

struct ConfigurationSecretLoadResult {
    bool rewrite = false;
    bool decryptionFailed = false;
};

[[nodiscard]] bool protectConfigurationSecrets(QJsonObject* document, QString* error);
[[nodiscard]] ConfigurationSecretLoadResult unprotectConfigurationSecrets(QJsonObject* document);

} // namespace snow_shot::storage
