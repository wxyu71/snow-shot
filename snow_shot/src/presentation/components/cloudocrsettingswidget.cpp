#include "snow_shot/presentation/components/cloudocrsettingswidget.h"
#include "snow_shot/network/communitycloudocrsettings.h"
#include "snow_shot/app/edition.h"
#include "snow_shot/customaimodelconfiguration.h"
#include "snow_shot/storage/applicationstorage.h"
#include "snow_shot/storage/settingsadapters.h"

#include <QComboBox>
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
    m_explanation = new QLabel(this);
    m_explanation->setWordWrap(true);
    outer->addWidget(m_explanation);

    auto* form = new QFormLayout;
    m_engine = new QComboBox(this);
    m_engine->setObjectName(QStringLiteral("onlineOcrEngine"));
    m_engine->addItem({}, QStringLiteral("paddleocr"));
#if SNOW_SHOT_ENABLE_IMAGE_CONVERSION
    m_engine->addItem({}, QStringLiteral("vision"));
#endif
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
    m_model->addItems({QStringLiteral("PaddleOCR-VL-1.6"), QStringLiteral("PP-StructureV3"),
                       QStringLiteral("PP-OCRv6")});
    m_modelLabel = new QLabel(this);
    form->addRow(m_modelLabel, m_model);

    m_visionModel = new QComboBox(this);
    m_visionModel->setObjectName(QStringLiteral("onlineOcrVisionModel"));
    m_visionModelLabel = new QLabel(this);
    form->addRow(m_visionModelLabel, m_visionModel);
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
    if (m_model->findText(settings.model) >= 0)
        m_model->setCurrentText(settings.model);
    const int engineIndex = m_engine->findData(settings.engine);
    if (engineIndex >= 0)
        m_engine->setCurrentIndex(engineIndex);
    rebuildOutputFormats(settings.outputFormat);
    reloadVisionModels();

    connect(m_engine, &QComboBox::currentIndexChanged, this, [this] {
        rebuildOutputFormats();
        updateEngineUi();
    });
    connect(m_save, &QPushButton::clicked, this, [this] { save(); });
    connect(m_test, &QPushButton::clicked, this, [this] { testConnection(); });
#if SNOW_SHOT_ENABLE_API_CONFIGURATION && SNOW_SHOT_ENABLE_IMAGE_CONVERSION
    connect(&snow_shot::storage::ApplicationStorage::instance().configuration(),
            &snow_shot::storage::ConfigurationStore::valueChanged, this,
            [this](const QString& key, const QJsonValue&) {
                if (key == QStringLiteral("api_configuration/custom_models"))
                    reloadVisionModels();
            });
#endif
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
    for (auto* control : {static_cast<QWidget*>(m_engine), static_cast<QWidget*>(m_output),
                          static_cast<QWidget*>(m_endpoint), static_cast<QWidget*>(m_token),
                          static_cast<QWidget*>(m_model), static_cast<QWidget*>(m_visionModel)})
        control->setMinimumHeight(scheme.metricAlias.controlHeight);
}

void CloudOcrSettingsWidget::retranslateUi() {
    m_explanation->setText(
        tr("The Online OCR button uses the engine and output selected here. PaddleOCR connects "
           "directly to AI Studio and returns Markdown with formulas preserved as LaTeX; "
           "large-model OCR can produce Markdown, HTML, or LaTeX through your configured "
           "OpenAI-compatible vision model. Local text recognition remains offline."));
    m_engineLabel->setText(tr("OCR engine"));
    m_outputLabel->setText(tr("Output format"));
    m_endpointLabel->setText(tr("PaddleOCR AI Studio API URL"));
    m_tokenLabel->setText(tr("Access Token"));
    m_modelLabel->setText(tr("PaddleOCR model"));
    m_visionModelLabel->setText(tr("Large-model vision model"));
    m_save->setText(tr("Save"));
    m_test->setText(tr("Test PaddleOCR connection"));
    m_test->setToolTip(tr("Sends a small OCR sample directly to the selected PaddleOCR provider; "
                         "this can consume API quota."));

    for (int i = 0; i < m_engine->count(); ++i) {
        const QString value = m_engine->itemData(i).toString();
        m_engine->setItemText(
            i, value == QStringLiteral("vision") ? tr("OpenAI-compatible vision model")
                                                 : tr("Baidu PaddleOCR (AI Studio)"));
    }
    for (int i = 0; i < m_output->count(); ++i) {
        const QString value = m_output->itemData(i).toString();
        m_output->setItemText(
            i, value == QStringLiteral("latex")
                   ? tr("LaTeX")
                   : value == QStringLiteral("html")
                         ? tr("HTML")
                         : selectedEngine() == QStringLiteral("paddleocr")
                               ? tr("Markdown (formulas preserved as LaTeX)")
                               : tr("Markdown"));
    }
    if (m_visionModel->count() == 1 && m_visionModel->itemData(0).toString().isEmpty())
        m_visionModel->setItemText(0, tr("No custom vision models configured"));
}

QString CloudOcrSettingsWidget::selectedEngine() const {
    const QString value = m_engine->currentData().toString();
    return value == QStringLiteral("vision") ? value : QStringLiteral("paddleocr");
}

