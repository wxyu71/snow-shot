#include "snow_shot/presentation/screenshotimageconversionview.h"
#include "snow_shot/presentation/screenshotclipboardservice.h"
#include "snow_shot/presentation/screenshotimageconversionclipboard.h"
#include "snow_shot/presentation/screenshotdocumentformatting.h"

#include "theme/theme_manager.h"
#include "antd_icons.h"
#include "widgets/button.h"
#include "widgets/context_menu.h"
#include "widgets/scroll_area.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QTextCursor>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextTable>
#include <QTextBlock>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <memory>

ScreenshotImageConversionView::ScreenshotImageConversionView(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("screenshotImageConversionView"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_browser = new QTextBrowser(this);
    m_browser->setObjectName(QStringLiteral("screenshotImageConversionPreview"));
    m_browser->setDocument(new snow_shot::presentation::ScreenshotInertDocument(m_browser));
    m_browser->setFrameStyle(QFrame::NoFrame);
    m_browser->setOpenLinks(false);
    m_browser->setOpenExternalLinks(false);
    m_browser->setVerticalScrollBar(new adqt::widgets::AdScrollBar(Qt::Vertical, m_browser));
    m_browser->setHorizontalScrollBar(new adqt::widgets::AdScrollBar(Qt::Horizontal, m_browser));
    m_browser->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_browser->setLineWrapMode(QTextEdit::WidgetWidth);
    m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_browser, 1);
    m_rawEditor = new QPlainTextEdit(this);
    m_rawEditor->setObjectName(QStringLiteral("screenshotMarkdownRawEditor"));
    m_rawEditor->setVisible(false);
    m_rawEditor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    layout->addWidget(m_rawEditor, 1);
    auto* editRow = new QHBoxLayout;
    editRow->setContentsMargins(12, 4, 12, 4);
    editRow->addStretch();
    m_editButton = new adqt::widgets::AdButton(this);
    m_editButton->setObjectName(QStringLiteral("screenshotMarkdownEditToggle"));
    m_editButton->setButtonStyle(adqt::widgets::AdButton::ButtonStyle::Text);
    m_editButton->setSizeClass(adqt::widgets::AdButton::SizeClass::Small);
    editRow->addWidget(m_editButton);
    layout->addLayout(editRow);
    connect(m_rawEditor, &QPlainTextEdit::textChanged, this, [this] {
        if (!m_editMode)
            return;
        m_source = m_rawEditor->toPlainText();
        emit sourceEdited(m_source);
    });
    connect(m_editButton, &adqt::widgets::AdButton::clicked, this, [this] {
        m_editMode = !m_editMode;
        m_browser->setVisible(!m_editMode);
        m_rawEditor->setVisible(m_editMode);
        if (m_editMode) {
            m_rawEditor->setFocus(Qt::OtherFocusReason);
        } else {
            render();
            m_browser->setFocus(Qt::OtherFocusReason);
        }
        retranslate();
    });
    setFocusProxy(m_browser);
    connect(m_browser, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        if (url.isValid() &&
            (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"))) {
            emit linkActivated(url);
        }
    });
    connect(m_browser, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        showContextMenu(m_browser->viewport()->mapToGlobal(position));
    });
    m_status = new QWidget(this);
    auto* statusLayout = new QHBoxLayout(m_status);
    statusLayout->setContentsMargins(12, 6, 12, 6);
    m_statusText = new QLabel(m_status);
    m_statusText->setObjectName(QStringLiteral("screenshotImageConversionStatus"));
    m_statusText->setWordWrap(true);
    m_statusText->setTextFormat(Qt::PlainText);
    m_retry = new adqt::widgets::AdButton(m_status);
    m_retry->setObjectName(QStringLiteral("screenshotImageConversionRetry"));
    m_retry->setButtonStyle(adqt::widgets::AdButton::ButtonStyle::Text);
    m_retry->setSizeClass(adqt::widgets::AdButton::SizeClass::Small);
    connect(m_retry, &adqt::widgets::AdButton::clicked, this,
            &ScreenshotImageConversionView::retryRequested);
    statusLayout->addWidget(m_statusText, 1);
    statusLayout->addWidget(m_retry);
    layout->addWidget(m_status);
    connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
            [this]() { render(); });
    retranslate();
    render();
}

void ScreenshotImageConversionView::setContent(SnowShotImageConversionFormat format,
                                               const QString& source, bool busy,
                                               const QString& error) {
    const bool changed = m_format != format || m_lastProvidedSource != source;
    m_format = format;
    if (changed) {
        m_lastProvidedSource = source;
        m_source = source;
        const QSignalBlocker blocker(m_rawEditor);
        m_rawEditor->setPlainText(source);
    }
    if (format != SnowShotImageConversionFormat::Markdown && m_editMode) {
        m_editMode = false;
        m_rawEditor->hide();
        m_browser->show();
    }
    m_busy = busy;
    m_error = error;
    if (changed && !m_editMode)
        render();
    retranslate();
}

