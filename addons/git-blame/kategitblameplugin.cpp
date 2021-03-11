/*
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kategitblameplugin.h"
#include "gitblametooltip.h"

#include <algorithm>

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <KTextEditor/Editor>
#include <KTextEditor/Document>
#include <KTextEditor/InlineNoteInterface>
#include <KTextEditor/InlineNoteProvider>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QUrl>
#include <QDir>

#include <QFontMetricsF>
#include <QHash>
#include <QPainter>
#include <QRegularExpression>
#include <QVariant>

GitBlameInlineNoteProvider::GitBlameInlineNoteProvider(KTextEditor::Document *doc, KateGitBlamePlugin *plugin)
    : m_doc(doc)
    , m_plugin(plugin)
{
    for (auto view : m_doc->views()) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->registerInlineNoteProvider(this);
    }

    connect(m_doc, &KTextEditor::Document::viewCreated, this, [this](KTextEditor::Document *, KTextEditor::View *view) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->registerInlineNoteProvider(this);
    });

    // textInserted and textRemoved are emitted per line, then the last line is followed by a textChanged signal
    connect(m_doc, &KTextEditor::Document::textInserted, this, [/*this*/](KTextEditor::Document *, const KTextEditor::Cursor &/*cur*/, const QString &/*str*/) {
        //qDebug() << cur.line() << str << this;
    });
    connect(m_doc, &KTextEditor::Document::textRemoved, this, [/*this*/](KTextEditor::Document *, const KTextEditor::Range &/*range*/, const QString &/*str*/) {
        //qDebug() << range.start() << str << this;
    });

    connect(m_doc, &KTextEditor::Document::textChanged, this, [/*this*/](KTextEditor::Document *) {
        //qDebug() << this;
    });
}

GitBlameInlineNoteProvider::~GitBlameInlineNoteProvider()
{
    for (auto view : m_doc->views()) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->unregisterInlineNoteProvider(this);
    }
}

QVector<int> GitBlameInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_plugin->hasBlameInfo()) {
        return QVector<int>();
    }

    int lineLen = m_doc->line(line).size();
    for (const auto view: m_doc->views()) {
        if (view->cursorPosition().line() == line) {
            return QVector<int>{lineLen + 4};
        }
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
    const KateGitBlameInfo &info = m_plugin->blameInfo(lineNr, m_doc->line(lineNr));

    QString text = info.title.isEmpty() ? QStringLiteral(" %1: %2 ").arg(info.name, m_locale.toString(info.date, QLocale::NarrowFormat))
                                        : QStringLiteral(" %1: %2: %3 ").arg(info.name, m_locale.toString(info.date, QLocale::NarrowFormat), info.title);
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
        const KateGitBlameInfo &info = m_plugin->blameInfo(lineNr, m_doc->line(lineNr));

        // Hack: view->mainWindow()->view() to de-constify view
        m_plugin->showCommitInfo(info.commitHash, note.view()->mainWindow()->activeView());
    }
}

K_PLUGIN_FACTORY_WITH_JSON(KateGitBlamePluginFactory, "kategitblameplugin.json", registerPlugin<KateGitBlamePlugin>();)

KateGitBlamePlugin::KateGitBlamePlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
    , m_blameInfoProc(this)
{

    connect(&m_blameInfoProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateGitBlamePlugin::blameFinished);

    connect(&m_showProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateGitBlamePlugin::showFinished);
}

KateGitBlamePlugin::~KateGitBlamePlugin()
{
    qDeleteAll(m_inlineNoteProviders);
}

