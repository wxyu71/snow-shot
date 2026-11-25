#include "snow_shot/presentation/components/cloudocrsettingscontrols.h"

#include "widgets/button.h"
#include "widgets/combo_box.h"
#include "widgets/input_line_edit.h"
#include "widgets/input_password_edit.h"

#include <utility>

namespace snow_shot::presentation::components {
using namespace adqt::widgets;

CloudOcrSelect createCloudOcrSelect(QWidget* parent) {
    auto* editor = new AdComboBox(parent);
    editor->setSearchable(false);
    editor->setEditable(false);
    editor->setAllowClear(false);
    return {
        editor,
        [editor] { return editor->currentValue().toString(); },
        [editor](const QString& value) { editor->setCurrentValue(value); },
        [editor](const CloudOcrChoices& choices) {
            QVector<AdComboBox::Option> options;
            options.reserve(choices.size());
            for (const auto& choice : choices)
                options.append({choice.first, choice.second});
            editor->setOptions(options);
        },
        [editor](QObject* receiver, std::function<void()> callback) {
            QObject::connect(editor, &AdComboBox::currentValueChanged, receiver,
                             [callback = std::move(callback)] { callback(); });
        },
    };
}

QLineEdit* createCloudOcrInput(bool password, QWidget* parent) {
    if (password)
        return new AdPasswordEdit(parent);
    return new AdLineEdit(parent);
}

QPushButton* createCloudOcrButton(CloudOcrButtonRole role, QWidget* parent) {
    auto* button = new AdButton(parent);
    button->setShape(AdButton::Shape::Rounded);
    if (role == CloudOcrButtonRole::Save) {
        button->setButtonStyle(AdButton::ButtonStyle::Solid);
        button->setAccentRole(AdButton::AccentRole::Primary);
    } else if (role == CloudOcrButtonRole::TokenLink) {
        button->setButtonStyle(AdButton::ButtonStyle::Link);
        button->setAccentRole(AdButton::AccentRole::Primary);
    } else {
        button->setButtonStyle(AdButton::ButtonStyle::Outline);
        button->setAccentRole(AdButton::AccentRole::Neutral);
    }
    return button;
}
} // namespace snow_shot::presentation::components