void ScreenshotImageConversionView::retranslate() {
    m_browser->setAccessibleName(m_format == SnowShotImageConversionFormat::Markdown
                                     ? tr("Markdown preview")
                                     : tr("HTML preview"));
    m_retry->setText(tr("Retry"));
    m_editButton->setVisible(m_format == SnowShotImageConversionFormat::Markdown && !m_busy);
    m_editButton->setText(m_editMode ? tr("Preview Markdown") : tr("Edit Markdown"));
    m_status->setVisible(!m_busy && !m_error.isEmpty());
    m_retry->setVisible(!m_busy && !m_error.isEmpty());
    m_statusText->setText(m_source.isEmpty() ? m_error : tr("Incomplete result: %1").arg(m_error));
}

void ScreenshotImageConversionView::render() {
    if (m_browser == nullptr) {
        return;
    }
    const auto theme = adqt::theme::ThemeManager::instance().resolveTheme(m_browser);
    QPalette colors = m_browser->palette();
    colors.setColor(QPalette::Base, theme.colorBgContainer);
    colors.setColor(QPalette::Text, theme.colorText);
    colors.setColor(QPalette::Link, theme.colorLink);
    m_browser->setPalette(colors);
    QTextDocument* document = m_browser->document();
    // A default theme.appFont is an unresolved placeholder; deriving from the widget keeps
    // the preview on the application's unhinted outlines like every other themed surface.
    QFont font = theme.appFont.family().isEmpty() ? m_browser->font() : theme.appFont;
    font.setPixelSize(qRound(theme.fontSize));
    document->setDefaultFont(font);
    document->setDocumentMargin(12);
    const QString mono = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    // Theme fills carry alpha. Rich-text CSS needs the composited opaque color,
    // otherwise a translucent white dark-theme fill turns into solid white.
    const QColor fill = theme.colorFillTertiary;
    const QColor base = theme.colorBgContainer;
    const auto blended = [fill](int foreground, int background) {
        return qRound(static_cast<double>(fill.alphaF()) * foreground +
                      (1.0 - static_cast<double>(fill.alphaF())) * background);
    };
    const QColor codeBackground(blended(fill.red(), base.red()),
                                blended(fill.green(), base.green()),
                                blended(fill.blue(), base.blue()));
    const snow_shot::presentation::ScreenshotDocumentStyle style{
        theme.colorText, theme.colorLink,          mono,
        codeBackground,  theme.colorTextSecondary, theme.colorBorder};
    snow_shot::presentation::setScreenshotDocumentStyle(*document, style);
    const QTextCursor previous = m_browser->textCursor();
    const int previousAnchor = previous.anchor();
    const int previousPosition = previous.position();
    const int scroll = m_browser->verticalScrollBar()->value();
    const int horizontal = m_browser->horizontalScrollBar()->value();
    const bool follow =
        scroll >= m_browser->verticalScrollBar()->maximum() - 2 && !previous.hasSelection();
    snow_shot::presentation::setScreenshotDocumentSource(
        *document, m_source, m_format == SnowShotImageConversionFormat::Markdown, style);
    const int end = std::max(0, document->characterCount() - 1);
    QTextCursor restored(document);
    restored.setPosition(std::min(previousAnchor, end));
    restored.setPosition(std::min(previousPosition, end), QTextCursor::KeepAnchor);
    m_browser->setTextCursor(restored);
    m_browser->verticalScrollBar()->setValue(follow ? m_browser->verticalScrollBar()->maximum()
                                                    : scroll);
    m_browser->horizontalScrollBar()->setValue(horizontal);
}

bool ScreenshotImageConversionView::hasTextSelection() const {
    return m_browser->textCursor().hasSelection();
}

std::unique_ptr<QMimeData> ScreenshotImageConversionView::clipboardMimeData() const {
    return snow_shot::presentation::imageConversionClipboardMimeData(m_format, m_source,
                                                                     m_browser->textCursor());
}

bool ScreenshotImageConversionView::copyToClipboard() const {
    if (m_editMode) {
        const QTextCursor cursor = m_rawEditor->textCursor();
        QString selected = cursor.selectedText();
        selected.replace(QChar::ParagraphSeparator, u'\n');
        QApplication::clipboard()->setText(
            cursor.hasSelection() ? selected : m_source, QClipboard::Clipboard);
        return true;
    }
    if (const auto request = m_requestCopy)
        return request();
    auto mimeData = clipboardMimeData();
    if (mimeData == nullptr) {
        return false;
    }
    static_cast<void>(ScreenshotClipboardService::reservePublication());
    QApplication::clipboard()->setMimeData(mimeData.release(), QClipboard::Clipboard);
    return true;
}

void ScreenshotImageConversionView::selectAll() {
    if (m_editMode)
        m_rawEditor->selectAll();
    else
        m_browser->selectAll();
}

void ScreenshotImageConversionView::showContextMenu(const QPoint& globalPosition) {
    adqt::widgets::AdContextMenu menu(this);
    menu.setObjectName(QStringLiteral("screenshotImageConversionContextMenu"));
    QAction* copy = menu.addItem(tr("Copy"), adqt::icons::antd::outlined::Copy());
    QAction* selectAll = menu.addItem(tr("Select all"), adqt::icons::antd::outlined::Select());
    copy->setEnabled(!m_source.isEmpty());
    selectAll->setEnabled(!m_browser->document()->isEmpty());
    connect(copy, &QAction::triggered, this, [this]() { static_cast<void>(copyToClipboard()); });
    connect(selectAll, &QAction::triggered, this, &ScreenshotImageConversionView::selectAll);
    menu.execAt(globalPosition);
}

void ScreenshotImageConversionView::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
}
