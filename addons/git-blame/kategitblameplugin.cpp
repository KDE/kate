/*
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kategitblameplugin.h"
#include "commitfilesview.h"

#include <gitprocess.h>

#include <algorithm>

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/InlineNoteInterface>
#include <KTextEditor/View>

#include <QDir>
#include <QUrl>

#include <QFontMetrics>
#include <QLayout>
#include <QPainter>
#include <QVariant>

static bool isUncomittedLine(const QByteArray &hash)
{
    return hash == "hash" || hash == "0000000000000000000000000000000000000000";
}

GitBlameInlineNoteProvider::GitBlameInlineNoteProvider(KateGitBlamePluginView *pluginView)
    : KTextEditor::InlineNoteProvider()
    , m_pluginView(pluginView)
{
}

GitBlameInlineNoteProvider::~GitBlameInlineNoteProvider()
{
    QPointer<KTextEditor::View> view = m_pluginView->activeView();
    if (view) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->unregisterInlineNoteProvider(this);
    }
}

QVector<int> GitBlameInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_pluginView->hasBlameInfo()) {
        return QVector<int>();
    }

    QPointer<KTextEditor::Document> doc = m_pluginView->activeDocument();
    if (!doc) {
        qDebug() << "no document";
        return QVector<int>();
    }

    if (m_mode == KateGitBlameMode::None) {
        return {};
    }

    int lineLen = doc->line(line).size();
    QPointer<KTextEditor::View> view = m_pluginView->activeView();
    if (view->cursorPosition().line() == line || m_mode == KateGitBlameMode::AllLines) {
        return QVector<int>{lineLen + 4};
    }
    return QVector<int>();
}

QSize GitBlameInlineNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    return QSize(note.lineHeight() * 50, note.lineHeight());
}

void GitBlameInlineNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter) const
{
    QFont font = note.font();
    painter.setFont(font);
    const QFontMetrics fm(note.font());

    int lineNr = note.position().line();
    const CommitInfo &info = m_pluginView->blameInfo(lineNr);

    bool isToday = info.authorDate.date() == QDate::currentDate();
    QString date =
        isToday ? m_locale.toString(info.authorDate.time(), QLocale::NarrowFormat) : m_locale.toString(info.authorDate.date(), QLocale::NarrowFormat);

    QString text = info.summary.isEmpty()
        ? i18nc("git-blame information \"author: date \"", " %1: %2 ", info.authorName, date)
        : i18nc("git-blame information \"author: date: commit title \"", " %1: %2: %3 ", info.authorName, date, QString::fromUtf8(info.summary));
    QRect rectangle{0, 0, fm.horizontalAdvance(text), note.lineHeight()};

    auto editor = KTextEditor::Editor::instance();
    auto color = QColor::fromRgba(editor->theme().textColor(KSyntaxHighlighting::Theme::Normal));
    color.setAlpha(0);
    painter.setPen(color);
    color.setAlpha(8);
    painter.setBrush(color);
    painter.drawRect(rectangle);

    color.setAlpha(note.underMouse() ? 130 : 90);
    painter.setPen(color);
    painter.setBrush(color);
    painter.drawText(rectangle, text);
}

void GitBlameInlineNoteProvider::inlineNoteActivated(const KTextEditor::InlineNote &note, Qt::MouseButtons buttons, const QPoint &)
{
    if ((buttons & Qt::LeftButton) != 0) {
        int lineNr = note.position().line();
        const CommitInfo &info = m_pluginView->blameInfo(lineNr);

        if (isUncomittedLine(info.hash)) {
            return;
        }

        // Hack: view->mainWindow()->view() to de-constify view
        Q_ASSERT(note.view() == m_pluginView->activeView());
        m_pluginView->showCommitInfo(QString::fromUtf8(info.hash), note.view()->mainWindow()->activeView());
    }
}

void GitBlameInlineNoteProvider::cycleMode()
{
    int newMode = (int)m_mode + 1;
    if (newMode > (int)KateGitBlameMode::Count) {
        newMode = 0;
    }
    setMode(KateGitBlameMode(newMode));
}

void GitBlameInlineNoteProvider::setMode(KateGitBlameMode mode)
{
    m_mode = mode;
    Q_EMIT inlineNotesReset();
}

K_PLUGIN_FACTORY_WITH_JSON(KateGitBlamePluginFactory, "kategitblameplugin.json", registerPlugin<KateGitBlamePlugin>();)

KateGitBlamePlugin::KateGitBlamePlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

QObject *KateGitBlamePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new KateGitBlamePluginView(this, mainWindow);
}

KateGitBlamePluginView::KateGitBlamePluginView(KateGitBlamePlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(plugin)
    , m_mainWindow(mainwindow)
    , m_inlineNoteProvider(this)
    , m_blameInfoProc(this)
    , m_showProc(this)
    , m_tooltip(this)
{
    KXMLGUIClient::setComponentName(QStringLiteral("kategitblameplugin"), i18n("Git Blame"));
    setXMLFile(QStringLiteral("ui.rc"));
    QAction *showBlameAction = actionCollection()->addAction(QStringLiteral("git_blame_show"));
    showBlameAction->setText(i18n("Show Git Blame Details"));
    actionCollection()->setDefaultShortcut(showBlameAction, Qt::CTRL | Qt::ALT | Qt::Key_G);
    QAction *toggleBlameAction = actionCollection()->addAction(QStringLiteral("git_blame_toggle"));
    toggleBlameAction->setText(i18n("Toggle Git Blame Details"));
    m_mainWindow->guiFactory()->addClient(this);

    connect(showBlameAction, &QAction::triggered, plugin, [this, showBlameAction]() {
        KTextEditor::View *kv = m_mainWindow->activeView();
        if (!kv) {
            return;
        }
        setToolTipIgnoreKeySequence(showBlameAction->shortcut());
        int lineNr = kv->cursorPosition().line();
        const CommitInfo &info = blameInfo(lineNr);
        showCommitInfo(QString::fromUtf8(info.hash), kv);
    });
    connect(toggleBlameAction, &QAction::triggered, this, [this]() {
        m_inlineNoteProvider.cycleMode();
    });

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KateGitBlamePluginView::viewChanged);

    connect(&m_blameInfoProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateGitBlamePluginView::blameFinished);

    connect(&m_showProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateGitBlamePluginView::showFinished);

    m_inlineNoteProvider.setMode(KateGitBlameMode::SingleLine);
}

KateGitBlamePluginView::~KateGitBlamePluginView()
{
    // ensure to kill, we segfault otherwise
    m_blameInfoProc.kill();
    m_blameInfoProc.waitForFinished();
    m_showProc.kill();
    m_showProc.waitForFinished();

    m_mainWindow->guiFactory()->removeClient(this);
}

QPointer<KTextEditor::View> KateGitBlamePluginView::activeView() const
{
    return m_mainWindow->activeView();
}

QPointer<KTextEditor::Document> KateGitBlamePluginView::activeDocument() const
{
    KTextEditor::View *view = m_mainWindow->activeView();
    if (view && view->document()) {
        return view->document();
    }
    return nullptr;
}

void KateGitBlamePluginView::viewChanged(KTextEditor::View *view)
{
    if (m_lastView) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(m_lastView)->unregisterInlineNoteProvider(&m_inlineNoteProvider);
    }
    m_lastView = view;

    if (view == nullptr || view->document() == nullptr) {
        return;
    }

    const auto url = view->document()->url();
    // This can happen for example if you were looking at a "temporary"
    // view like a diff. => do nothing
    if (url.isEmpty() || !url.isValid()) {
        return;
    }

    qobject_cast<KTextEditor::InlineNoteInterface *>(view)->registerInlineNoteProvider(&m_inlineNoteProvider);

    startBlameProcess(url);
}

void KateGitBlamePluginView::startBlameProcess(const QUrl &url)
{
    // same document? maybe split view? => no work to do, reuse the result we already have
    if (m_blameUrl == url) {
        return;
    }

    // clear everything
    m_blameUrl.clear();
    m_blamedLines.clear();
    m_blameInfoForHash.clear();

    // Kill any existing process...
    if (m_blameInfoProc.state() != QProcess::NotRunning) {
        m_blameInfoProc.kill();
        m_blameInfoProc.waitForFinished();
    }

    QString fileName{url.fileName()};
    QDir dir{url.toLocalFile()};
    dir.cdUp();

    setupGitProcess(m_blameInfoProc, dir.absolutePath(), {QStringLiteral("blame"), QStringLiteral("-p"), QStringLiteral("./%1").arg(fileName)});
    m_blameInfoProc.start(QIODevice::ReadOnly);
    m_blameUrl = url;
}

void KateGitBlamePluginView::startShowProcess(const QUrl &url, const QString &hash)
{
    if (m_showProc.state() != QProcess::NotRunning) {
        // Wait for the previous process to be done...
        return;
    }

    QDir dir{url.toLocalFile()};
    dir.cdUp();

    setupGitProcess(m_showProc, dir.absolutePath(), {QStringLiteral("show"), hash, QStringLiteral("--numstat")});
    m_showProc.start(QIODevice::ReadOnly);
}

void KateGitBlamePluginView::showCommitInfo(const QString &hash, KTextEditor::View *view)
{
    m_showHash = hash;
    startShowProcess(view->document()->url(), hash);
}

static int nextBlockStart(const QByteArray &out, int from)
{
    int next = out.indexOf('\t', from);
    // tab must be the first character in line for next block
    if (next > 0 && out[next - 1] != '\n') {
        next++;
        // move forward one line
        next = out.indexOf('\n', next);
        // try to look for another tab char
        next = out.indexOf('\t', next);
        // if not found => end
    }
    return next;
}

void KateGitBlamePluginView::sendMessage(const QString &text, bool error)
{
    QVariantMap genericMessage;
    genericMessage.insert(QStringLiteral("type"), error ? QStringLiteral("Error") : QStringLiteral("Info"));
    genericMessage.insert(QStringLiteral("category"), i18n("Git"));
    genericMessage.insert(QStringLiteral("categoryIcon"), QIcon(QStringLiteral(":/icons/icons/sc-apps-git.svg")));
    genericMessage.insert(QStringLiteral("text"), text);
    Q_EMIT message(genericMessage);
}

void KateGitBlamePluginView::blameFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        QString text = i18n("Git blame failed.");
        sendMessage(text + QStringLiteral("\n") + QString::fromUtf8(m_blameInfoProc.readAllStandardError()), true);
        return;
    }

    QByteArray out = m_blameInfoProc.readAllStandardOutput();
    out.replace("\r", ""); // KTextEditor removes all \r characters in the internal buffers
    // printf("recieved output: %d for: git %s\n", out.size(), qPrintable(m_blameInfoProc.arguments().join(QLatin1Char(' '))));

    /**
     * This is out git blame output parser.
     *
     * The output contains info about each line of text and commit info
     * for that line. We store the commit info separately in a hash-map
     * so that they don't need to be duplicated. For each line we store
     * its line text and short commit. Text is needed because if you
     * modify the doc, we use it to figure out where the original blame
     * line is. The short commit is used to fetch the full commit from
     * the hashmap
     */

    int start = 0;
    int next = out.indexOf('\t');
    next = out.indexOf('\n', next);

    while (next != -1) {
        //         printf("Block: (Size: %d) %s\n\n", (next - start), out.mid(start, next - start).constData());

        CommitInfo commitInfo;
        BlamedLine lineInfo;

        /**
         * Parse hash and line numbers
         *
         * 5c7f27a0915a9b20dc9f683d0d85b6e4b829bc85 1 1 5
         */
        int pos = out.indexOf(' ', start);
        constexpr int hashLen = 40;
        if (pos == -1 || (pos - start) != hashLen) {
            printf("no proper hash\n");
            break;
        }
        QByteArray hash = out.mid(start, pos - start);

        // skip to line end,
        // we don't care about line no etc here
        int from = pos + 1;
        pos = out.indexOf('\n', from);
        if (pos == -1) {
            qWarning() << "Git blame: Invalid blame output : No new line";
            break;
        }
        pos++;

        lineInfo.shortCommitHash = hash.mid(0, 7);

        m_blamedLines.push_back(lineInfo);

        // are we done because this line references the commit instead of
        // containing the content?
        if (out[pos] == '\t') {
            pos++; // skip \t
            from = pos;
            pos = out.indexOf('\n', from); // go to line end
            m_blamedLines.back().lineText = out.mid(from, pos - from);

            start = next + 1;
            next = nextBlockStart(out, start);
            if (next == -1)
                break;
            next = out.indexOf('\n', next);
            continue;
        }

        /**
         * Parse actual commit
         */
        commitInfo.hash = hash;

        // author Xyz
        constexpr int authorLen = sizeof("author ") - 1;
        pos += authorLen;
        from = pos;
        pos = out.indexOf('\n', pos);

        commitInfo.authorName = QString::fromUtf8(out.mid(from, pos - from));
        pos++;

        // author-time timestamp
        constexpr int authorTimeLen = sizeof("author-time ") - 1;
        pos = out.indexOf("author-time ", pos);
        if (pos == -1) {
            qWarning() << "Invalid commit while git-blameing";
            break;
        }
        pos += authorTimeLen;
        from = pos;
        pos = out.indexOf('\n', from);

        qint64 timestamp = out.mid(from, pos - from).toLongLong();
        commitInfo.authorDate = QDateTime::fromSecsSinceEpoch(timestamp);

        constexpr int summaryLen = sizeof("summary ") - 1;
        pos = out.indexOf("summary ", pos);
        pos += summaryLen;
        from = pos;
        pos = out.indexOf('\n', pos);

        commitInfo.summary = out.mid(from, pos - from);
        //         printf("Commit{\n %s,\n %s,\n %s,\n %s\n}\n", qPrintable(commitInfo.commitHash), qPrintable(commitInfo.name),
        //         qPrintable(commitInfo.date.toString()), qPrintable(commitInfo.title));

        m_blameInfoForHash[lineInfo.shortCommitHash] = commitInfo;

        from = pos;
        pos = out.indexOf('\t', from);
        from = pos + 1;
        pos = out.indexOf('\n', from);
        m_blamedLines.back().lineText = out.mid(from, pos - from);

        start = next + 1;
        next = nextBlockStart(out, start);
        if (next == -1)
            break;
        next = out.indexOf('\n', next);
    }
}

