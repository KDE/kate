/*
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kategitblameplugin.h"
#include "gitblametooltip.h"

#include <algorithm>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
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

    QString text = QStringLiteral("  %1: %2").arg(info.name, info.date);
    QRect rectangle = fm.boundingRect(text);
    rectangle.setWidth(rectangle.width() * 1.1);
    rectangle.moveTo(0,0);

    auto editor = KTextEditor::Editor::instance();
    auto color = QColor::fromRgba(editor->theme().textColor(KSyntaxHighlighting::Theme::Normal));
    color.setAlpha(0);
    painter.setPen(color);
    color.setAlpha(15);
    painter.setBrush(color);
    painter.drawRect(0,0, rectangle.width(), note.lineHeight());
    color.setAlpha(50);
    painter.setBrush(color);
    painter.drawRect(0, 0, 3, note.lineHeight());

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
        m_plugin->showCommitInfo(info.commitHash);
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


    return nullptr;
}

void KateGitBlamePlugin::addDocument(KTextEditor::Document *doc)
{
    if (!m_inlineNoteProviders.contains(doc)) {
        m_inlineNoteProviders.insert(doc, new GitBlameInlineNoteProvider(doc, this));
    }

    connect(doc, &KTextEditor::Document::destroyed, this, [this, doc]() {
        m_inlineNoteProviders.remove(doc);
    });
}

void KateGitBlamePlugin::viewChanged(KTextEditor::View *view)
{
    m_blameInfo.clear();

    if (view == nullptr || view->document() == nullptr) {
        return;
    }

    if (m_blameInfoProc.state() != QProcess::NotRunning) {
        // Wait for the previous process to be done...
        return;
    }

    m_blameInfoView = view;

    QUrl url = view->document()->url();
    QString fileName{url.fileName()};
    QDir dir{url.toLocalFile()};
    dir.cdUp();

    QStringList args{QStringLiteral("blame"),  QStringLiteral("./%1").arg(fileName)};

    m_blameInfoProc.setWorkingDirectory(dir.absolutePath());
    m_blameInfoProc.start(QStringLiteral("git"), args);
}

void KateGitBlamePlugin::showCommitInfo(const QString &hash)
{
    if (!m_mainWindow || !m_mainWindow->activeView() || !m_mainWindow->activeView()->document()) {
        return;
    }

    QUrl url = m_mainWindow->activeView()->document()->url();
    QDir dir{url.toLocalFile()};
    dir.cdUp();

    QStringList args{QStringLiteral("show"),  hash};

    m_showProc.setWorkingDirectory(dir.absolutePath());
    m_showProc.start(QStringLiteral("git"), args);
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

    const static QRegularExpression lineReg(QStringLiteral("(\\S+)[^\\(]+\\((.*)\\s+(\\d\\d\\d\\d-\\d\\d-\\d\\d \\d\\d:\\d\\d:\\d\\d)[^\\)]+\\)\\s(.*)"));

    m_blameInfo.clear();

    for (const auto &line: stdOut) {
        const QRegularExpressionMatch match = lineReg.match(line);
        if (match.hasMatch()) {
            m_blameInfo.append({ match.captured(1), match.captured(2).trimmed(), match.captured(3), match.captured(4) });
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

    m_tooltip.show(stdOut, m_mainWindow->activeView());
}

bool KateGitBlamePlugin::hasBlameInfo() const
{
    return !m_blameInfo.isEmpty();
}

const KateGitBlameInfo &KateGitBlamePlugin::blameInfo(int lineNr, const QStringView &lineText)
{
    static const KateGitBlameInfo dummy{QStringLiteral("hash"), i18n("Not Committed Yet"), QStringLiteral(""), QStringLiteral("")};

    if (m_blameInfo.isEmpty()) {
        return dummy;
    }

    int adjustedLineNr = lineNr + m_lineOffset;

    if (adjustedLineNr >= 0 && adjustedLineNr < m_blameInfo.size()) {
        if (m_blameInfo[adjustedLineNr].line == lineText) {
            return m_blameInfo.at(adjustedLineNr);
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
            return m_blameInfo.at(lineNr+m_lineOffset);
        }
        m_lineOffset++;
    }

    m_lineOffset = 0;
    while (m_lineOffset > -100  &&
        lineNr+m_lineOffset >= 0 &&
        (lineNr+m_lineOffset) < m_blameInfo.size())
    {
        if (m_blameInfo[lineNr+m_lineOffset].line == lineText) {
            return m_blameInfo.at(lineNr+m_lineOffset);
        }
        m_lineOffset--;
    }

    return dummy;
}


void KateGitBlamePlugin::readConfig()
{
}

#include "kategitblameplugin.moc"
