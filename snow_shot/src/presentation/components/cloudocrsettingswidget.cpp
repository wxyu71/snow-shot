#include "snow_shot/presentation/components/cloudocrsettingswidget.h"
#include "snow_shot/network/communitycloudocrsettings.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

namespace {
namespace controls = snow_shot::presentation::components;

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

    auto* form = new QFormLayout;
    m_engine = controls::createCloudOcrSelect(this);
    m_engine.view->setObjectName(QStringLiteral("onlineOcrEngine"));
    m_engineLabel = new QLabel(this);
    form->addRow(m_engineLabel, m_engine.view);

    m_output = controls::createCloudOcrSelect(this);
    m_output.view->setObjectName(QStringLiteral("onlineOcrOutputFormat"));
    m_outputLabel = new QLabel(this);
    form->addRow(m_outputLabel, m_output.view);

    m_endpoint = controls::createCloudOcrInput(false, this);
    m_endpoint->setObjectName(QStringLiteral("paddleOcrApiUrl"));
    m_endpointLabel = new QLabel(this);
    form->addRow(m_endpointLabel, m_endpoint);
    m_token = controls::createCloudOcrInput(true, this);
    m_token->setObjectName(QStringLiteral("paddleOcrAccessToken"));
    m_tokenLabel = new QLabel(this);
    form->addRow(m_tokenLabel, m_token);
    m_model = controls::createCloudOcrSelect(this);
    m_model.view->setObjectName(QStringLiteral("paddleOcrModel"));
    m_modelLabel = new QLabel(this);
    form->addRow(m_modelLabel, m_model.view);

    outer->addLayout(form);

    auto* actions = new QHBoxLayout;
    m_save = controls::createCloudOcrButton(controls::CloudOcrButtonRole::Save, this);
    m_save->setObjectName(QStringLiteral("paddleOcrSave"));
    actions->addWidget(m_save);
    m_test = controls::createCloudOcrButton(controls::CloudOcrButtonRole::Test, this);
    m_test->setObjectName(QStringLiteral("paddleOcrTestConnection"));
    actions->addWidget(m_test);
    m_getToken = controls::createCloudOcrButton(controls::CloudOcrButtonRole::TokenLink, this);
    m_getToken->setObjectName(QStringLiteral("paddleOcrGetToken"));
    actions->addWidget(m_getToken);
    actions->addStretch();
    outer->addLayout(actions);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("paddleOcrConnectionStatus"));
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    outer->addWidget(m_status);

    const auto settings = snow_shot::network::readCommunityCloudOcrSettings();
    m_endpoint->setText(settings.apiUrl);
    m_token->setText(settings.accessToken);
    retranslateUi();
    m_model.select(settings.model);
    rebuildOutputFormats();
    m_output.select(settings.outputFormat);

    m_model.onChanged(this, [this] { rebuildOutputFormats(); });
    connect(m_getToken, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://aistudio.baidu.com/account/accessToken")));
    });
    connect(m_save, &QPushButton::clicked, this, [this] { save(); });
    connect(m_test, &QPushButton::clicked, this, [this] { testConnection(); });
    updateEngineUi();
}

CloudOcrSettingsWidget::~CloudOcrSettingsWidget() {
    if (m_probe)
        m_probe->cancel();
}

void CloudOcrSettingsWidget::applyTheme(
    const snow_shot::presentation::styles::ThemeColorScheme& scheme) {
    layout()->setSpacing(scheme.metricAlias.paddingSM);
    // AdQt editors obtain colors, focus, popup and sizing from the shared
    // application ThemeManager. Do not override their painting with QSS.
    for (auto* control : {m_engine.view, m_output.view, m_model.view,
                          static_cast<QWidget*>(m_endpoint), static_cast<QWidget*>(m_token),
                          static_cast<QWidget*>(m_save), static_cast<QWidget*>(m_test),
                          static_cast<QWidget*>(m_getToken)})
        control->setMinimumHeight(scheme.metricAlias.controlHeight);
}

