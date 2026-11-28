// SPDX-License-Identifier: GPL-3.0-or-later
#include "snow_shot/network/webdavsettingssync.h"

#include <QtTest>

using namespace snow_shot::network::webdav;
using namespace Qt::StringLiterals;

class WebDavSettingsSyncTests : public QObject {
    Q_OBJECT
  private slots:
    void onlySafePreferences() {
        const QJsonObject local{{u"interface/theme_mode"_s, u"dark"_s},
                                {u"api_configuration/custom_models"_s, u"secret"_s},
                                {u"cloud_upload/configuration"_s, u"secret"_s},
                                {u"screenshot/image_save_directory"_s, u"C:/private"_s},
                                {u"screen_recording/enable_microphone"_s, true}};
        const auto projected = projectSettings(local);
        QCOMPARE(projected.size(), 1);
        QVERIFY(!encodeSettings(local));
        const auto encoded = encodeSettings(projected);
        QVERIFY(encoded);
        const auto decoded = decodeSettings(*encoded);
        QVERIFY(decoded);
        QCOMPARE(*decoded, projected);
        QVERIFY(!decodeSettings(R"({"format":1,"settings":{"interface/app_font_size":100}})"));
        QVERIFY(!decodeSettings(R"({"format":2,"settings":{}})"));
        QVERIFY(!decodeSettings(QByteArray(maxDocumentBytes + 1, 'x')));
    }

    void mergesIndependentChangesAndRejectsConflicts() {
        const QJsonObject base{{u"interface/theme_mode"_s, u"system"_s},
                               {u"interface/app_font_size"_s, 14}};
        auto local = base;
        auto remote = base;
        local.insert(u"interface/theme_mode"_s, u"dark"_s);
        remote.insert(u"interface/app_font_size"_s, 18);
        const auto merged = mergeSettings(base, local, remote);
        QVERIFY(merged.valid);
        QVERIFY(merged.conflicts.isEmpty());
        QCOMPARE(merged.values.value(u"interface/theme_mode"_s).toString(), u"dark"_s);
        QCOMPARE(merged.values.value(u"interface/app_font_size"_s).toInt(), 18);
        remote.insert(u"interface/theme_mode"_s, u"light"_s);
        QCOMPARE(mergeSettings(base, local, remote).conflicts,
                 QStringList{u"interface/theme_mode"_s});
        QVERIFY(!mergeSettings({}, local, remote).conflicts.isEmpty());
        QCOMPARE(mergeSettings(base, {}, {}).values, base);
    }

    void requiresHttpsAndConditionalWrites() {
        const QUrl url(u"https://dav.example.invalid/user/settings.json"_s);
        const auto read = requestFor(url, u"user"_s, u"token"_s);
        QVERIFY(read);
        QCOMPARE(read->attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),
                 int(QNetworkRequest::ManualRedirectPolicy));
        QVERIFY(!requestFor(QUrl(u"http://dav.example.invalid/settings.json"_s), u"u"_s, u"p"_s));
        QVERIFY(!requestFor(QUrl(u"https://u:p@dav.example.invalid/settings.json"_s), u"u"_s, u"p"_s));
        QVERIFY(!requestFor(QUrl(u"https://dav.example.invalid/settings.json?token=secret"_s),
                            u"u"_s, u"p"_s));
        QVERIFY(!strongEtag("W/\"weak\""));
        QVERIFY(!strongEtag("\"bad\r\nheader\""));
        QVERIFY(!writeRequest(url, u"u"_s, u"p"_s, {}, false));
        const auto create = writeRequest(url, u"u"_s, u"p"_s, {}, true);
        QVERIFY(create);
        QCOMPARE(create->rawHeader("If-None-Match"), QByteArray("*"));
        const auto update = writeRequest(url, u"u"_s, u"p"_s, "\"revision\"", false);
        QVERIFY(update);
        QCOMPARE(update->rawHeader("If-Match"), QByteArray("\"revision\""));
    }
};
QTEST_GUILESS_MAIN(WebDavSettingsSyncTests)
#include "webdav_settings_sync_tests.moc"
