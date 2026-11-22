#ifndef SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRENGINECHOICES_H
#define SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRENGINECHOICES_H

class QComboBox;

namespace snow_shot::presentation::components {

// The tested production source of the OCR engine's Qt::UserRole identifiers.
// Labels are translated by CloudOcrSettingsWidget::retranslateUi().
void populateCloudOcrEngineChoices(QComboBox& combo, bool supportsVision);

} // namespace snow_shot::presentation::components

#endif // SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRENGINECHOICES_H
