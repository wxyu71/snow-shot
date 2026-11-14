#include "snow_shot/presentation/screenshotimageconversionclipboard.h"

#include <QCoreApplication>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QMimeData>
#include <QString>

static void require(bool ok, const char* message) {
    if (!ok)
        qFatal("Community Markdown clipboard contract: %s", message);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    constexpr auto format = SnowShotImageConversionFormat::Markdown;
    const QString source = QStringLiteral(
        "# 2026 Invoice\n\n| Item | Value |\n| --- | --- |\n| Book | 42 |\n\n$$E=mc^2$$\n");
    // Auto-copy uses the complete raw Markdown, preserving formulas, table pipes,
    // and whitespace. Selection copy is separate and must not overwrite source.
    auto full = snow_shot::presentation::imageConversionClipboardMimeData(format, source);
    require(full && full->text() == source && !full->hasHtml(),
            "full Markdown copy exactly preserves source without HTML MIME");

    QTextDocument doc;
    doc.setPlainText(source);
    QTextCursor cursor(&doc);
    const int formula = source.indexOf(QStringLiteral("$$E=mc^2$$"));
    require(formula >= 0, "formula fixture exists");
    cursor.setPosition(formula);
    cursor.setPosition(formula + QStringLiteral("$$E=mc^2$$").size(),
                       QTextCursor::KeepAnchor);
    auto partial = snow_shot::presentation::imageConversionClipboardMimeData(format, source, cursor);
    require(partial && partial->text() == QStringLiteral("$$E=mc^2$$") &&
            partial->text() != source && !partial->hasHtml(),
            "selected Markdown formula copies only the selected text");

    cursor.clearSelection();
    auto restored = snow_shot::presentation::imageConversionClipboardMimeData(format, source, cursor);
    require(restored && restored->text() == source,
            "removing the selection restores exact full-source copy");

    auto empty = snow_shot::presentation::imageConversionClipboardMimeData(format, {});
    require(!empty, "empty OCR results do not replace the clipboard");
    return 0;
}
