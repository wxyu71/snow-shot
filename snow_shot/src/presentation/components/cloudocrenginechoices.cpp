#include "snow_shot/presentation/components/cloudocrenginechoices.h"

#include <QComboBox>
#include <QString>

namespace snow_shot::presentation::components {

void populateCloudOcrEngineChoices(QComboBox& combo, bool supportsVision) {
    combo.clear();
    // Explicit QString{} selects QComboBox::addItem(text, userData), not
    // addItem(icon, text). The latter silently loses the engine identifier.
    combo.addItem(QString{}, QStringLiteral("paddleocr"));
    if (supportsVision)
        combo.addItem(QString{}, QStringLiteral("vision"));
}

} // namespace snow_shot::presentation::components