void KateGitBlamePluginView::showFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        QString text = i18n("Git blame, show commit failed.");
        sendMessage(text + QStringLiteral("\n") + QString::fromUtf8(m_showProc.readAllStandardError()), true);
        return;
    }

    QString stdOut = QString::fromUtf8(m_showProc.readAllStandardOutput());
    QStringList args = m_showProc.arguments();

    int titleStart = 0;
    for (int i = 0; i < 4; ++i) {
        titleStart = stdOut.indexOf(QLatin1Char('\n'), titleStart + 1);
        if (titleStart < 0 || titleStart >= stdOut.size() - 1) {
            qWarning() << "This is not a known git show format";
            return;
        }
    }

    int titleEnd = stdOut.indexOf(QLatin1Char('\n'), titleStart + 1);
    if (titleEnd < 0 || titleEnd >= stdOut.size() - 1) {
        qWarning() << "This is not a known git show format";
        return;
    }

    // Find 'Date:'
    int dateIdx = stdOut.indexOf(QStringLiteral("Date:"));
    if (dateIdx != -1) {
        int newLine = stdOut.indexOf(QLatin1Char('\n'), dateIdx);
        if (newLine != -1) {
            QString btn = QLatin1String("\n<a href=\"%1\">Click To Show Commit In Tree View</a>\n").arg(args[1]);
            stdOut.insert(newLine + 1, btn);
        }
    }

    if (!m_showHash.isEmpty() && m_showHash != args[1]) {
        startShowProcess(m_mainWindow->activeView()->document()->url(), m_showHash);
        return;
    }
    if (!m_showHash.isEmpty()) {
        m_showHash.clear();
        m_tooltip.show(stdOut, m_mainWindow->activeView());
    }
}

