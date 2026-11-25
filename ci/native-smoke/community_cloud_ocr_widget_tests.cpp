#include "snow_shot/network/communitycloudocrsettings.h"
#include "snow_shot/presentation/components/cloudocrsettingswidget.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTranslator>
#include <QtTest/QtTest>

namespace {
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

    void productionWidgetExposesOnlyPaddleAndMarkdown() {
        CloudOcrSettingsWidget widget;
        auto* engine = widget.findChild<QComboBox*>(QStringLiteral("onlineOcrEngine"));
        auto* output = widget.findChild<QComboBox*>(QStringLiteral("onlineOcrOutputFormat"));
        auto* endpoint = widget.findChild<QLineEdit*>(QStringLiteral("paddleOcrApiUrl"));
        auto* token = widget.findChild<QLineEdit*>(QStringLiteral("paddleOcrAccessToken"));
        auto* model = widget.findChild<QComboBox*>(QStringLiteral("paddleOcrModel"));
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
        QVERIFY(!widget.findChild<QComboBox*>(QStringLiteral("onlineOcrVisionModel")));
        QCOMPARE(engine->count(), 1);
        QCOMPARE(engine->currentData().toString(), QStringLiteral("paddleocr"));
        QCOMPARE(engine->findData(QStringLiteral("vision")), -1);
        QVERIFY(!engine->isEnabled());
        QCOMPARE(output->count(), 1);
        QCOMPARE(output->currentData().toString(), QStringLiteral("markdown"));
        QCOMPARE(output->findData(QStringLiteral("html")), -1);
        QCOMPARE(output->findData(QStringLiteral("latex")), -1);
        QVERIFY(!output->isEnabled());
        QCOMPARE(token->echoMode(), QLineEdit::Password);
        QCOMPARE(model->count(), 3);
        QVERIFY(model->findText(QStringLiteral("PaddleOCR-VL-1.6")) >= 0);
        QVERIFY(model->findText(QStringLiteral("PP-StructureV3")) >= 0);
        QVERIFY(model->findText(QStringLiteral("PP-OCRv6")) >= 0);
    }

    void widgetUsesItsOwnTranslationContextAndKeepsModelValues() {
        CloudOcrContextTranslator translator;
        QVERIFY(QCoreApplication::installTranslator(&translator));
        CloudOcrSettingsWidget widget;
        auto* engine = widget.findChild<QComboBox*>(QStringLiteral("onlineOcrEngine"));
        auto* output = widget.findChild<QComboBox*>(QStringLiteral("onlineOcrOutputFormat"));
        auto* model = widget.findChild<QComboBox*>(QStringLiteral("paddleOcrModel"));
        auto* save = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrSave"));
        auto* test = widget.findChild<QPushButton*>(QStringLiteral("paddleOcrTestConnection"));
        QVERIFY(engine);
        QVERIFY(output);
        QVERIFY(model);
        QVERIFY(save);
        QVERIFY(test);
        QCOMPARE(engine->currentText(), QStringLiteral("translated: Baidu PaddleOCR (AI Studio)"));
        QCOMPARE(output->currentText(),
                 QStringLiteral("translated: Markdown (formulas preserved as LaTeX)"));
        QCOMPARE(save->text(), QStringLiteral("translated: Save"));
        QCOMPARE(test->text(), QStringLiteral("translated: Test PaddleOCR connection"));
        QCOMPARE(engine->currentData().toString(), QStringLiteral("paddleocr"));
        QCOMPARE(output->currentData().toString(), QStringLiteral("markdown"));
        QCOMPARE(model->currentText(), QStringLiteral("PaddleOCR-VL-1.6"));
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
        auto* model = widget.findChild<QComboBox*>(QStringLiteral("paddleOcrModel"));
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
        model->setCurrentText(QStringLiteral("PP-OCRv6"));
        QTest::mouseClick(save, Qt::LeftButton);
        QCOMPARE(status->text(), QStringLiteral("PaddleOCR configuration saved."));

        const auto stored = snow_shot::network::readCommunityCloudOcrSettings();
        QCOMPARE(stored.apiUrl, QStringLiteral("http://127.0.0.1:5102"));
        QCOMPARE(stored.accessToken, QStringLiteral("test-token"));
        QCOMPARE(stored.model, QStringLiteral("PP-OCRv6"));
        QCOMPARE(stored.engine, QStringLiteral("paddleocr"));
        QCOMPARE(stored.outputFormat, QStringLiteral("markdown"));

        QSettings persisted(QSettings::IniFormat, QSettings::UserScope, organization, application);
        const QString persistedEngine = persisted.value(QStringLiteral("engine")).toString();
        const QString persistedOutput = persisted.value(QStringLiteral("output_format")).toString();
        QCOMPARE(persistedEngine, QStringLiteral("paddleocr"));
        QCOMPARE(persistedOutput, QStringLiteral("markdown"));
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
