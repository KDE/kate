/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateoutputview.h"
#include "kateapp.h"
#include "katemainwindow.h"

#include <KColorScheme>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/Editor>

#include <QClipboard>
#include <QDateTime>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QTimeLine>
#include <QToolButton>
#include <QVBoxLayout>

#include <ktexteditor_utils.h>

class BlockData : public QTextBlockUserData
{
public:
    BlockData(const QString &t)
        : token(t)
    {
    }
    const QString token;
};

class NewMsgIndicator : public QWidget
{
    Q_OBJECT
public:
    NewMsgIndicator(QWidget *parent)
        : QWidget(parent)
        , m_timeline(1000, this)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setGeometry(parent->geometry().adjusted(-2, -2, 2, 2));

        m_timeline.setDirection(QTimeLine::Forward);
        m_timeline.setEasingCurve(QEasingCurve::SineCurve);
        m_timeline.setFrameRange(20, 150);
        auto update = QOverload<>::of(&QWidget::update);
        connect(&m_timeline, &QTimeLine::valueChanged, this, update);
        connect(&m_timeline, &QTimeLine::finished, this, &QObject::deleteLater);
    }

    void run(int loopCount, QColor c)
    {
        // If parent is not visible, do nothing
        if (parentWidget() && !parentWidget()->isVisible()) {
            return;
        }
        m_flashColor = c;
        show();
        raise();
        m_timeline.setLoopCount(loopCount);
        m_timeline.start();
    }

    Q_SLOT void stop()
    {
        m_timeline.stop();
        deleteLater();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (m_timeline.state() == QTimeLine::Running) {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            m_flashColor.setAlpha(m_timeline.currentFrame());
            p.setBrush(m_flashColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect(), 15, 15);
        }
    }

private:
    QColor m_flashColor;
    QTimeLine m_timeline;
};

class KateOutputEdit : public QTextBrowser
{
    Q_OBJECT
public:
    KateOutputEdit(QWidget *parent)
        : QTextBrowser(parent)
    {
        setOpenExternalLinks(true);
        m_iconCache[QStringLiteral("dialog-scripts")] = QIcon::fromTheme(QStringLiteral("dialog-scripts")).pixmap(16, 16);
    }

    QVariant loadResource(int type, const QUrl &name) override
    {
        if (type == QTextDocument::ImageResource) {
            const QPixmap icon = m_iconCache[name.toString()];
            if (!icon.isNull()) {
                return icon;
            }
        }
        return QTextBrowser::loadResource(type, name);
    }

    void addIcon(const QString &cat, const QIcon &icon)
    {
        m_iconCache[cat] = icon.pixmap(16, 16);
    }

private:
    QHash<QString, QPixmap> m_iconCache;
};

KateOutputView::KateOutputView(KateMainWindow *mainWindow, QWidget *parent, QWidget *tabButton)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_textEdit(new KateOutputEdit(this))
    , tabButton(tabButton)
{
    Q_ASSERT(tabButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchTimer.setInterval(400);
    m_searchTimer.setSingleShot(true);
    m_searchTimer.callOnTimeout(this, &KateOutputView::search);

    // filter line edit
    m_filterLine.setPlaceholderText(i18n("Search..."));
    connect(&m_filterLine, &QLineEdit::textChanged, this, [this]() {
        m_searchTimer.start();
    });

    // copy button
    auto copy = new QToolButton(this);
    connect(copy, &QToolButton::clicked, this, [this] {
        const QString text = m_textEdit->toPlainText();
        if (!text.isEmpty()) {
            qApp->clipboard()->setText(text);
        }
    });
    copy->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    copy->setToolButtonStyle(Qt::ToolButtonIconOnly);
    copy->setToolTip(i18n("Copy all text to clipboard"));

    // clear button
    auto clear = new QToolButton(this);
    clear->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
    clear->setToolTip(i18n("Clear all messages"));
    connect(clear, &QPushButton::clicked, this, [this] {
        m_textEdit->clear();
    });

    // setup top horizontal layout
    // tried toolbar, has bad spacing
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(&m_filterLine);
    hLayout->addWidget(copy);
    hLayout->addWidget(clear);
    hLayout->setStretch(0, 1);

    // tree view
    layout->addLayout(hLayout);
    layout->addWidget(m_textEdit);

    // handle config changes & apply initial configuration
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateOutputView::readConfig);
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, &KateOutputView::readConfig);
    readConfig();
}

