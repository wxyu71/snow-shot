// Small Qt-only editor backend for the fast settings logic contracts.
// The real AdQt backend is also compiled in this build; the full application
// test links and exercises it with the same CloudOcrSettingsWidget.
#include "snow_shot/presentation/components/cloudocrsettingscontrols.h"
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <utility>

namespace snow_shot::presentation::components {
CloudOcrSelect createCloudOcrSelect(QWidget* parent) {
    auto* editor = new QComboBox(parent);
    return {
        editor,
        [editor] { return editor->currentData().toString(); },
        [editor](const QString& value) { editor->setCurrentIndex(editor->findData(value)); },
        [editor](const CloudOcrChoices& choices) {
            editor->clear();
            for (const auto& choice : choices)
                editor->addItem(choice.second, choice.first);
        },
        [editor](QObject* receiver, std::function<void()> callback) {
            QObject::connect(editor, &QComboBox::currentIndexChanged, receiver,
                             [callback = std::move(callback)] { callback(); });
        },
    };
}

QLineEdit* createCloudOcrInput(bool password, QWidget* parent) {
    auto* editor = new QLineEdit(parent);
    if (password)
        editor->setEchoMode(QLineEdit::Password);
    return editor;
}

QPushButton* createCloudOcrButton(CloudOcrButtonRole, QWidget* parent) {
    return new QPushButton(parent);
}
} // namespace snow_shot::presentation::components