KateGitBlamePluginView::KateGitBlamePluginView(KateGitBlamePlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(plugin)
    , m_mainWindow(mainwindow)
{
    qDebug() << "creating new plugin view";
    KXMLGUIClient::setComponentName(QStringLiteral("kategitblameplugin"), i18n("Git Blame"));
    setXMLFile(QStringLiteral("ui.rc"));
    QAction *showBlameAction = actionCollection()->addAction(QStringLiteral("git_blame_show"));
    showBlameAction->setText(i18n("Show Git Blame Details"));
    actionCollection()->setDefaultShortcut(showBlameAction, Qt::CTRL | Qt::ALT | Qt::Key_G);

    connect(showBlameAction, &QAction::triggered, plugin, [this, plugin, showBlameAction]() {
        KTextEditor::View *kv = m_mainWindow->activeView();
        if (!kv) {
            return;
        }
        KTextEditor::Document *doc = kv->document();
        if (!doc) {
            return;
        }
        plugin->setToolTipIgnoreKeySequence(showBlameAction->shortcut());
        int lineNr = kv->cursorPosition().line();
        const KateGitBlameInfo &info = plugin->blameInfo(lineNr, doc->line(lineNr));
        plugin->showCommitInfo(info.commitHash, kv);
    });
    m_mainWindow->guiFactory()->addClient(this);
}

KateGitBlamePluginView::~KateGitBlamePluginView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

QObject *KateGitBlamePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
    for (auto view : m_mainWindow->views()) {
        addDocument(view->document());
    }

    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, [this](KTextEditor::View *view) {
        addDocument(view->document());
    });

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KateGitBlamePlugin::viewChanged);

    return new KateGitBlamePluginView(this, mainWindow);
}

void KateGitBlamePlugin::addDocument(KTextEditor::Document *doc)
{
    if (!m_inlineNoteProviders.contains(doc)) {
        m_inlineNoteProviders.insert(doc, new GitBlameInlineNoteProvider(doc, this));
    }

    connect(doc, &KTextEditor::Document::destroyed, this, [this, doc]() {
        m_inlineNoteProviders.remove(doc);
    });
    connect(doc, &KTextEditor::Document::reloaded, this, [this, doc]() {
        startBlameProcess(doc->url());
    });
    connect(doc, &KTextEditor::Document::documentSavedOrUploaded, this, [this, doc]() {
        startBlameProcess(doc->url());
    });
}

void KateGitBlamePlugin::viewChanged(KTextEditor::View *view)
{
    m_blameInfo.clear();

    if (view == nullptr || view->document() == nullptr) {
        return;
    }
    m_blameInfoView = view;
    startBlameProcess(view->document()->url());
}

void KateGitBlamePlugin::startBlameProcess(const QUrl &url)
{
    if (m_blameInfoProc.state() != QProcess::NotRunning) {
        // Wait for the previous process to be done...
        return;
    }

    QString fileName{url.fileName()};
    QDir dir{url.toLocalFile()};
    dir.cdUp();

    QStringList args{QStringLiteral("blame"), QStringLiteral("--date=iso-strict"), QStringLiteral("./%1").arg(fileName)};

    m_blameInfoProc.setWorkingDirectory(dir.absolutePath());
    m_blameInfoProc.start(QStringLiteral("git"), args, QIODevice::ReadOnly);
}

void KateGitBlamePlugin::startShowProcess(const QUrl &url, const QString &hash)
{
    if (m_showProc.state() != QProcess::NotRunning) {
        // Wait for the previous process to be done...
        return;
    }
    if (hash == m_activeCommitInfo.m_hash) {
        // We have already the data
        return;
    }

    QDir dir{url.toLocalFile()};
    dir.cdUp();

    QStringList args{QStringLiteral("show"),  hash};
    m_showProc.setWorkingDirectory(dir.absolutePath());
    m_showProc.start(QStringLiteral("git"), args, QIODevice::ReadOnly);
}

void KateGitBlamePlugin::showCommitInfo(const QString &hash, KTextEditor::View *view)
{

    if (hash == m_activeCommitInfo.m_hash) {
        m_showHash.clear();
        m_tooltip.show(m_activeCommitInfo.m_content, view);
    } else {
        m_showHash = hash;
        startShowProcess(view->document()->url(), hash);
    }
}

