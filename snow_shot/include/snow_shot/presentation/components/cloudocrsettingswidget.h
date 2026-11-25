#ifndef SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRSETTINGSWIDGET_H
#define SNOW_SHOT_PRESENTATION_COMPONENTS_CLOUDOCRSETTINGSWIDGET_H

#include "snow_shot/presentation/components/settingscustomwidget.h"
#include "snow_shot/network/baidupaddleocrclient.h"

#include <QCoreApplication>
#include <QPointer>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

class CloudOcrSettingsWidget final : public SettingsCustomWidget {
    Q_DECLARE_TR_FUNCTIONS(CloudOcrSettingsWidget)
  public:
    explicit CloudOcrSettingsWidget(QWidget* parent = nullptr);
    ~CloudOcrSettingsWidget() override;
    void applyTheme(const snow_shot::presentation::styles::ThemeColorScheme& scheme) override;
    void retranslateUi() override;

  private:
    void save();
    void testConnection();
    void setTesting(bool busy);
    void rebuildOutputFormats();
    void updateEngineUi();
    snow_shot::network::BaiduPaddleOcrClient::Options currentOptions() const;

    QLabel* m_explanation = nullptr;
    QLabel* m_engineLabel = nullptr;
    QLabel* m_outputLabel = nullptr;
    QLabel* m_endpointLabel = nullptr;
    QLabel* m_tokenLabel = nullptr;
    QLabel* m_modelLabel = nullptr;
    QLabel* m_status = nullptr;
    QComboBox* m_engine = nullptr;
    QComboBox* m_output = nullptr;
    QLineEdit* m_endpoint = nullptr;
    QLineEdit* m_token = nullptr;
    QComboBox* m_model = nullptr;
    QPushButton* m_save = nullptr;
    QPushButton* m_test = nullptr;
    snow_shot::network::BaiduPaddleOcrClient* m_probe = nullptr;
    bool m_testing = false;
};
#endif