void KateOutputView::search()
{
    const QString text = m_filterLine.text();
    if (text.isEmpty()) {
        m_textEdit->setExtraSelections({});
        return;
    }

    const auto theme = KTextEditor::Editor::instance()->theme();
    QTextCharFormat f;
    f.setBackground(QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight)));

    QList<QTextEdit::ExtraSelection> sels;
    const auto *doc = m_textEdit->document();
    QTextCursor cursor = doc->find(text, 0);
    while (!cursor.isNull()) {
        QTextEdit::ExtraSelection s;
        s.cursor = cursor;
        s.format = f;
        sels.append(s);
        cursor = doc->find(text, cursor);
    }

    if (!sels.isEmpty()) {
        m_textEdit->setExtraSelections(sels);
        if (auto scroll = m_textEdit->verticalScrollBar()) {
            scroll->setValue(sels.constFirst().cursor.blockNumber());
        }
    }
}

void KateOutputView::readConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");
    m_showOutputViewForMessageType = cgGeneral.readEntry("Show output view for message type", 1);
    const int historyLimit = cgGeneral.readEntry("Output History Limit", 100);

    if (historyLimit != m_historyLimit) {
        m_historyLimit = historyLimit;
        m_textEdit->document()->setMaximumBlockCount(m_historyLimit);
    }

    // use editor fonts
    const auto theme = KTextEditor::Editor::instance()->theme();
    auto pal = m_textEdit->palette();
    pal.setColor(QPalette::Base, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor)));
    pal.setColor(QPalette::Highlight, QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection)));
    pal.setColor(QPalette::Text, QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal)));
    m_textEdit->setPalette(pal);
    m_textEdit->setFont(Utils::editorFont());

    auto brighten = [](QColor &c) {
        c = c.toHsv();
        c.setHsv(c.hue(), qMin(c.saturation() + 35, 255), qMin(c.value() + 10, 255));
    };

    KColorScheme c;
    m_msgIndicatorColors[0] = c.background(KColorScheme::NegativeBackground).color();
    brighten(m_msgIndicatorColors[0]);
    m_msgIndicatorColors[1] = c.background(KColorScheme::NeutralBackground).color();
    brighten(m_msgIndicatorColors[1]);
    m_msgIndicatorColors[2] = c.background(KColorScheme::PositiveBackground).color();
    brighten(m_msgIndicatorColors[2]);
}

static void wrapLinksWithHref(QString &text)
{
    static const QRegularExpression re(
        QStringLiteral(R"re((https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)))re"));
    text.replace(re, QStringLiteral("<a href=\"\\1\" >\\1</a>"));
}