void KateGitBlamePlugin::blameFinished(int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
{
    QString stdErr = QString::fromUtf8(m_blameInfoProc.readAllStandardError());
    const QStringList stdOut = QString::fromUtf8(m_blameInfoProc.readAllStandardOutput()).split(QLatin1Char('\n'));

    // check if the git process was running for a previous document when the view changed.
    // if that is the case re-trigger the process and skip this data
    if (m_blameInfoView != m_mainWindow->activeView()) {
        viewChanged(m_mainWindow->activeView());
        return;
    }

    const static QRegularExpression lineReg(QStringLiteral("(\\S+)[^\\(]+\\((.*)\\s+(\\d\\d\\d\\d-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d\\S+)[^\\)]+\\)\\s(.*)"));

    m_blameInfo.clear();

    for (const auto &line: stdOut) {
        const QRegularExpressionMatch match = lineReg.match(line);
        if (match.hasMatch()) {
            m_blameInfo.append(
                {match.captured(1), match.captured(2).trimmed(), QDateTime::fromString(match.captured(3), Qt::ISODate), QString(), match.captured(4)});
        }
    }
}

void KateGitBlamePlugin::showFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString stdErr = QString::fromUtf8(m_showProc.readAllStandardError());
    const QString stdOut = QString::fromUtf8(m_showProc.readAllStandardOutput());

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        return;
    }
    QStringList args = m_showProc.arguments();
    if (args.size() != 2) {
        qWarning() << "Wrong number of parameters:" << args;
        return;
    }

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

    m_activeCommitInfo.m_title = stdOut.mid(titleStart, titleEnd - titleStart);
    m_activeCommitInfo.m_hash = args[1];
    m_activeCommitInfo.m_title = m_activeCommitInfo.m_title.trimmed();
    m_activeCommitInfo.m_content = stdOut;

    if (!m_showHash.isEmpty() && m_showHash != args[1]) {
        startShowProcess(m_mainWindow->activeView()->document()->url(), m_showHash);
        return;
    }
    if (!m_showHash.isEmpty()) {
        m_showHash.clear();
        m_tooltip.show(stdOut, m_mainWindow->activeView());
    }
}

bool KateGitBlamePlugin::hasBlameInfo() const
{
    return !m_blameInfo.isEmpty();
}

const KateGitBlameInfo &KateGitBlamePlugin::blameInfo(int lineNr, const QStringView &lineText)
{
    if (m_blameInfo.isEmpty()) {
        return blameGetUpdateInfo(-1);
    }

    int adjustedLineNr = lineNr + m_lineOffset;

    if (adjustedLineNr >= 0 && adjustedLineNr < m_blameInfo.size()) {
        if (m_blameInfo[adjustedLineNr].line == lineText) {
            return blameGetUpdateInfo(adjustedLineNr);
        }
    }

    // FIXME search for the matching line
    // search for the line 100 lines before and after until it matches
    m_lineOffset = 0;
    while (m_lineOffset < 100  &&
        lineNr+m_lineOffset >= 0 &&
        lineNr+m_lineOffset < m_blameInfo.size())
    {
        if (m_blameInfo[lineNr+m_lineOffset].line == lineText) {
            return blameGetUpdateInfo(lineNr + m_lineOffset);
        }
        m_lineOffset++;
    }

    m_lineOffset = 0;
    while (m_lineOffset > -100  &&
        lineNr+m_lineOffset >= 0 &&
        (lineNr+m_lineOffset) < m_blameInfo.size())
    {
        if (m_blameInfo[lineNr+m_lineOffset].line == lineText) {
            return blameGetUpdateInfo(lineNr + m_lineOffset);
        }
        m_lineOffset--;
    }

    return blameGetUpdateInfo(-1);
}

const KateGitBlameInfo &KateGitBlamePlugin::blameGetUpdateInfo(int lineNr)
{
    static const KateGitBlameInfo dummy{QStringLiteral("hash"),
                                        i18n("Not Committed Yet"),
                                        QDateTime::currentDateTime(),
                                        QStringLiteral(""),
                                        QStringLiteral("")};

    if (m_blameInfo.isEmpty() || lineNr < 0 || lineNr >= m_blameInfo.size()) {
        return dummy;
    }

    KateGitBlameInfo &info = m_blameInfo[lineNr];
    if (info.commitHash == m_activeCommitInfo.m_hash) {
        if (info.title != m_activeCommitInfo.m_title) {
            info.title = m_activeCommitInfo.m_title;
        }
    } else {
        startShowProcess(m_mainWindow->activeView()->document()->url(), info.commitHash);
    }
    return info;
}

void KateGitBlamePlugin::setToolTipIgnoreKeySequence(QKeySequence sequence)
{
    m_tooltip.setIgnoreKeySequence(sequence);
}

void KateGitBlamePlugin::readConfig()
{
}

void KateGitBlamePlugin::CommitInfo::clear()
{
    m_hash.clear();
    m_title.clear();
    m_content.clear();
}

#include "kategitblameplugin.moc"