void CloudOcrSettingsWidget::retranslateUi() {
    m_engineLabel->setText(tr("OCR engine"));
    m_outputLabel->setText(tr("Output format"));
    m_endpointLabel->setText(tr("PaddleOCR AI Studio API URL"));
    m_tokenLabel->setText(tr("Access Token"));
    m_modelLabel->setText(tr("PaddleOCR model"));
    m_save->setText(tr("Save"));
    m_test->setText(tr("Test connection"));
    m_getToken->setText(tr("Get Access Token"));
    m_getToken->setToolTip(tr("Open the official AI Studio Access Token page"));
    m_test->setToolTip(tr("Sends a small OCR sample directly to the selected PaddleOCR provider; "
                         "this can consume API quota."));

    const QString currentModel = m_model.value();
    const QSignalBlocker modelBlocker(m_model.view);
    m_engine.setChoices({{QStringLiteral("paddleocr"), tr("Baidu PaddleOCR (AI Studio)")}});
    m_engine.select(QStringLiteral("paddleocr"));
    m_model.setChoices({
        {QStringLiteral("auto"), tr("Automatic")},
        {QStringLiteral("PaddleOCR-VL-1.6"), QStringLiteral("PaddleOCR-VL-1.6")},
        {QStringLiteral("PP-StructureV3"), QStringLiteral("PP-StructureV3")},
        {QStringLiteral("PP-OCRv6"), QStringLiteral("PP-OCRv6")},
    });
    m_model.select(currentModel.isEmpty() ? QStringLiteral("auto") : currentModel);
    m_model.view->setToolTip(tr("Automatic selects a compatible model and switches on daily quota exhaustion."));
    rebuildOutputFormats();
}

void CloudOcrSettingsWidget::rebuildOutputFormats() {
    const QString previous = m_output.value();
    const QSignalBlocker blocker(m_output.view);
    const bool document = m_model.value() != QStringLiteral("PP-OCRv6");
    controls::CloudOcrChoices choices;
    if (document)
        choices.append({QStringLiteral("markdown"), QStringLiteral("Markdown")});
    choices.append({QStringLiteral("text"), tr("Plain text")});
    choices.append({QStringLiteral("jsonl"), QStringLiteral("JSONL")});
    if (document)
        choices.append({QStringLiteral("html"), tr("HTML (local conversion)")});
    QString selected = choices.first().first;
    for (const auto& choice : choices) {
        if (choice.first == previous)
            selected = previous;
    }
    m_output.setChoices(choices);
    m_output.select(selected);
}

void CloudOcrSettingsWidget::updateEngineUi() {
    m_endpoint->setEnabled(!m_testing);
    m_token->setEnabled(!m_testing);
    m_model.view->setEnabled(!m_testing);
    m_test->setEnabled(!m_testing);
    m_save->setEnabled(!m_testing);
    m_engine.view->setEnabled(false);
    m_output.view->setEnabled(!m_testing);
}

snow_shot::network::BaiduPaddleOcrClient::Options CloudOcrSettingsWidget::currentOptions() const {
    snow_shot::network::BaiduPaddleOcrClient::Options result;
    result.apiUrl = m_endpoint->text().trimmed();
    result.accessToken = m_token->text().trimmed();
    result.model = m_model.value();
    result.outputFormat = m_output.value();
    return result;
}

void CloudOcrSettingsWidget::save() {
    const auto options = currentOptions();
    if (!validCloudOcrEndpoint(options.apiUrl) || options.accessToken.isEmpty()) {
        m_status->setText(tr("Enter a valid HTTPS URL and Access Token for PaddleOCR."));
        return;
    }
    if (m_probe) {
        m_probe->cancel();
        m_probe->deleteLater();
        m_probe = nullptr;
        setTesting(false);
    }
    snow_shot::network::CommunityCloudOcrSettings settings{
        options.apiUrl, options.accessToken, options.model};
    settings.outputFormat = options.outputFormat;
    const bool saved = snow_shot::network::writeCommunityCloudOcrSettings(settings);
    m_status->setText(saved ? tr("PaddleOCR configuration saved.")
                            : tr("Could not securely save PaddleOCR configuration."));
}

void CloudOcrSettingsWidget::setTesting(bool busy) {
    m_testing = busy;
    updateEngineUi();
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
        if (!error.isEmpty())
            m_status->setText(tr("Connection failed: %1").arg(error));
        else if (result.trimmed().isEmpty())
            m_status->setText(tr("The OCR service returned an empty result."));
        else
            m_status->setText(tr("Connection successful. Model: %1").arg(probe->actualModel()));
    });
}