bool KateGitBlamePluginView::hasBlameInfo() const
{
    return !m_blamedLines.empty();
}

const CommitInfo &KateGitBlamePluginView::blameInfo(int lineNr)
{
    if (m_blamedLines.empty() || m_blameInfoForHash.isEmpty() || !activeDocument()) {
        return blameGetUpdateInfo(-1);
    }

    int totalBlamedLines = m_blamedLines.size();

    int adjustedLineNr = lineNr + m_lineOffset;
    const QByteArray lineText = activeDocument()->line(lineNr).toUtf8();

    if (adjustedLineNr >= 0 && adjustedLineNr < totalBlamedLines) {
        if (m_blamedLines[adjustedLineNr].lineText == lineText) {
            return blameGetUpdateInfo(adjustedLineNr);
        }
    }

    // search for the line 100 lines before and after until it matches
    m_lineOffset = 0;
    while (m_lineOffset < 100 && lineNr + m_lineOffset >= 0 && lineNr + m_lineOffset < totalBlamedLines) {
        if (m_blamedLines[lineNr + m_lineOffset].lineText == lineText) {
            return blameGetUpdateInfo(lineNr + m_lineOffset);
        }
        m_lineOffset++;
    }

    m_lineOffset = 0;
    while (m_lineOffset > -100 && lineNr + m_lineOffset >= 0 && (lineNr + m_lineOffset) < totalBlamedLines) {
        if (m_blamedLines[lineNr + m_lineOffset].lineText == lineText) {
            return blameGetUpdateInfo(lineNr + m_lineOffset);
        }
        m_lineOffset--;
    }

    return blameGetUpdateInfo(-1);
}