QString CloudOcrSettingsWidget::selectedOutputFormat() const {
    const QString value = m_output->currentData().toString();
    if (value == QStringLiteral("html") || value == QStringLiteral("latex"))
        return value;
    return QStringLiteral("markdown");
}

void CloudOcrSettingsWidget::rebuildOutputFormats(const QString& preferred) {
    const QString previous = preferred.isEmpty() ? selectedOutputFormat() : preferred;
    QSignalBlocker blocker(m_output);
    m_output->clear();
    m_output->addItem(selectedEngine() == QStringLiteral("paddleocr")
                          ? tr("Markdown (formulas preserved as LaTeX)")
                          : tr("Markdown"),
                      QStringLiteral("markdown"));
#if SNOW_SHOT_ENABLE_IMAGE_CONVERSION
    if (selectedEngine() == QStringLiteral("vision")) {
        m_output->addItem(tr("HTML"), QStringLiteral("html"));
#if SNOW_SHOT_ENABLE_LATEX_RECOGNITION
        m_output->addItem(tr("LaTeX"), QStringLiteral("latex"));
#endif
    }
#endif
    const int index = m_output->findData(previous);
    m_output->setCurrentIndex(index >= 0 ? index : 0);
}

void CloudOcrSettingsWidget::reloadVisionModels() {
    QSignalBlocker blocker(m_visionModel);
    QString selected;
#if SNOW_SHOT_ENABLE_IMAGE_CONVERSION
    selected = snow_shot::storage::ScreenshotImageConversionSettings().visionModel();
#endif
    m_visionModel->clear();
#if SNOW_SHOT_ENABLE_API_CONFIGURATION && SNOW_SHOT_ENABLE_IMAGE_CONVERSION
    const auto raw = snow_shot::storage::ApplicationStorage::instance().configuration().value(
        QStringLiteral("api_configuration/custom_models"));
    const auto models = snow_shot::customAiModelsFromJson(raw);
    for (const auto& model : models)
        if (model.supportsVision)
            m_visionModel->addItem(model.name, model.selectionId());
#endif
    if (m_visionModel->count() == 0)
        m_visionModel->addItem(tr("No custom vision models configured"), QString{});
    const int index = m_visionModel->findData(selected);
    m_visionModel->setCurrentIndex(index >= 0 ? index : 0);
    updateEngineUi();
}

void CloudOcrSettingsWidget::updateEngineUi() {
    const bool paddle = selectedEngine() == QStringLiteral("paddleocr");
    m_endpoint->setEnabled(!m_testing && paddle);
    m_token->setEnabled(!m_testing && paddle);
    m_model->setEnabled(!m_testing && paddle);
    m_test->setEnabled(!m_testing && paddle);
    m_visionModel->setEnabled(!m_testing && !paddle &&
                              !m_visionModel->currentData().toString().isEmpty());
    m_output->setEnabled(!m_testing);
    m_engine->setEnabled(!m_testing);
    m_save->setEnabled(!m_testing);
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
    const QString engine = selectedEngine();
    const QString output = selectedOutputFormat();
    if (engine == QStringLiteral("paddleocr") &&
        (!validCloudOcrEndpoint(options.apiUrl) || options.accessToken.isEmpty())) {
        m_status->setText(tr("Enter a valid HTTPS URL and Access Token for PaddleOCR."));
        return;
    }
#if SNOW_SHOT_ENABLE_IMAGE_CONVERSION
    if (engine == QStringLiteral("vision")) {
        const QString model = m_visionModel->currentData().toString();
        if (model.isEmpty()) {
            m_status->setText(
                tr("Configure an OpenAI-compatible custom model with vision support first."));
            return;
        }
        if (!snow_shot::storage::ScreenshotImageConversionSettings().setVisionModel(model)) {
            m_status->setText(tr("Could not save the large-model vision selection."));
            return;
        }
#if SNOW_SHOT_ENABLE_LATEX_RECOGNITION
        if (output == QStringLiteral("latex") &&
            !snow_shot::storage::ScreenshotRecognitionModelSettings().setLatexModel(model)) {
            m_status->setText(tr("Could not save the LaTeX vision model selection."));
            return;
        }
#endif
    }
#endif
    if (m_probe) {
        m_probe->cancel();
        m_probe->deleteLater();
        m_probe = nullptr;
        setTesting(false);
    }
    snow_shot::network::CommunityCloudOcrSettings settings{
        options.apiUrl, options.accessToken, options.model};
    settings.engine = engine;
    settings.outputFormat = output;
    const bool saved = snow_shot::network::writeCommunityCloudOcrSettings(settings);
    m_status->setText(saved ? tr("Online OCR configuration saved.")
                            : tr("Could not securely save Online OCR configuration."));
}

void CloudOcrSettingsWidget::setTesting(bool busy) {
    m_testing = busy;
    updateEngineUi();
}

void CloudOcrSettingsWidget::testConnection() {
    if (selectedEngine() != QStringLiteral("paddleocr")) {
        m_status->setText(
            tr("Large-model OCR uses the custom vision model configured in Connections & services."));
        return;
    }
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
