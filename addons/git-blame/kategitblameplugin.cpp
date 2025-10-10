/*
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kategitblameplugin.h"
#include "ktexteditor_utils.h"

#include "hostprocess.h"
#include <commitfilesview.h>
#include <gitprocess.h>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QFileInfo>
#include <QFontMetrics>
#include <QKeySequence>
#include <QLayout>
#include <QPainter>
#include <QtMath>

static bool isUncomittedLine(QByteArrayView hash)
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
    if (m_pluginView->activeView()) {
        m_pluginView->activeView()->unregisterInlineNoteProvider(this);
    }
}

QList<int> GitBlameInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_pluginView->hasBlameInfo()) {
        return QList<int>();
    }

    QPointer<KTextEditor::Document> doc = m_pluginView->activeDocument();
    if (!doc) {
        return QList<int>();
    }

    if (m_mode == KateGitBlameMode::None) {
        return {};
    }

    int lineLen = doc->line(line).size();
    QPointer<KTextEditor::View> view = m_pluginView->activeView();
    if (view->cursorPosition().line() == line || m_mode == KateGitBlameMode::AllLines) {
        return QList<int>{lineLen + 4};
    }
    return QList<int>();
}

QSize GitBlameInlineNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    return QSize(note.lineHeight() * 50, note.lineHeight());
}

void GitBlameInlineNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection dir) const
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
    bool isRTL = false;
    isRTL = dir == Qt::RightToLeft;

    if (isRTL) {
        const qreal horizontalTx = painter.worldTransform().m31();
        // the amount of translation by x is from start of view (0,0) i.e., its the max
        // amount of space we can use in rtl.
        const int availableWidth = qFloor(horizontalTx);
        const auto rectWidth = rectangle.width();
        // the painter is translated to be at the end of line, move it left so that it is
        // in front of the RTL line
        const int moveBy = -(qAbs(rectWidth), qAbs(availableWidth));
        rectangle.moveLeft(moveBy);
        if (rectWidth > availableWidth) {
            // reduce the width to available width
            rectangle.setWidth(availableWidth);
            // elide the text in the middle
            text = painter.fontMetrics().elidedText(text, Qt::ElideMiddle, availableWidth);
        }
    }

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
    painter.drawText(rectangle, Qt::AlignLeft | Qt::AlignVCenter, text);
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

KateGitBlamePlugin::KateGitBlamePlugin(QObject *parent)
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
    KActionCollection::setDefaultShortcut(showBlameAction, QKeySequence(QStringLiteral("Ctrl+T, B"), QKeySequence::PortableText));
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

    m_startBlameTimer.setSingleShot(true);
    m_startBlameTimer.setInterval(400);
    m_startBlameTimer.callOnTimeout(this, &KateGitBlamePluginView::startGitBlameForActiveView);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, [this](KTextEditor::View *) {
        m_startBlameTimer.start();

        m_tooltip.hide();
    });

    connect(&m_blameInfoProc, &QProcess::finished, this, &KateGitBlamePluginView::commandFinished);
    connect(&m_showProc, &QProcess::finished, this, &KateGitBlamePluginView::showFinished);

    connect(&m_blameInfoProc, &QProcess::errorOccurred, this, &KateGitBlamePluginView::onErrorOccurred);
    connect(&m_showProc, &QProcess::errorOccurred, this, &KateGitBlamePluginView::onErrorOccurred);

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

void KateGitBlamePluginView::startGitBlameForActiveView()
{
    if (m_lastView) {
        m_lastView->unregisterInlineNoteProvider(&m_inlineNoteProvider);
    }

    auto *view = m_mainWindow->activeView();
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

    view->registerInlineNoteProvider(&m_inlineNoteProvider);
    startBlameProcess(url);
}

void KateGitBlamePluginView::startBlameProcess(const QUrl &url)
{
    const QFileInfo fi{url.toLocalFile()};
    // same document? maybe split view? => no work to do, reuse the result we already have
    if (fi.absoluteFilePath() == m_absoluteFilePath) {
        return;
    }
    m_parentPath = fi.absolutePath();
    m_absoluteFilePath = fi.absoluteFilePath();

    // clear everything
    m_rawCommitData.clear();
    m_blamedLines.clear();
    m_blameInfoForHash.clear();

    // Kill any existing process...
    if (m_blameInfoProc.state() != QProcess::NotRunning) {
        m_blameInfoProc.kill();
        m_blameInfoProc.waitForFinished();
    }

    m_currentCommand = Command::RevParse;
    if (!setupGitProcess(m_blameInfoProc, m_parentPath, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")})) {
        return;
    }
    startHostProcess(m_blameInfoProc, QIODevice::ReadOnly);
}

void KateGitBlamePluginView::startShowProcess(const QUrl &url, const QString &hash)
{
    if (m_showProc.state() != QProcess::NotRunning) {
        // Wait for the previous process to be done...
        return;
    }

    const QFileInfo fi{url.toLocalFile()};
    m_absoluteFilePath = fi.absoluteFilePath();
    if (!setupGitProcess(m_showProc, fi.absolutePath(), {QStringLiteral("show"), hash, QStringLiteral("--numstat")})) {
        return;
    }
    startHostProcess(m_showProc, QIODevice::ReadOnly);
}

void KateGitBlamePluginView::showCommitInfo(const QString &hash, KTextEditor::View *view)
{
    m_showHash = hash;
    startShowProcess(view->document()->url(), hash);
}

void KateGitBlamePluginView::sendMessage(const QString &text, bool error)
{
    Utils::showMessage(text, gitIcon(), i18n("Git"), error ? MessageType::Error : MessageType::Info, m_mainWindow);
}

void KateGitBlamePluginView::commandFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // we ignore errors, we might just not be in a git repo, parsing errors is hard, as they are translated
    // switching to english is no good idea either, as the user will likely not understand it then anyways
    // Git returns error code 1 if IgnoreRevsFile is not found, so we ignore the error for it
    if (m_currentCommand != Command::IgnoreRevsFile && (exitCode != 0 || exitStatus != QProcess::NormalExit)) {
        return;
    }

    switch (m_currentCommand) {
    case Command::RevParse: {
        m_root = QString::fromUtf8(m_blameInfoProc.readAllStandardOutput().trimmed());

        if (!setupGitProcess(m_blameInfoProc, m_parentPath, {QStringLiteral("config"), QStringLiteral("blame.ignoreRevsFile")})) {
            return;
        }
        m_currentCommand = Command::IgnoreRevsFile;
        startHostProcess(m_blameInfoProc, QIODevice::ReadOnly);
        break;
    }
    case Command::Config: {
        m_ignoreRevsFile = QString::fromUtf8(m_blameInfoProc.readAllStandardOutput().trimmed());

        auto arguments = QStringList{QStringLiteral("blame"), QStringLiteral("-p")};

        // add ignore-revs-file
        const auto ignoreRevFile = !m_ignoreRevsFile.isEmpty() ? m_ignoreRevsFile : QStringLiteral(".git-blame-ignore-revs");
        const auto ignoreRevFilePath = QStringLiteral("%1/%2").arg(m_root, ignoreRevFile);
        if (QFileInfo::exists(ignoreRevFilePath)) {
            arguments.append({QStringLiteral("--ignore-revs-file"), ignoreRevFilePath});
        }
        arguments.append(m_absoluteFilePath);

        m_currentCommand = Command::Blame;
        if (!setupGitProcess(m_blameInfoProc, m_parentPath, arguments)) {
            return;
        }
        startHostProcess(m_blameInfoProc, QIODevice::ReadOnly);
        break;
    }
    case Command::Blame:
        parseGitBlameStdOutput();
        break;
    case Command::IgnoreRevsFile:
        m_currentCommand = Command::Config;
        commandFinished(0, QProcess::ExitStatus::NormalExit);
        break;
    }
}

struct LineBlock {
    int commitEnd;
    int lineEnd;
};

static LineBlock nextBlock(QByteArrayView rawData, int from)
{
    // A line block is either
    // <40 char hash> 1 2 3\n
    // \t<line data>\n
    // or
    // <40 char hash> 1 2 3\n
    // author ...\n
    // ...
    // author-time ...\n
    // ...
    // summary ...
    // \t<line data>\n
    //
    // As every code line starts with a \t, we can use \n\t to find the beginning of the code line
    int commitEnd = rawData.indexOf("\n\t", from) + 1;
    if (commitEnd == 0) {
        return {-1, -1};
    }
    int blockEnd = rawData.indexOf('\n', commitEnd);
    return {commitEnd, blockEnd};
}

void KateGitBlamePluginView::parseGitBlameStdOutput()
{
    // Save the data, everything else has views over this data
    m_rawCommitData = m_blameInfoProc.readAllStandardOutput();
    m_rawCommitData.replace("\r", ""); // KTextEditor removes all \r characters in the internal buffers

    QByteArrayView out = m_rawCommitData;
    QHash<QByteArrayView, QString> authorCache;

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

    while (start != -1) {
        auto block = nextBlock(out, start);
        if (block.commitEnd == -1 || block.lineEnd == -1) {
            return;
        }
        // qDebug() << start << block.commitEnd << block.lineEnd;
        // qDebug() << out.mid(start, block.commitEnd - start);
        // qDebug() << out.mid(block.commitEnd, block.lineEnd - block.commitEnd);

        CommitInfo commitInfo;
        BlamedLine lineInfo;

        /**
         * Parse hash and line numbers
         *
         * 5c7f27a0915a9b20dc9f683d0d85b6e4b829bc85 1 1 5
         */
        int hashEnd = out.indexOf(' ', start);
        constexpr int hashLen = 40;
        if (hashEnd == -1 || (hashEnd - start) != hashLen) {
            qWarning() << "Not a proper hash:" << out.mid(start, hashEnd - start);
            break;
        }
        QByteArrayView hash = out.mid(start, hashEnd - start);
        lineInfo.shortCommitHash = hash.mid(0, 7);
        m_blamedLines.push_back(lineInfo);

        // skip to line end,
        // we don't care about line no etc here
        start = out.indexOf('\n', hashEnd);
        if (start == -1) {
            qWarning("Git blame: Invalid blame output : No new line");
            break;
        }
        start++;

        // are we done because this line references the commit instead of
        // containing the content?
        if (start == block.commitEnd) {
            start++; // skip the \t
            m_blamedLines.back().lineText = out.mid(start, block.lineEnd - start);
            start = block.lineEnd + 1;
            continue;
        }

        /**
         * Parse commit
         */
        commitInfo.hash = hash;

        // author Xyz
        constexpr int authorLen = (int)sizeof("author");
        start += authorLen;
        int authorEnd = out.indexOf('\n', start);

        QByteArrayView authorView = out.mid(start, authorEnd - start);
        auto it = authorCache.find(authorView);
        if (it == authorCache.end()) {
            it = authorCache.insert(authorView, QString::fromUtf8(authorView));
        }
        commitInfo.authorName = it.value();

        start++;

        // author-time timestamp
        constexpr int authorTimeLen = (int)sizeof("author-time");
        start = out.indexOf("author-time ", start);
        if (start == -1) {
            qWarning("Invalid commit while git-blameing");
            break;
        }
        start += authorTimeLen;
        int timeEnd = out.indexOf('\n', start);

        qint64 timestamp = out.mid(start, timeEnd - start).toLongLong();
        commitInfo.authorDate = QDateTime::fromSecsSinceEpoch(timestamp);

        constexpr int summaryLen = (int)sizeof("summary");
        start = out.indexOf("summary ", start);
        start += summaryLen;
        int summaryEnd = out.indexOf('\n', start);

        commitInfo.summary = out.mid(start, summaryEnd - start);
        // qDebug() << "\nCommit {\n" //
        //          << commitInfo.hash << "\n" //
        //          << commitInfo.authorName << "\n" //
        //          << commitInfo.authorDate.toString() << "\n" //
        //          << commitInfo.summary << "\n}";

        m_blameInfoForHash[lineInfo.shortCommitHash] = commitInfo;
        m_blamedLines.back().lineText = out.mid(block.commitEnd + 1, block.lineEnd - block.commitEnd - 1);
        start = block.lineEnd + 1;
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
    const QStringList args = m_showProc.arguments();

    const int showIndex = args.indexOf(u"show");
    if (showIndex < 0 || showIndex >= args.size()) {
        qWarning("Bad git show command, did not find 'show' argument. %ls", qUtf16Printable(args.join(QLatin1String(" "))));
        Q_UNREACHABLE();
    }
    const QString commitHashArg = args[showIndex + 1];

    int titleStart = 0;
    for (int i = 0; i < 4; ++i) {
        titleStart = stdOut.indexOf(QLatin1Char('\n'), titleStart + 1);
        if (titleStart < 0 || titleStart >= stdOut.size() - 1) {
            qWarning("This is not a known git show format");
            return;
        }
    }

    int titleEnd = stdOut.indexOf(QLatin1Char('\n'), titleStart + 1);
    if (titleEnd < 0 || titleEnd >= stdOut.size() - 1) {
        qWarning("This is not a known git show format");
        return;
    }

    // Find 'Date:'
    int dateIdx = stdOut.indexOf(QStringLiteral("Date:"));
    if (dateIdx != -1) {
        int newLine = stdOut.indexOf(QLatin1Char('\n'), dateIdx);
        if (newLine != -1) {
            QString btn = QLatin1String("\n<a href=\"%1\">Click To Show Commit In Tree View</a>\n").arg(commitHashArg);
            stdOut.insert(newLine + 1, btn);
        }
    }

    if (!m_showHash.isEmpty() && m_showHash != commitHashArg) {
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

    const int totalBlamedLines = (int)m_blamedLines.size();
    const int adjustedLineNr = lineNr + m_lineOffset;
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
    static const CommitInfo dummy{.hash = "hash", .authorName = i18n("Not Committed Yet"), .authorDate = QDateTime::currentDateTime(), .summary = {}};
    if (m_blamedLines.empty() || lineNr < 0 || lineNr >= (int)m_blamedLines.size()) {
        return dummy;
    }

    auto &commitInfo = m_blamedLines[lineNr];

    Q_ASSERT(m_blameInfoForHash.contains(commitInfo.shortCommitHash));
    return m_blameInfoForHash[commitInfo.shortCommitHash];
}

void KateGitBlamePluginView::setToolTipIgnoreKeySequence(const QKeySequence &sequence)
{
    m_tooltip.setIgnoreKeySequence(sequence);
}

void KateGitBlamePluginView::showCommitTreeView(const QUrl &url)
{
    QString commitHash = url.toDisplayString();
    const auto file = m_mainWindow->activeView()->document()->url().toLocalFile();
    CommitView::openCommit(commitHash, file, m_mainWindow);
}

void KateGitBlamePluginView::onErrorOccurred(QProcess::ProcessError e)
{
    auto process = qobject_cast<QProcess *>(sender());
    if (process) {
        sendMessage(QStringLiteral("%1 with args %2, error occurred: %3.").arg(process->program(), process->arguments().join(QLatin1Char(' '))).arg(e), false);
    }
}

#include "kategitblameplugin.moc"
