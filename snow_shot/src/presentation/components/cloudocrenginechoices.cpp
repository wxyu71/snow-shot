#include "snow_shot/presentation/components/cloudocrenginechoices.h"

#include <QComboBox>
#include <QString>

namespace snow_shot::presentation::components {

void populateCloudOcrEngineChoices(QComboBox& combo, bool /*supportsVision*/) {
    combo.clear();
    // The Community product intentionally exposes one user-configured provider:
    // PaddleOCR. Preserve the stable Qt::UserRole ID for production settings.
    combo.addItem(QString{}, QStringLiteral("paddleocr"));
}

} // namespace snow_shot::presentation::components
