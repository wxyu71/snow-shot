#include "snow_shot/presentation/components/cloudocrsettingswidget.h"
#include "snow_shot/presentation/components/cloudocrenginechoices.h"
#include "snow_shot/network/communitycloudocrsettings.h"

#include <QComboBox>
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
    m_engine = new QComboBox(this);
    m_engine->setObjectName(QStringLiteral("onlineOcrEngine"));
    // Use the text + userData overload, not the QIcon + text overload.
    // An empty braced first argument would silently discard the engine ID.
    // This exact production option population is compiled in fast native Qt tests.
    snow_shot::presentation::components::populateCloudOcrEngineChoices(*m_engine, false);
    m_engineLabel = new QLabel(this);
    form->addRow(m_engineLabel, m_engine);

    m_output = new QComboBox(this);
    m_output->setObjectName(QStringLiteral("onlineOcrOutputFormat"));
    m_outputLabel = new QLabel(this);
    form->addRow(m_outputLabel, m_output);

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
    m_model->addItem(tr("Automatic"), QStringLiteral("auto"));
    for (const auto& model : {QStringLiteral("PaddleOCR-VL-1.6"),
                              QStringLiteral("PP-StructureV3"), QStringLiteral("PP-OCRv6")})
        m_model->addItem(model, model);
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
    m_getToken = new QPushButton(this);
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
    if (m_model->findData(settings.model) >= 0)
        m_model->setCurrentIndex(m_model->findData(settings.model));
    const int engineIndex = m_engine->findData(settings.engine);
    if (engineIndex >= 0)
        m_engine->setCurrentIndex(engineIndex);
    rebuildOutputFormats();
    m_output->setCurrentIndex(m_output->findData(settings.outputFormat));

    connect(m_model, &QComboBox::currentIndexChanged, this, [this] { rebuildOutputFormats(); });
    connect(m_getToken, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://aistudio.baidu.com/account/accessToken")));
    });
    connect(m_save, &QPushButton::clicked, this, [this] { save(); });
    connect(m_test, &QPushButton::clicked, this, [this] { testConnection(); });
    retranslateUi();
    updateEngineUi();
}

CloudOcrSettingsWidget::~CloudOcrSettingsWidget() {
    if (m_probe)
        m_probe->cancel();
}

void CloudOcrSettingsWidget::applyTheme(
    const snow_shot::presentation::styles::ThemeColorScheme& scheme) {
    const int spacing = scheme.metricAlias.paddingSM;
    layout()->setSpacing(spacing);
    const auto& colors = scheme.map;
    const auto& metrics = scheme.metricAlias;
    const auto css = [](const QColor& color) { return color.name(QColor::HexArgb); };
    const QString fieldsStyle = QStringLiteral(
        "QComboBox, QLineEdit { color: %1; background: %2; border: %3px solid %4; "
        "border-radius: %5px; padding: 0 %6px; }"
        "QComboBox:hover, QLineEdit:hover { border-color: %7; }"
        "QComboBox:focus, QLineEdit:focus { border-color: %8; }"
        "QComboBox:disabled, QLineEdit:disabled { color: %9; background: %10; border-color: %11; }"
        "QComboBox QAbstractItemView { color: %1; background: %12; "
        "selection-background-color: %13; selection-color: %1; border: %3px solid %4; }")
        .arg(css(colors.colorText), css(colors.colorBgContainer))
        .arg(metrics.lineWidth).arg(css(colors.colorBorder)).arg(metrics.borderRadius)
        .arg(metrics.paddingXS).arg(css(colors.colorPrimaryHover), css(colors.colorPrimary),
             css(colors.colorTextQuaternary), css(colors.colorFillTertiary),
             css(colors.colorBorderDisabled), css(colors.colorBgElevated),
             css(colors.colorPrimaryBg));
    for (auto* control : {static_cast<QWidget*>(m_engine), static_cast<QWidget*>(m_output),
                          static_cast<QWidget*>(m_endpoint), static_cast<QWidget*>(m_token),
                          static_cast<QWidget*>(m_model)}) {
        control->setMinimumHeight(metrics.controlHeight);
        control->setStyleSheet(fieldsStyle);
    }
    const QString buttonStyle = QStringLiteral(
        "QPushButton { color: %1; background: %2; border: %3px solid %4; "
        "border-radius: %5px; padding: 0 %6px; }"
        "QPushButton:hover { color: %7; border-color: %7; }"
        "QPushButton:disabled { color: %8; background: %9; border-color: %10; }")
        .arg(css(colors.colorText), css(colors.colorBgContainer))
        .arg(metrics.lineWidth).arg(css(colors.colorBorder)).arg(metrics.borderRadius)
        .arg(metrics.paddingSM).arg(css(colors.colorPrimaryHover),
             css(colors.colorTextQuaternary), css(colors.colorFillTertiary),
             css(colors.colorBorderDisabled));
    for (auto* button : {m_save, m_test, m_getToken}) {
        button->setMinimumHeight(metrics.controlHeight);
        button->setStyleSheet(buttonStyle);
    }
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

    m_engine->setItemText(0, tr("Baidu PaddleOCR (AI Studio)"));
    m_model->setItemText(0, tr("Automatic"));
    m_model->setToolTip(tr("Automatic selects a compatible model and switches on daily quota exhaustion."));
    rebuildOutputFormats();
}

void CloudOcrSettingsWidget::rebuildOutputFormats() {
    const QString previous = m_output->currentData().toString();
    const QSignalBlocker blocker(m_output);
    m_output->clear();
    if (m_model->currentData().toString() != QStringLiteral("PP-OCRv6"))
        m_output->addItem(QStringLiteral("Markdown"), QStringLiteral("markdown"));
    m_output->addItem(tr("Plain text"), QStringLiteral("text"));
    m_output->addItem(QStringLiteral("JSONL"), QStringLiteral("jsonl"));
    if (m_model->currentData().toString() != QStringLiteral("PP-OCRv6"))
        m_output->addItem(tr("HTML (local conversion)"), QStringLiteral("html"));
    const int index = m_output->findData(previous);
    m_output->setCurrentIndex(index < 0 ? 0 : index);
}

void CloudOcrSettingsWidget::updateEngineUi() {
    m_endpoint->setEnabled(!m_testing);
    m_token->setEnabled(!m_testing);
    m_model->setEnabled(!m_testing);
    m_test->setEnabled(!m_testing);
    m_save->setEnabled(!m_testing);
    m_engine->setEnabled(false);
    m_output->setEnabled(!m_testing);
}

snow_shot::network::BaiduPaddleOcrClient::Options CloudOcrSettingsWidget::currentOptions() const {
    snow_shot::network::BaiduPaddleOcrClient::Options result;
    result.apiUrl = m_endpoint->text().trimmed();
    result.accessToken = m_token->text().trimmed();
    result.model = m_model->currentData().toString();
    result.outputFormat = m_output->currentData().toString();
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
            m_status->setText(tr("Connection successful. PaddleOCR returned recognized text."));
    });
}
