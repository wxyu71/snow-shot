#include "snow_shot/presentation/components/cloudocrsettingswidget.h"
#include "snow_shot/network/communitycloudocrsettings.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {
bool validCloudOcrEndpoint(const QString& value) {
    const QUrl url(value);
    const QString host = url.host().toLower();
    return url.isValid() && !host.isEmpty() && url.userInfo().isEmpty() &&
           url.query().isEmpty() && url.fragment().isEmpty() &&
           (url.scheme() == QStringLiteral("https") ||
            (url.scheme() == QStringLiteral("http") &&
             (host == QStringLiteral("localhost") || host == QStringLiteral("127.0.0.1") ||
              host == QStringLiteral("::1"))));
}
} // namespace

CloudOcrSettingsWidget::CloudOcrSettingsWidget(QWidget* parent) : SettingsCustomWidget(parent) {
    setObjectName(QStringLiteral("settings-cloud-ocr"));
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    m_explanation = new QLabel(this);
    m_explanation->setWordWrap(true);
    outer->addWidget(m_explanation);

    auto* form = new QFormLayout;
    m_endpoint = new QLineEdit(this);
    m_endpoint->setObjectName(QStringLiteral("paddleOcrApiUrl"));
    m_endpointLabel = new QLabel(this);
    form->addRow(m_endpointLabel, m_endpoint);
    m_token = new QLineEdit(this);
    m_token->setObjectName(QStringLiteral("paddleOcrAccessToken"));
    m_token->setEchoMode(QLineEdit::Password);
    m_tokenLabel = new QLabel(this);
    form->addRow(m_tokenLabel, m_token);
    m_model = new QComboBox(this);
    m_model->setObjectName(QStringLiteral("paddleOcrModel"));
    m_model->addItems({QStringLiteral("PaddleOCR-VL-1.6"),
                       QStringLiteral("PP-StructureV3"),
                       QStringLiteral("PP-OCRv6")});
    m_modelLabel = new QLabel(this);
    form->addRow(m_modelLabel, m_model);
    outer->addLayout(form);

    auto* actions = new QHBoxLayout;
    m_save = new QPushButton(this);
    m_save->setObjectName(QStringLiteral("paddleOcrSave"));
    actions->addWidget(m_save);
    m_test = new QPushButton(this);
    m_test->setObjectName(QStringLiteral("paddleOcrTestConnection"));
    actions->addWidget(m_test);
    actions->addStretch();
    outer->addLayout(actions);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("paddleOcrConnectionStatus"));
    m_status->setWordWrap(true);
    outer->addWidget(m_status);

    const auto settings = snow_shot::network::readCommunityCloudOcrSettings();
    m_endpoint->setText(settings.apiUrl);
    m_token->setText(settings.accessToken);
    const QString model = settings.model;
    if (m_model->findText(model) >= 0)
        m_model->setCurrentText(model);

    connect(m_save, &QPushButton::clicked, this, [this] { save(); });
    connect(m_test, &QPushButton::clicked, this, [this] { testConnection(); });
    retranslateUi();
}

CloudOcrSettingsWidget::~CloudOcrSettingsWidget() {
    if (m_probe)
        m_probe->cancel();
}

void CloudOcrSettingsWidget::applyTheme(
    const snow_shot::presentation::styles::ThemeColorScheme& scheme) {
    const int spacing = scheme.metricAlias.paddingSM;
    layout()->setSpacing(spacing);
    m_endpoint->setMinimumHeight(scheme.metricAlias.controlHeight);
    m_token->setMinimumHeight(scheme.metricAlias.controlHeight);
    m_model->setMinimumHeight(scheme.metricAlias.controlHeight);
}

void CloudOcrSettingsWidget::retranslateUi() {
    m_explanation->setText(tr("Configure cloud document recognition once. Ctrl+D and local "
                              "text recognition continue to run offline."));
    m_endpointLabel->setText(tr("PaddleOCR AI Studio API URL"));
    m_tokenLabel->setText(tr("Access Token"));
    m_modelLabel->setText(tr("Recognition model"));
    m_save->setText(tr("Save"));
    m_test->setText(tr("Test connection"));
    m_test->setToolTip(tr("Sends a small OCR sample to the selected provider; "
                         "this can consume API quota."));
}

snow_shot::network::BaiduPaddleOcrClient::Options CloudOcrSettingsWidget::currentOptions() const {
    snow_shot::network::BaiduPaddleOcrClient::Options result;
    result.apiUrl = m_endpoint->text().trimmed();
    result.accessToken = m_token->text().trimmed();
    result.model = m_model->currentText();
    return result;
}

void CloudOcrSettingsWidget::save() {
    const auto options = currentOptions();
    if (!validCloudOcrEndpoint(options.apiUrl) || options.accessToken.isEmpty()) {
        m_status->setText(tr("Enter a valid HTTPS URL and Access Token."));
        return;
    }
    if (m_probe) {
        m_probe->cancel();
        m_probe->deleteLater();
        m_probe = nullptr;
        setTesting(false);
    }
    const bool saved = snow_shot::network::writeCommunityCloudOcrSettings(
        {options.apiUrl, options.accessToken, options.model});
    m_status->setText(saved ? tr("PaddleOCR configuration saved.")
                            : tr("Could not securely save PaddleOCR configuration."));
}

void CloudOcrSettingsWidget::setTesting(bool busy) {
    m_test->setEnabled(!busy);
    m_save->setEnabled(!busy);
}

void CloudOcrSettingsWidget::testConnection() {
    auto options = currentOptions();
    if (!validCloudOcrEndpoint(options.apiUrl) || options.accessToken.isEmpty()) {
        m_status->setText(tr("Enter a valid HTTPS URL and Access Token before testing."));
        return;
    }
    if (m_probe) {
        m_probe->cancel();
        m_probe->deleteLater();
    }
    m_probe = new snow_shot::network::BaiduPaddleOcrClient(this);
    setTesting(true);
    m_status->setText(tr("Testing the OCR service..."));

    // Exercise the same authenticated asynchronous API used by the Markdown tool.
    // A GET /health request would not verify AI Studio OCR authorization.
    QImage sample(480, 120, QImage::Format_RGB32);
    sample.fill(Qt::white);
    {
        QPainter painter(&sample);
        painter.setPen(Qt::black);
        QFont font = painter.font();
        font.setPixelSize(32);
        painter.setFont(font);
        painter.drawText(sample.rect(), Qt::AlignCenter, QStringLiteral("Snow Shot 123"));
    }
    auto* probe = m_probe;
    probe->recognize(sample, std::move(options),
                     [this, probe](const QString& result, const QString& error) {
        if (probe != m_probe)
            return;
        m_probe = nullptr;
        probe->deleteLater();
        setTesting(false);
        if (!error.isEmpty()) {
            m_status->setText(tr("Connection failed: %1").arg(error));
        } else if (result.trimmed().isEmpty()) {
            m_status->setText(tr("The OCR service returned an empty result."));
        } else {
            m_status->setText(tr("Connection successful. Cloud OCR returned recognized text."));
        }
    });
}

