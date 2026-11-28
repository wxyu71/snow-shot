#include "snow_shot/network/communitycloudocrsettings.h"
#include "snow_shot/presentation/components/cloudocrsettingswidget.h"

#ifdef SNOW_CLOUD_OCR_ADQT_TEST
#include "widgets/combo_box.h"
#include "widgets/button.h"
#include "widgets/input_line_edit.h"
#include "widgets/input_password_edit.h"
using OcrCombo = adqt::widgets::AdComboBox;
#else
#include <QComboBox>
using OcrCombo = QComboBox;
#endif
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTranslator>
#include <QtTest/QtTest>

namespace {
int comboCount(OcrCombo* combo) {
#ifdef SNOW_CLOUD_OCR_ADQT_TEST
    return static_cast<int>(combo->options().size());
#else
    return combo->count();
#endif
}

int comboFind(OcrCombo* combo, const QString& value) {
#ifdef SNOW_CLOUD_OCR_ADQT_TEST
    const auto options = combo->options();
    for (qsizetype i = 0; i < options.size(); ++i)
        if (options.at(i).value.toString() == value)
            return static_cast<int>(i);
    return -1;
#else
    return combo->findData(value);
#endif
}

QString comboValue(OcrCombo* combo) {
#ifdef SNOW_CLOUD_OCR_ADQT_TEST
    return combo->currentValue().toString();
#else
    return combo->currentData().toString();
#endif
}

void comboSelect(OcrCombo* combo, const QString& value) {
#ifdef SNOW_CLOUD_OCR_ADQT_TEST
    combo->setCurrentValue(value);
#else
    combo->setCurrentIndex(combo->findData(value));
#endif
}

class CloudOcrContextTranslator final : public QTranslator {
  public:
    bool isEmpty() const override {
        return false;
    }

    QString translate(const char* context, const char* sourceText, const char*,
                      int) const override {
        if (QByteArray(context) != QByteArrayLiteral("CloudOcrSettingsWidget"))
            return {};
        return QStringLiteral("translated: ") + QString::fromUtf8(sourceText);
    }
};
} // namespace

