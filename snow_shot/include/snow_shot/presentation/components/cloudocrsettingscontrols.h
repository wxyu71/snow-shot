#ifndef SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRSETTINGSCONTROLS_H
#define SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRSETTINGSCONTROLS_H

#include <QString>
#include <QVector>
#include <QPair>
#include <functional>

class QWidget;
class QLineEdit;
class QPushButton;
class QObject;

namespace snow_shot::presentation::components {
// Keep settings logic independent of the editor backend so the small native
// contracts can exercise it without linking the entire application UI stack.
using CloudOcrChoices = QVector<QPair<QString, QString>>; // stable value, display label
struct CloudOcrSelect {
    QWidget* view = nullptr;
    std::function<QString()> value;
    std::function<void(const QString&)> select;
    std::function<void(const CloudOcrChoices&)> setChoices;
    std::function<void(QObject*, std::function<void()>)> onChanged;
};

enum class CloudOcrButtonRole { Save, Test, TokenLink };
[[nodiscard]] CloudOcrSelect createCloudOcrSelect(QWidget* parent);
[[nodiscard]] QLineEdit* createCloudOcrInput(bool password, QWidget* parent);
[[nodiscard]] QPushButton* createCloudOcrButton(CloudOcrButtonRole role, QWidget* parent);
} // namespace snow_shot::presentation::components
#endif
