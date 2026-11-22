// SPDX-License-Identifier: GPL-3.0-or-later
#include "snow_shot/app/editionfeatures.h"
#include "snow_shot/storage/floatingtoolbarsettings.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QtTest/QtTest>

class CommunityToolbarPolicyTests final : public QObject {
    Q_OBJECT

  private slots:
    void nativeFloatingToolbarMatchesReviewedCommunityBaseline() {
        // Read the immutable original-author fixture as data; call the actual
        // compiled Community policy and production default-layout function.
        QFile source(QString::fromUtf8(SNOW_COMMUNITY_SCHEMA_BASELINE_JSON));
        QVERIFY2(source.open(QIODevice::ReadOnly), "Frozen Full schema baseline must be readable");
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(source.readAll(), &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(document.isObject());

        const QSet<QString> disabled{
            QStringLiteral("barcode-recognition"),
            QStringLiteral("table-recognition"),
            QStringLiteral("latex-recognition"),
            QStringLiteral("document-recognition"),
            QStringLiteral("convert-to-html")};
        for (const QString& id : disabled)
            QVERIFY2(!snow_shot::app::editionActionToolAvailable(id),
                     qPrintable(QStringLiteral("Disabled Community action was enabled: ") + id));
        for (const QString& id :
             {QStringLiteral("screenshot"), QStringLiteral("convert-to-markdown"),
              QStringLiteral("text-recognition"), QStringLiteral("text-translation")}) {
            QVERIFY2(snow_shot::app::editionActionToolAvailable(id),
                     qPrintable(QStringLiteral("Required Community action was disabled: ") + id));
        }

        QJsonObject original;
        bool found = false;
        for (const auto& value : document.object().value(QStringLiteral("entries")).toArray()) {
            const QJsonObject entry = value.toObject();
            if (entry.value(QStringLiteral("key")).toString() ==
                QStringLiteral("floating_toolbar/layout")) {
                QVERIFY2(!found, "Original baseline may contain exactly one floating layout");
                original = entry.value(QStringLiteral("defaultValue")).toObject();
                found = true;
            }
        }
        QVERIFY2(found, "Frozen original baseline must contain the floating toolbar");
        QVERIFY(original.value(QStringLiteral("positions")).isArray());
        QVERIFY(original.value(QStringLiteral("hidden")).isArray());

        QVector<QStringList> expectedPositions;
        QStringList expectedHidden;
        QSet<QString> removed;

        const auto retainEnabled = [&disabled, &removed](const QJsonArray& values) {
            QStringList retained;
            for (const QJsonValue& item : values) {
                const QString id = item.toString();
                if (disabled.contains(id))
                    removed.insert(id);
                else
                    retained.append(id);
            }
            return retained;
        };
        for (const QJsonValue& group : original.value(QStringLiteral("positions")).toArray()) {
            QVERIFY(group.isArray());
            const QStringList retained = retainEnabled(group.toArray());
            if (!retained.isEmpty())
                expectedPositions.append(retained);
        }
        expectedHidden = retainEnabled(original.value(QStringLiteral("hidden")).toArray());

        QVERIFY(removed == QSet<QString>({QStringLiteral("latex-recognition"),
                                          QStringLiteral("barcode-recognition"),
                                          QStringLiteral("table-recognition"),
                                          QStringLiteral("convert-to-html")}));
        const auto live = snow_shot::storage::defaultFloatingToolbarLayout();
        QVERIFY2(live.positions == expectedPositions,
                 "Live production floating toolbar does not match the reviewed Community exclusions");
        QCOMPARE(live.hidden, expectedHidden);
    }
};

QTEST_GUILESS_MAIN(CommunityToolbarPolicyTests)
#include "community_toolbar_policy_tests.moc"