const CommitInfo &KateGitBlamePluginView::blameGetUpdateInfo(int lineNr)
{
    static const CommitInfo dummy{"hash", i18n("Not Committed Yet"), QDateTime::currentDateTime(), {}};
    if (m_blamedLines.empty() || lineNr < 0 || lineNr >= (int)m_blamedLines.size()) {
        return dummy;
    }

    auto &commitInfo = m_blamedLines[lineNr];

    Q_ASSERT(m_blameInfoForHash.contains(commitInfo.shortCommitHash));
    return m_blameInfoForHash[commitInfo.shortCommitHash];
}

void KateGitBlamePluginView::setToolTipIgnoreKeySequence(QKeySequence sequence)
{
    m_tooltip.setIgnoreKeySequence(sequence);
}

void KateGitBlamePluginView::showCommitTreeView(const QUrl &url)
{
    QString commitHash = url.toDisplayString();
    createToolView();
    m_commitFilesView->openCommit(commitHash, m_mainWindow->activeView()->document()->url().toLocalFile());
    m_mainWindow->showToolView(m_toolView.get());
}

void KateGitBlamePluginView::createToolView()
{
    if (m_toolView) {
        return;
    }

    auto plugin = static_cast<KTextEditor::Plugin *>(parent());
    m_toolView.reset(m_mainWindow->createToolView(plugin,
                                                  QStringLiteral("commitfilesview"),
                                                  KTextEditor::MainWindow::Left,
                                                  QIcon::fromTheme(QStringLiteral(":/icons/icons/sc-apps-git.svg")),
                                                  i18n("Commit")));

    m_commitFilesView = new CommitDiffTreeView(m_toolView.get());
    m_toolView->layout()->addWidget(m_commitFilesView);
    connect(m_commitFilesView, &CommitDiffTreeView::closeRequested, this, &KateGitBlamePluginView::hideToolView);
    connect(m_commitFilesView, &CommitDiffTreeView::showDiffRequested, this, &KateGitBlamePluginView::showDiffForFile);
}

void KateGitBlamePluginView::hideToolView()
{
    m_mainWindow->hideToolView(m_toolView.get());
    m_toolView.reset();
    // CommitFileView will be destroyed as well as it is the child of m_ToolView
}

void KateGitBlamePluginView::showDiffForFile(const QByteArray &diffContents)
{
    if (m_diffView) {
        m_diffView->document()->setText(QString::fromUtf8(diffContents));
        m_diffView->document()->setModified(false);
        m_mainWindow->activateView(m_diffView->document());
        m_diffView->setCursorPosition({0, 0});
        return;
    }
    m_diffView = m_mainWindow->openUrl(QUrl());
    m_diffView->document()->setHighlightingMode(QStringLiteral("Diff"));
    m_diffView->document()->setText(QString::fromUtf8(diffContents));
    m_diffView->document()->setModified(false);
    m_mainWindow->activateView(m_diffView->document());
    m_diffView->setCursorPosition({0, 0});
}

#include "kategitblameplugin.moc"
