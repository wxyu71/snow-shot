#include "snow_shot/storage/configurationschema.h"
#include "snow_shot/app/edition.h"
#include "snow_shot/shortcuts/shortcutbinding.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>

#include <cstdlib>
#include <iostream>
#include <utility>

namespace {
using snow_shot::storage::ConfigurationSchema;
using snow_shot::storage::ConfigurationSchemaEntry;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

QString outputDirectory(QStandardPaths::StandardLocation location) {
    const QString directory = QStandardPaths::writableLocation(location);
    return directory.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                               : directory;
}

QJsonArray metadata() {
    constexpr const char* kinds[] = {"Boolean",    "Integer",      "String",
                                     "StringList", "ShortcutList", "Structured"};
    QJsonArray result;
    for (const auto& entry : ConfigurationSchema::entries()) {
        QJsonArray allowed;
        for (const auto& item : entry.allowedStringValues)
            allowed.append(item);
        QJsonValue range;
        if (entry.integerRange)
            range = QJsonArray{entry.integerRange->minimum, entry.integerRange->maximum,
                               entry.integerRange->step};
        QJsonValue value = entry.defaultValue;
        // The reference captures all actual factory results. Only these two
        // environment-dependent values use markers; they are checked separately.
        if (entry.key == QStringLiteral("screen_recording/video_save_directory"))
            value = QStringLiteral("$movies-directory");
        else if (entry.key == QStringLiteral("screenshot/image_save_directory"))
            value = QStringLiteral("$pictures-directory");
        result.append(QJsonObject{
            {QStringLiteral("key"), entry.key},
            {QStringLiteral("defaultValue"), value},
            {QStringLiteral("defaultJsonType"), static_cast<int>(entry.defaultValue.type())},
            {QStringLiteral("defaultStringIsNull"),
             entry.defaultValue.isString() && entry.defaultValue.toString().isNull()},
            {QStringLiteral("valueKind"),
             QString::fromLatin1(kinds[static_cast<int>(entry.valueKind)])},
            {QStringLiteral("integerRange"), range},
            {QStringLiteral("allowedStringValues"), allowed},
            {QStringLiteral("maximumListItems"), entry.maximumListItems},
        });
    }
    return result;
}


#ifdef Q_OS_WIN
// The original author baseline is intentionally immutable. Only the five
// reviewed Community-disabled toolbar actions may disappear from three
// affected Full-edition layouts. Compare all remaining data exactly.
QJsonObject communityToolbarBaseline(QJsonObject entry) {
    if (snow_shot::app::edition::isMini)
        return entry;

    const QString key = entry.value(QStringLiteral("key")).toString();
    const QString floating = QStringLiteral("floating_toolbar/layout");
    const QString pinned = QStringLiteral("pin_to_screen/action_tools_layout");
    const QString screenshot = QStringLiteral("screenshot_toolbar/action_tools_layout");
    if (key != floating && key != pinned && key != screenshot)
        return entry;

    const QSet<QString> unavailable{
        QStringLiteral("barcode-recognition"), QStringLiteral("table-recognition"),
        QStringLiteral("latex-recognition"), QStringLiteral("document-recognition"),
        QStringLiteral("convert-to-html")};
    QSet<QString> expectedRemoved = unavailable;
    if (key == floating)
        expectedRemoved.remove(QStringLiteral("document-recognition"));

    QJsonObject layout = entry.value(QStringLiteral("defaultValue")).toObject();
    require(layout.value(QStringLiteral("positions")).isArray() &&
                layout.value(QStringLiteral("hidden")).isArray(),
            "frozen original toolbar layout must contain positions and hidden arrays");
    QSet<QString> removed;
    const auto withoutUnavailable = [&unavailable, &removed](const QJsonArray& original) {
        QJsonArray retained;
        for (const auto& item : original) {
            if (unavailable.contains(item.toString()))
                removed.insert(item.toString());
            else
                retained.append(item);
        }
        return retained;
    };
    QJsonArray positions;
    for (const auto& group : layout.value(QStringLiteral("positions")).toArray()) {
        require(group.isArray(), "frozen toolbar positions must be arrays");
        const QJsonArray retained = withoutUnavailable(group.toArray());
        if (!retained.isEmpty())
            positions.append(retained);
    }
    layout.insert(QStringLiteral("positions"), positions);
    layout.insert(QStringLiteral("hidden"),
                  withoutUnavailable(layout.value(QStringLiteral("hidden")).toArray()));
    require(removed == expectedRemoved,
            "frozen toolbar baseline must retain exactly the reviewed disabled Community actions");
    entry.insert(QStringLiteral("defaultValue"), layout);
    return entry;
}
#endif

void publicMetadataMatchesOriginal(const QJsonArray& actual) {
    // The Community safety policy makes startup registration opt-in on every
    // platform, including Windows. The original-author fixtures stay frozen.
    require(
        !ConfigurationSchema::defaultValue(QStringLiteral("system/auto_start_at_boot")).toBool(),
        "Community auto-start must default off in both editions on all platforms");
    require(ConfigurationSchema::defaultValue(QStringLiteral("system/launch_as_administrator")) == QJsonValue(false) &&
                ConfigurationSchema::defaultValue(QStringLiteral("mcp/enabled")) == QJsonValue(false),
            "Local administrator launch and MCP must require explicit opt-in");
    require(ConfigurationSchema::defaultValue(QStringLiteral("capture_history/retention_mode")) == QJsonValue(QStringLiteral("full")) &&
                ConfigurationSchema::defaultValue(QStringLiteral("pinned_history/enabled")) == QJsonValue(true),
            "Local screenshot and pinned histories default on; explicit user settings remain editable");

#ifdef Q_OS_WIN
    QFile file(QString::fromUtf8(SNOW_SCHEMA_BASELINE_JSON));
    require(file.open(QIODevice::ReadOnly), "the original schema metadata fixture must open");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    require(error.error == QJsonParseError::NoError && document.isObject(),
            "the original schema metadata fixture must be valid JSON");
    const auto fixture = document.object();
    require(fixture.value(QStringLiteral("originalSourceSha256")).toString() ==
                QStringLiteral("8ceb23d8125a6c950b33f3a486629fdcb5844939b2f1f1bdff5376ac77c1ca8f"),
            "the reference must come from the frozen original initializer");
    require(fixture.value(QStringLiteral("edition")).toString() ==
                (snow_shot::app::edition::isMini ? QStringLiteral("mini") : QStringLiteral("full")),
            "the reference must match the linked storage edition");
    const auto expected = fixture.value(QStringLiteral("entries")).toArray();
    // Both frozen Windows fixtures already contain the annotation cycle shortcut.
    // Validate its runtime contract independently and compare every baseline row.
    const QString communityKey =
        QStringLiteral("drawing_shortcuts/cycle_annotation_tool_options");
    bool foundCommunityKey = false;
    for (const auto& value : actual) {
        const auto entry = value.toObject();
        if (entry.value(QStringLiteral("key")).toString() != communityKey)
            continue;
        require(!foundCommunityKey, "Community shortcut schema key must be unique");
        foundCommunityKey = true;
        // The raw descriptor stores "Tab" as a StringList, but buildEntries()
        // materializes all drawing shortcuts as typed ShortcutList objects.
        // Validate the runtime contract rather than the intermediate descriptor.
        const QJsonValue storedDefault = entry.value(QStringLiteral("defaultValue"));
        const auto bindings = snow_shot::shortcuts::shortcutBindingsFromJson(storedDefault, false);
        const auto normalized = ConfigurationSchema::normalize(communityKey, storedDefault);
        require(
            entry.value(QStringLiteral("valueKind")).toString() == QStringLiteral("ShortcutList") &&
                bindings == snow_shot::shortcuts::ShortcutBindingList{QStringLiteral("Tab")} &&
                storedDefault == snow_shot::shortcuts::shortcutBindingsToJson(bindings) &&
                normalized.valid && !normalized.changed && normalized.value == storedDefault &&
                entry.value(QStringLiteral("maximumListItems")).toInt() == 2,
            "Community annotation cycle shortcut must retain its typed Tab default");
    }
    require(foundCommunityKey, "Community annotation cycle shortcut must be present");
    if (actual.size() != expected.size()) {
        std::cerr << "schema entries: actual=" << actual.size() << " expected=" << expected.size()
                  << '\n';
        const auto keys = [](const QJsonArray& entries) {
            QSet<QString> result;
            for (const auto& entry : entries)
                result.insert(entry.toObject().value(QStringLiteral("key")).toString());
            return result;
        };
        const auto actualKeys = keys(actual);
        const auto expectedKeys = keys(expected);
        for (const auto& key : actualKeys - expectedKeys)
            std::cerr << "missing fixture key: " << key.toStdString() << '\n';
        for (const auto& key : expectedKeys - actualKeys)
            std::cerr << "unexpected fixture key: " << key.toStdString() << '\n';
    }
    require(actual.size() == expected.size(),
            "original schema entry counts must match");
    int changedEntries = 0;
    for (qsizetype i = 0; i < expected.size(); ++i) {
        QJsonObject expectedEntry = communityToolbarBaseline(expected.at(i).toObject());
        const auto key = expectedEntry.value(QStringLiteral("key")).toString();
        // Keep the original-author fixture unchanged for regression provenance.
        // Preserve original enabled local histories. Review only Community's
        // opt-in startup, manual unsigned-release updates and toolbar filtering.
        if (key == QStringLiteral("system/auto_start_at_boot")) {
            require(expectedEntry.value(QStringLiteral("defaultValue")).toBool(),
                    "the upstream startup default must remain enabled");
            expectedEntry.insert(QStringLiteral("defaultValue"), false);
        }
        if (key == QStringLiteral("updates/mode")) {
            require(expectedEntry.value(QStringLiteral("defaultValue")) == QStringLiteral("next_launch"),
                    "the upstream update default must remain next_launch");
            expectedEntry.insert(QStringLiteral("defaultValue"), QStringLiteral("manual"));
        }
        if (actual.at(i).toObject() != expectedEntry) {
            ++changedEntries;
            // Show multiple independent schema mismatches in a single native CI
            // run instead of iterating through one failing fixture row per run.
            if (changedEntries <= 12) {
                std::cerr << "schema metadata changed at entry " << i << ": "
                          << key.toStdString() << "\nactual: "
                          << QJsonDocument(actual.at(i).toObject()).toJson().constData()
                          << "expected: " << QJsonDocument(expectedEntry).toJson().constData();
            }
        }
    }
    require(changedEntries == 0,
            "unreviewed original schema metadata differences remain (see all listed keys)");
#else
    // The frozen field-by-field references are native Windows Full/Mini runs.
    // Other hosts still exercise the common contracts and native defaults below.
    require(actual.size() == ConfigurationSchema::entries().size(),
            "every native entry must expose complete metadata");
#endif
}

void entryReferencesAndCopiesRemainIndependent() {
    const auto& entries = ConfigurationSchema::entries();
    require(!entries.isEmpty(), "the schema must contain entries");
    const auto* storage = entries.constData();
    QSet<QString> keys;
    bool checkedAllowedValues = false;
    bool checkedObject = false;
    bool checkedArray = false;
    for (const auto& entry : entries) {
        require(!entry.key.isEmpty() && !keys.contains(entry.key),
                "schema keys must remain nonempty and unique");
        keys.insert(entry.key);
        require(ConfigurationSchema::entry(entry.key) == &entry &&
                    ConfigurationSchema::contains(entry.key) &&
                    ConfigurationSchema::defaultValue(entry.key) == entry.defaultValue,
                "indexed lookups must return the canonical entry and default");
        ConfigurationSchemaEntry copy = entry;
        copy.key[0] = QChar(u'#');
        require(copy.key != entry.key && ConfigurationSchema::entry(entry.key) == &entry,
                "copied keys must detach from immutable schema text");
        if (!entry.allowedStringValues.isEmpty() && !entry.allowedStringValues.front().isEmpty()) {
            copy.allowedStringValues[0][0] =
                entry.allowedStringValues.front().front() == QChar(u'#') ? QChar(u'?')
                                                                         : QChar(u'#');
            require(copy.allowedStringValues != entry.allowedStringValues,
                    "copied allowed values must detach from immutable schema text");
            checkedAllowedValues = true;
        }
        if (entry.defaultValue.isObject()) {
            auto object = copy.defaultValue.toObject();
            object.insert(QStringLiteral("schema-test-copy"), true);
            copy.defaultValue = object;
            require(copy.defaultValue != entry.defaultValue,
                    "copied JSON object defaults must remain independent");
            checkedObject = true;
        } else if (entry.defaultValue.isArray()) {
            auto array = copy.defaultValue.toArray();
            array.append(QStringLiteral("schema-test-copy"));
            copy.defaultValue = array;
            require(copy.defaultValue != entry.defaultValue,
                    "copied JSON array defaults must remain independent");
            checkedArray = true;
        }
    }
    auto detachedEntries = entries;
    detachedEntries[0].key = QStringLiteral("schema-test-replacement");
    require(entries.constData() == storage && &ConfigurationSchema::entries() == &entries &&
                ConfigurationSchema::entry(entries.front().key) == &entries.front(),
            "lookups and detached vector copies must preserve canonical entry addresses");
    require(checkedAllowedValues && checkedObject && checkedArray,
            "copy independence must cover option strings and structured defaults");
}

void nativeDefaultsRemainIntact(const QString& movies, const QString& pictures) {
    require(ConfigurationSchema::defaultValue(
                QStringLiteral("screen_recording/video_save_directory")) == movies &&
                ConfigurationSchema::defaultValue(
                    QStringLiteral("screenshot/image_save_directory")) == pictures,
            "directory factories must preserve eager native defaults before application/test-mode "
            "changes");
    require(
        ConfigurationSchema::defaultValue(QStringLiteral("text_recognition/resident_process")) ==
                QJsonValue(!snow_shot::app::edition::isMini) &&
            ConfigurationSchema::defaultValue(
                QStringLiteral("pin_to_screen/automatic_text_recognition")) ==
                QJsonValue(!snow_shot::app::edition::isMini),
        "edition-specific default booleans must remain intact");
#ifdef Q_OS_WIN
    require(ConfigurationSchema::defaultValue(QStringLiteral("updates/mode")) == QStringLiteral("manual"),
            "Community Windows must not schedule unsigned automatic updates");
#endif
#ifdef Q_OS_MACOS
    require(ConfigurationSchema::defaultValue(QStringLiteral("updates/mode")) ==
                    QStringLiteral("next_launch") &&
                ConfigurationSchema::defaultValue(QStringLiteral(
                    "screenshot_ui/selection_display_unit")) == QStringLiteral("logical_pixels"),
            "native macOS update and coordinate defaults must remain intact");
    for (const auto& binding : {std::pair{"global_shortcuts/screenshot", 18u},
                                std::pair{"global_shortcuts/screenshot_copy", 19u},
                                std::pair{"global_shortcuts/pin_clipboard_content", 20u},
                                std::pair{"global_shortcuts/restore_last_closed_windows", 20u}}) {
        const auto values = snow_shot::shortcuts::shortcutBindingsFromJson(
            ConfigurationSchema::defaultValue(QString::fromLatin1(binding.first)), true);
        require(values.size() == 1 &&
                    values.front().physicalKeys.value(
                        snow_shot::shortcuts::ShortcutPlatform::MacOS) == binding.second,
                "macOS physical shortcut bindings must remain attached to their defaults");
    }
#endif
}
} // namespace

int main(int argc, char** argv) {
    const QString movies = outputDirectory(QStandardPaths::MoviesLocation);
    const QString pictures = outputDirectory(QStandardPaths::PicturesLocation);
    QCoreApplication application(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    const auto before = metadata();
    publicMetadataMatchesOriginal(before);
    nativeDefaultsRemainIntact(movies, pictures);
    entryReferencesAndCopiesRemainIndependent();
    require(metadata() == before, "copy mutation must not change any public schema field");
    return 0;
}