void KateOutputView::slotMessage(const QVariantMap &message)
{
    /**
     * discard all messages without any real text
     */
    auto text = message.value(QStringLiteral("text")).toString().trimmed().replace(QStringLiteral("\n"), QStringLiteral("<br>"));
    if (text.isEmpty()) {
        return;
    }

    /*
     * subsequent message might replace a former one (e.g. for progress)
     */
    const auto token = message.value(QStringLiteral("token")).toString();

    QString t = QStringLiteral("[");

    /**
     * date time column: we want to know when a message arrived
     * TODO: perhaps store full date time for more stuff later
     */
    const QDateTime current = QDateTime::currentDateTime();
    t += current.time().toString(Qt::TextDate);

    /**
     * category
     * provided by sender to better categorize the output into stuff like: lsp, git, ...
     * optional icon support
     */
    const QString category = message.value(QStringLiteral("category")).toString().trimmed();
    const auto categoryIcon = message.value(QStringLiteral("categoryIcon")).value<QIcon>();
    if (categoryIcon.isNull()) {
        t += QStringLiteral(" <img src=\"") + QStringLiteral("dialog-scripts") + QStringLiteral("\"/> ");
    } else {
        m_textEdit->addIcon(category, categoryIcon);
        t += QStringLiteral(" <img src=\"") + category + QStringLiteral("\"/> ");
    }

    t += QStringLiteral("<u>") + category + QStringLiteral("</u> ");

    /**
     * type column: shows the type, icons for some types only
     */
    bool shouldShowOutputToolView = false;
    int indicatorLoopCount = 0; // for warning/error infinite loop
    QColor color;
    const auto typeString = message.value(QStringLiteral("type")).toString();
    if (typeString == QLatin1String("Error")) {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 1);
        t += QStringLiteral("<span style=\"color:red\">") + i18nc("@info", "Error") + QStringLiteral("</span>");
        color = m_msgIndicatorColors[0];
    } else if (typeString == QLatin1String("Warning")) {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 2);
        t += QStringLiteral("<span style=\"color:#FFA500\">") + i18nc("@info", "Warning") + QStringLiteral("</span>");
        color = m_msgIndicatorColors[1];
    } else if (typeString == QLatin1String("Info")) {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 3);
        const bool dark = m_textEdit->palette().base().color().lightness() < 127;
        const QString blue = dark ? QStringLiteral("#0096FF") : QStringLiteral("#4169E1");
        t += QStringLiteral("<span style=\"color:%1\">").arg(blue) + i18nc("@info", "Info") + QStringLiteral("</span>");
        indicatorLoopCount = 2;
        color = m_msgIndicatorColors[2];
    } else {
        shouldShowOutputToolView = (m_showOutputViewForMessageType >= 4);
        t += i18nc("@info", "Log");
        indicatorLoopCount = -1; // no FadingIndicator for log messages
    }

    t += QStringLiteral("] ");

    if (shouldShowOutputToolView || isVisible()) {
        // if we are going to show the output toolview afterwards
        indicatorLoopCount = 1;
    }

    if (!m_fadingIndicator && indicatorLoopCount >= 0) {
        m_fadingIndicator = new NewMsgIndicator(tabButton);
        m_fadingIndicator->run(indicatorLoopCount, color);
        connect(tabButton, SIGNAL(clicked()), m_fadingIndicator, SLOT(stop()));
    }

    /**
     * actual message text
     */
    wrapLinksWithHref(text);
    t += text;

    if (!token.isEmpty()) {
        const auto doc = m_textEdit->document();
        auto block = doc->lastBlock();
        bool found = false;
        while (block.isValid()) {
            auto data = dynamic_cast<BlockData *>(block.userData());
            if (data && data->token == token) {
                found = true;
                break;
            }
            block = block.previous();
        }

        if (!found) {
            m_textEdit->append(t);
            auto data = new BlockData(token);
            m_textEdit->document()->lastBlock().setUserData(data);
        } else if (block.isValid()) {
            QTextCursor c(block);

            c.select(QTextCursor::BlockUnderCursor);
            c.removeSelectedText();
            c.insertBlock();
            c.insertHtml(t);
            c.block().setUserData(new BlockData(token));
        } else {
            qWarning() << Q_FUNC_INFO << "unable to find valid block!";
            m_textEdit->append(t);
            auto data = new BlockData(token);
            m_textEdit->document()->lastBlock().setUserData(data);
        }
    } else {
        m_textEdit->append(t);
    }

    /**
     * ensure last item is visible
     */
    if (auto scroll = m_textEdit->verticalScrollBar()) {
        scroll->setValue(scroll->maximum());
    }

    /**
     * if message requires it => show the tool view if hidden
     */
    if (shouldShowOutputToolView) {
        m_mainWindow->showToolView(parentWidget());
    }
}

#include "kateoutputview.moc"