class CommunityCloudOcrWidgetTests final : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() {
        QVERIFY(m_settingsDirectory.isValid());
        const QString root = m_settingsDirectory.path();
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, root);
    }

    void init() {
        const QString organization = QStringLiteral("SnowShot");
        const QString application = QStringLiteral("CloudOcr");
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, organization, application);
        settings.clear();
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
    }

    void productionWidgetExposesPaddleAndCompatibleFormats() {
        CloudOcrSettingsWidget widget;
        auto* engine = widget.findChild<OcrCombo*>(QStringLiteral("onlineOcrEngine"));
        auto* output = widget.findChild<OcrCombo*>(QStringLiteral("onlineOcrOutputFormat"));
        auto* endpoint = widget.findChild<QLineEdit*>(QStringLiteral("paddleOcrApiUrl"));
        auto* token = widget.findChild<QLineEdit*>(QStringLiteral("paddleOcrAccessToken"));
        auto* model = widget.findChild<OcrCombo*>(QStringLiteral("paddleOcrModel"));
        auto* save = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrSave"));
        auto* test = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrTestConnection"));
        auto* status = widget.findChild<QLabel*>(QStringLiteral("paddleOcrConnectionStatus"));

        QVERIFY(engine);
        QVERIFY(output);
        QVERIFY(endpoint);
        QVERIFY(token);
        QVERIFY(model);
        QVERIFY(save);
        QVERIFY(test);
        QVERIFY(status);
#ifdef SNOW_CLOUD_OCR_ADQT_TEST
        QVERIFY(qobject_cast<adqt::widgets::AdLineEdit*>(endpoint));
        QVERIFY(qobject_cast<adqt::widgets::AdPasswordEdit*>(token));
        QVERIFY(qobject_cast<adqt::widgets::AdButton*>(save));
        QVERIFY(qobject_cast<adqt::widgets::AdButton*>(test));
#endif
        QVERIFY(!widget.findChild<OcrCombo*>(QStringLiteral("onlineOcrVisionModel")));
        QCOMPARE(comboCount(engine), 1);
        QCOMPARE(comboValue(engine), QStringLiteral("paddleocr"));
        QCOMPARE(comboFind(engine, QStringLiteral("vision")), -1);
        QVERIFY(!engine->isEnabled());
        QCOMPARE(comboCount(output), 4);
        QCOMPARE(comboValue(output), QStringLiteral("markdown"));
        QVERIFY(comboFind(output, QStringLiteral("html")) >= 0);
        QVERIFY(comboFind(output, QStringLiteral("jsonl")) >= 0);
        QVERIFY(comboFind(output, QStringLiteral("text")) >= 0);
        QCOMPARE(comboFind(output, QStringLiteral("latex")), -1);
        QVERIFY(output->isEnabled());
        QCOMPARE(token->echoMode(), QLineEdit::Password);
        QCOMPARE(comboCount(model), 4);
        QCOMPARE(comboValue(model), QStringLiteral("auto"));
        QVERIFY(comboFind(model, QStringLiteral("PaddleOCR-VL-1.6")) >= 0);
        QVERIFY(comboFind(model, QStringLiteral("PP-StructureV3")) >= 0);
        QVERIFY(comboFind(model, QStringLiteral("PP-OCRv6")) >= 0);
    }

    void widgetUsesItsOwnTranslationContextAndKeepsModelValues() {
        CloudOcrContextTranslator translator;
        QVERIFY(QCoreApplication::installTranslator(&translator));
        CloudOcrSettingsWidget widget;
        auto* engine = widget.findChild<OcrCombo*>(QStringLiteral("onlineOcrEngine"));
        auto* output = widget.findChild<OcrCombo*>(QStringLiteral("onlineOcrOutputFormat"));
        auto* model = widget.findChild<OcrCombo*>(QStringLiteral("paddleOcrModel"));
        auto* save = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrSave"));
        auto* test = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrTestConnection"));
        QVERIFY(engine);
        QVERIFY(output);
        QVERIFY(model);
        QVERIFY(save);
        QVERIFY(test);
        QCOMPARE(engine->currentText(), QStringLiteral("translated: Baidu PaddleOCR (AI Studio)"));
        QCOMPARE(output->currentText(), QStringLiteral("Markdown"));
        QCOMPARE(save->text(), QStringLiteral("translated: Save"));
        QCOMPARE(test->text(), QStringLiteral("translated: Test connection"));
        QCOMPARE(comboValue(engine), QStringLiteral("paddleocr"));
        QCOMPARE(comboValue(output), QStringLiteral("markdown"));
        QCOMPARE(model->currentText(), QStringLiteral("translated: Automatic"));
        QCOMPARE(comboValue(model), QStringLiteral("auto"));
        QCoreApplication::removeTranslator(&translator);
        widget.retranslateUi();
        QCOMPARE(save->text(), QStringLiteral("Save"));
    }

    void widgetSavesPaddleAndNormalizesLegacyChoices() {
        const QString organization = QStringLiteral("SnowShot");
        const QString application = QStringLiteral("CloudOcr");
        QSettings legacy(QSettings::IniFormat, QSettings::UserScope, organization, application);
        legacy.setValue(QStringLiteral("engine"), QStringLiteral("vision"));
        legacy.setValue(QStringLiteral("output_format"), QStringLiteral("latex"));
        legacy.sync();
        QCOMPARE(legacy.status(), QSettings::NoError);

        const auto normalized = snow_shot::network::readCommunityCloudOcrSettings();
        QCOMPARE(normalized.engine, QStringLiteral("paddleocr"));
        QCOMPARE(normalized.outputFormat, QStringLiteral("markdown"));

        CloudOcrSettingsWidget widget;
        widget.show();
        auto* endpoint = widget.findChild<QLineEdit*>(QStringLiteral("paddleOcrApiUrl"));
        auto* token = widget.findChild<QLineEdit*>(QStringLiteral("paddleOcrAccessToken"));
        auto* model = widget.findChild<OcrCombo*>(QStringLiteral("paddleOcrModel"));
        auto* save = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrSave"));
        auto* status = widget.findChild<QLabel*>(QStringLiteral("paddleOcrConnectionStatus"));
        QVERIFY(endpoint);
        QVERIFY(token);
        QVERIFY(model);
        QVERIFY(save);
        QVERIFY(status);

        endpoint->setText(QStringLiteral("not-a-url"));
        token->setText(QStringLiteral("test-token"));
        QTest::mouseClick(save, Qt::LeftButton);
        QVERIFY(status->text().contains(QStringLiteral("valid HTTPS URL")));

        endpoint->setText(QStringLiteral("http://127.0.0.1:5102"));
        comboSelect(model, QStringLiteral("PP-OCRv6"));
        auto* output = widget.findChild<OcrCombo*>(QStringLiteral("onlineOcrOutputFormat"));
        QVERIFY(output);
        QCOMPARE(comboCount(output), 2);
        QCOMPARE(comboValue(output), QStringLiteral("text"));
        comboSelect(output, QStringLiteral("jsonl"));
        QTest::mouseClick(save, Qt::LeftButton);
        QCOMPARE(status->text(), QStringLiteral("PaddleOCR configuration saved."));

        const auto stored = snow_shot::network::readCommunityCloudOcrSettings();
        QCOMPARE(stored.apiUrl, QStringLiteral("http://127.0.0.1:5102"));
        QCOMPARE(stored.accessToken, QStringLiteral("test-token"));
        QCOMPARE(stored.model, QStringLiteral("PP-OCRv6"));
        QCOMPARE(stored.engine, QStringLiteral("paddleocr"));
        QCOMPARE(stored.outputFormat, QStringLiteral("jsonl"));

        QSettings persisted(QSettings::IniFormat, QSettings::UserScope, organization, application);
        const QString persistedEngine = persisted.value(QStringLiteral("engine")).toString();
        const QString persistedOutput = persisted.value(QStringLiteral("output_format")).toString();
        QCOMPARE(persistedEngine, QStringLiteral("paddleocr"));
        QCOMPARE(persistedOutput, QStringLiteral("jsonl"));
#ifdef Q_OS_WIN
        QVERIFY(persisted.value(QStringLiteral("access_token")).toString().isEmpty());
        QVERIFY(!persisted.value(QStringLiteral("access_token_dpapi_v1")).toString().isEmpty());
#endif
    }

  private:
    QTemporaryDir m_settingsDirectory;
};

QTEST_MAIN(CommunityCloudOcrWidgetTests)
#include "community_cloud_ocr_widget_tests.moc"
