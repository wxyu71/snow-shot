#include "snow_shot/network/communitycloudocrsettings.h"
#include "snow_shot/presentation/components/cloudocrenginechoices.h"

#include <QComboBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class CommunityNativeSmoke final : public QObject {
    Q_OBJECT

  private slots:
    void engineIdsAreStoredInQtUserRole() {
        QComboBox engines;
        snow_shot::presentation::components::populateCloudOcrEngineChoices(engines, true);
        QCOMPARE(engines.count(), 2);
        QCOMPARE(engines.itemData(0, Qt::UserRole).toString(), QStringLiteral("paddleocr"));
        QCOMPARE(engines.itemData(1, Qt::UserRole).toString(), QStringLiteral("vision"));
        QCOMPARE(engines.findData(QStringLiteral("paddleocr")), 0);
        QCOMPARE(engines.findData(QStringLiteral("vision")), 1);
    }

    void paddleOnlyBuildOmitsVision() {
        QComboBox engines;
        snow_shot::presentation::components::populateCloudOcrEngineChoices(engines, false);
        QCOMPARE(engines.count(), 1);
        QCOMPARE(engines.currentData().toString(), QStringLiteral("paddleocr"));
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
        QCOMPARE(restored.engine, QStringLiteral("vision"));
        QCOMPARE(restored.outputFormat, QStringLiteral("html"));
        QCOMPARE(restored.accessToken, QStringLiteral("native-test-token"));

        selected.engine = QStringLiteral("paddleocr");
        selected.outputFormat = QStringLiteral("latex");
        QVERIFY(snow_shot::network::writeCommunityCloudOcrSettings(selected));
        QCOMPARE(snow_shot::network::readCommunityCloudOcrSettings().outputFormat,
                 QStringLiteral("markdown"));
    }
};

QTEST_MAIN(CommunityNativeSmoke)
#include "community_native_smoke_tests.moc"
