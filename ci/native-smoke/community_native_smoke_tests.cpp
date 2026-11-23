#include "snow_shot/network/communitycloudocrsettings.h"
#include "snow_shot/presentation/components/cloudocrenginechoices.h"

#include <QComboBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class CommunityNativeSmoke final : public QObject {
    Q_OBJECT

  private slots:
    void productionEngineIsPaddleOnly() {
        QComboBox engines;
        snow_shot::presentation::components::populateCloudOcrEngineChoices(engines, true);
        QCOMPARE(engines.count(), 1);
        QCOMPARE(engines.itemData(0, Qt::UserRole).toString(), QStringLiteral("paddleocr"));
        QCOMPARE(engines.findData(QStringLiteral("vision")), -1);
    }

    void realCloudSettingsNormalizeAndPersist() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temp.path());
        {
            const QString organization = QStringLiteral("SnowShot");
            const QString application = QStringLiteral("CloudOcr");
            QSettings store(QSettings::IniFormat, QSettings::UserScope, organization, application);
            store.clear();
            store.sync();
        }
        const auto initial = snow_shot::network::readCommunityCloudOcrSettings();
        QCOMPARE(initial.engine, QStringLiteral("paddleocr"));
        QCOMPARE(initial.outputFormat, QStringLiteral("markdown"));

        auto selected = initial;
        selected.engine = QStringLiteral("vision");
        selected.outputFormat = QStringLiteral("html");
        selected.accessToken = QStringLiteral("native-test-token");
        QVERIFY(snow_shot::network::writeCommunityCloudOcrSettings(selected));

        const auto restored = snow_shot::network::readCommunityCloudOcrSettings();
        QCOMPARE(restored.engine, QStringLiteral("paddleocr"));
        QCOMPARE(restored.outputFormat, QStringLiteral("markdown"));
        QCOMPARE(restored.accessToken, QStringLiteral("native-test-token"));

        const QString organization = QStringLiteral("SnowShot");
        const QString application = QStringLiteral("CloudOcr");
        QSettings legacy(QSettings::IniFormat, QSettings::UserScope, organization, application);
        const QString storedEngine = legacy.value(QStringLiteral("engine")).toString();
        const QString storedOutput = legacy.value(QStringLiteral("output_format")).toString();
        QCOMPARE(storedEngine, QStringLiteral("paddleocr"));
        QCOMPARE(storedOutput, QStringLiteral("markdown"));
        // Existing installs can have obsolete Vision preferences. Even before
        // saving, the reader must not route a future OCR job to that provider.
        legacy.setValue(QStringLiteral("engine"), QStringLiteral("vision"));
        legacy.setValue(QStringLiteral("output_format"), QStringLiteral("latex"));
        legacy.sync();
        const auto migrated = snow_shot::network::readCommunityCloudOcrSettings();
        QCOMPARE(migrated.engine, QStringLiteral("paddleocr"));
        QCOMPARE(migrated.outputFormat, QStringLiteral("markdown"));
    }
};

QTEST_MAIN(CommunityNativeSmoke)
#include "community_native_smoke_tests.moc"
