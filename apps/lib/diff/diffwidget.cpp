/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "diffwidget.h"
#include "commitinfoview.h"
#include "gitdiff.h"
#include "gitprocess.h"
#include "hostprocess.h"
#include "ktexteditor_utils.h"

#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QStyle>
#include <QSyntaxHighlighter>
#include <QTemporaryFile>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <ktexteditor/cursor.h>

Q_DECL_COLD_FUNCTION static void gitNotFoundMessage()
{
    Utils::showMessage(i18n("<b>git</b> not found. Git is needed to diff the documents. If git is already installed, make sure it is your PATH variable. See "
                            "https://git-scm.com/downloads"),
                       gitIcon(),
                       i18n("Diff"),
                       MessageType::Error);
}

DiffWidget *DiffWidgetManager::existingDiffWidgetForParams(KTextEditor::MainWindow *mw, const DiffParams &p)
{
    const auto widgets = Utils::widgets(mw);
    for (auto widget : widgets) {
        auto diffWidget = qobject_cast<DiffWidget *>(widget);
        if (!diffWidget) {
            continue;
        }

        if (diffWidget->m_params.arguments == p.arguments) {
            return diffWidget;
            break;
        }
    }
    return nullptr;
}

void DiffWidgetManager::openDiff(const QByteArray &diff, DiffParams p, class KTextEditor::MainWindow *mw)
{
    DiffWidget *existing = existingDiffWidgetForParams(mw, p);
    if (!existing) {
        existing = new DiffWidget(p);
        mw->connect(existing, &DiffWidget::openFileRequested, mw, [mw](QString path, int line, int columnNumber) {
            auto view = mw->openUrl(QUrl(path));
            view->setCursorPosition(KTextEditor::Cursor(line - 1, columnNumber));
        });
        if (!p.tabTitle.isEmpty()) {
            existing->setWindowTitle(p.tabTitle);
        } else {
            if (p.destFile.isEmpty())
                existing->setWindowTitle(i18n("Diff %1", Utils::fileNameFromPath(p.srcFile)));
            else
                existing->setWindowTitle(i18n("Diff %1..%2", Utils::fileNameFromPath(p.srcFile), Utils::fileNameFromPath(p.destFile)));
        }
        existing->setWindowIcon(QIcon::fromTheme(QStringLiteral("text-x-patch")));
        existing->openDiff(diff);
        mw->addWidget(existing);
    } else {
        existing->clearData();
        existing->m_params = p;
        existing->openDiff(diff);
        mw->activateWidget(existing);
    }
}

void DiffWidgetManager::diffDocs(KTextEditor::Document *l, KTextEditor::Document *r, class KTextEditor::MainWindow *mw)
{
    DiffParams p;
    p.arguments = DiffWidget::diffDocsGitArgs(l, r);
    p.flags.setFlag(DiffParams::Flag::ShowEditLeftSide, true);
    p.flags.setFlag(DiffParams::Flag::ShowEditRightSide, true);
    p.srcFile = l->url().toLocalFile();
    p.destFile = r->url().toLocalFile();
    DiffWidget *existing = existingDiffWidgetForParams(mw, p);
    if (!existing) {
        existing = new DiffWidget(p);
        mw->connect(existing, &DiffWidget::openFileRequested, mw, [mw](QString path, int line, int columnNumber) {
            auto view = mw->openUrl(QUrl(path));
            view->setCursorPosition(KTextEditor::Cursor(line - 1, columnNumber));
        });
        existing->diffDocs(l, r);
        existing->setWindowTitle(i18n("Diff %1 .. %2", l->documentName(), r->documentName()));
        existing->setWindowIcon(QIcon::fromTheme(QStringLiteral("text-x-patch")));
        mw->addWidget(existing);
    } else {
        existing->clearData();
        existing->m_params = p;
        existing->diffDocs(l, r);
        mw->activateWidget(existing);
    }
}

class Toolbar : public QToolBar
{
    Q_OBJECT
public:
    Toolbar(QWidget *parent)
        : QToolBar(parent)
    {
        setContentsMargins({});
        if (layout()) {
            layout()->setContentsMargins({});
        }

        setToolButtonStyle(Qt::ToolButtonIconOnly);

        KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
        bool show = cgGeneral.readEntry("DiffWidget Show Commit Info", true);

        m_showCommitInfoAction = addAction(QIcon::fromTheme(QStringLiteral("view-visible")), QString());
        m_showCommitInfoAction->setCheckable(true);
        m_showCommitInfoAction->setToolTip(i18n("Show/Hide Commit Info"));
        m_showCommitInfoAction->setChecked(show);
        connect(m_showCommitInfoAction, &QAction::toggled, this, &Toolbar::showCommitInfoChanged);

        m_showNextFile = addAction(QIcon::fromTheme(QStringLiteral("arrow-down-double")), QString());
        m_showNextFile->setToolTip(i18n("Jump to Next File"));
        connect(m_showNextFile, &QAction::triggered, this, &Toolbar::jumpToNextFile);

        m_showPrevFile = addAction(QIcon::fromTheme(QStringLiteral("arrow-up-double")), QString());
        m_showPrevFile->setToolTip(i18n("Jump to Previous File"));
        connect(m_showPrevFile, &QAction::triggered, this, &Toolbar::jumpToPrevFile);

        m_showNextHunk = addAction(QIcon::fromTheme(QStringLiteral("arrow-down")), QString());
        m_showNextHunk->setToolTip(i18n("Jump to Next Hunk"));
        connect(m_showNextHunk, &QAction::triggered, this, &Toolbar::jumpToNextHunk);

        m_showPrevHunk = addAction(QIcon::fromTheme(QStringLiteral("arrow-up")), QString());
        m_showPrevHunk->setToolTip(i18n("Jump to Previous Hunk"));
        connect(m_showPrevHunk, &QAction::triggered, this, &Toolbar::jumpToPrevHunk);

        m_reload = addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), QString());
        m_reload->setToolTip(i18nc("Tooltip for a button, clicking the button reloads the diff", "Reload Diff"));
        connect(m_reload, &QAction::triggered, this, &Toolbar::reload);

        m_fullContext = addAction(QIcon::fromTheme(QStringLiteral("view-fullscreen")), QString());
        m_fullContext->setToolTip(i18nc("Tooltip for a button", "Show diff with full context, not just the changed lines"));
        m_fullContext->setCheckable(true);
        connect(m_fullContext, &QAction::triggered, this, &Toolbar::showWithFullContextChanged);
    }

    void setShowCommitActionVisible(bool vis)
    {
        if (m_showCommitInfoAction->isVisible() != vis) {
            m_showCommitInfoAction->setVisible(vis);
        }
    }

    void setShowFullContextVisible(bool value)
    {
        if (m_fullContext->isVisible() != value) {
            m_fullContext->setVisible(value);
        }
    }

    bool showCommitInfo()
    {
        return m_showCommitInfoAction->isChecked();
    }

    void setShowFullContext(bool fullContextEnabled)
    {
        m_fullContext->setChecked(fullContextEnabled);
    }

private:
    QAction *m_showCommitInfoAction;
    QAction *m_showNextFile;
    QAction *m_showPrevFile;
    QAction *m_showNextHunk;
    QAction *m_showPrevHunk;
    QAction *m_reload;
    QAction *m_fullContext;

Q_SIGNALS:
    void showCommitInfoChanged(bool);
    void jumpToNextFile();
    void jumpToPrevFile();
    void jumpToNextHunk();
    void jumpToPrevHunk();
    void reload();
    void showWithFullContextChanged(bool);
};

static void syncScroll(QPlainTextEdit *src, QPlainTextEdit *tgt)
{
    int srcValue = src->verticalScrollBar()->value();
    const QTextBlock srcBlock = src->document()->findBlockByLineNumber(srcValue);
    QTextBlock tgtBlock = tgt->document()->findBlockByLineNumber(srcValue);
    if (srcBlock.blockNumber() == tgtBlock.blockNumber()) {
        tgt->verticalScrollBar()->setValue(srcValue);
    } else {
        tgtBlock = tgt->document()->findBlockByNumber(srcBlock.blockNumber());
        tgt->verticalScrollBar()->setValue(tgtBlock.firstLineNumber());
    }
}

DiffWidget::DiffWidget(DiffParams p, QWidget *parent)
    : QWidget(parent)
    , m_left(new DiffEditor(p.flags, this))
    , m_right(new DiffEditor(p.flags, this))
    , m_commitInfo(new CommitInfoView(this))
    , m_toolbar(new Toolbar(this))
    , m_params(p)
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins({});
    layout->addWidget(m_toolbar);
    layout->addWidget(m_commitInfo);
    auto diffLayout = new QHBoxLayout;
    diffLayout->setContentsMargins({});
    diffLayout->addWidget(m_left);
    diffLayout->addWidget(m_right);
    layout->addLayout(diffLayout);

    leftHl = new DiffSyntaxHighlighter(m_left->document(), this);
    rightHl = new DiffSyntaxHighlighter(m_right->document(), this);
    leftHl->setTheme(KTextEditor::Editor::instance()->theme());
    rightHl->setTheme(KTextEditor::Editor::instance()->theme());

    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, [this] {
        auto currentThemeName = leftHl->theme().name();
        auto newTheme = KTextEditor::Editor::instance()->theme();
        if (currentThemeName != newTheme.name()) {
            leftHl->setTheme(newTheme);
            rightHl->setTheme(newTheme);
            leftHl->rehighlight();
            rightHl->rehighlight();
        }
    });

    connect(m_left->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        if (m_stopScrollSync) {
            return;
        }
        m_stopScrollSync = true;
        syncScroll(m_left, m_right);
        m_stopScrollSync = false;
    });
    connect(m_right->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        if (m_stopScrollSync) {
            return;
        }
        m_stopScrollSync = true;
        syncScroll(m_right, m_left);
        m_stopScrollSync = false;
    });

    for (DiffEditor *e : {m_left, m_right}) {
        connect(e, &DiffEditor::switchStyle, this, &DiffWidget::handleStyleChange);
        connect(e, &DiffEditor::actionTriggered, this, &DiffWidget::handleStageUnstage);
    }

    // Connect both left and right editors to open line requests
    connect(m_left, &DiffEditor::openLineNumARequested, this, [this, p](int line, int columnNumber) {
        Q_EMIT openFileRequested(p.srcFile, line, columnNumber);
    });
    connect(m_left, &DiffEditor::openLineNumBRequested, this, [this, p](int line, int columnNumber) {
        Q_EMIT openFileRequested(!p.destFile.isEmpty() ? p.destFile : p.srcFile, line, columnNumber);
    });
    connect(m_right, &DiffEditor::openLineNumARequested, this, [this, p](int line, int columnNumber) {
        Q_EMIT openFileRequested(!p.destFile.isEmpty() ? p.destFile : p.srcFile, line, columnNumber);
    });
    connect(m_right, &DiffEditor::openLineNumBRequested, this, [this, p](int line, int columnNumber) {
        Q_EMIT openFileRequested(!p.destFile.isEmpty() ? p.destFile : p.srcFile, line, columnNumber);
    });

    m_commitInfo->hide();
    m_commitInfo->setWordWrapMode(QTextOption::WordWrap);
    m_commitInfo->setFont(Utils::editorFont());
    m_commitInfo->setReadOnly(true);
    m_commitInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commitInfo->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_commitInfo->setMaximumHeight(250);

    connect(m_toolbar, &Toolbar::showCommitInfoChanged, this, [this](bool v) {
        m_commitInfo->setVisible(v);
        KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
        cgGeneral.writeEntry("DiffWidget Show Commit Info", v);
    });
    connect(m_toolbar, &Toolbar::jumpToNextFile, this, &DiffWidget::jumpToNextFile);
    connect(m_toolbar, &Toolbar::jumpToPrevFile, this, &DiffWidget::jumpToPrevFile);
    connect(m_toolbar, &Toolbar::jumpToNextHunk, this, &DiffWidget::jumpToNextHunk);
    connect(m_toolbar, &Toolbar::jumpToPrevHunk, this, &DiffWidget::jumpToPrevHunk);
    connect(m_toolbar, &Toolbar::reload, this, &DiffWidget::runGitDiff);
    connect(m_toolbar, &Toolbar::showWithFullContextChanged, this, &DiffWidget::showWithFullContextChanged);

    const int iconSize = style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, this);
    m_toolbar->setIconSize(QSize(iconSize, iconSize));

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));

    // DiffStyle::SideBySide defaults
    m_left->setOpenLineNumAEnabled(m_params.flags.testFlag(DiffParams::Flag::ShowEditLeftSide));
    m_right->setOpenLineNumAEnabled(m_params.flags.testFlag(DiffParams::Flag::ShowEditRightSide));

    handleStyleChange(cgGeneral.readEntry("Diff Show Style", (int)SideBySide));

    // clear, after handleStyleChange there might be "no differences found" text
    m_left->clear();
    m_right->clear();
}

DiffWidget::~DiffWidget()
{
    // if there are any living processes, disconnect them now before we get destroyed
    for (QObject *child : children()) {
        auto *p = qobject_cast<QProcess *>(child);
        if (p) {
            disconnect(p, nullptr, nullptr, nullptr);
        }
    }
}

void DiffWidget::showEvent(QShowEvent *e)
{
    if (!m_blockShowEvent && m_params.flags & DiffParams::ReloadOnShow) {
        runGitDiff();
    }

    QWidget::showEvent(e);
}

void DiffWidget::handleStyleChange(int newStyle)
{
    if (newStyle == m_style) {
        return;
    }
    m_style = (DiffStyle)newStyle;
    const QByteArray diff = m_rawDiff;
    const DiffParams params = m_params;
    clearData();
    m_params = params;

    if (m_style == SideBySide) {
        m_left->setVisible(true);
        m_left->setOpenLineNumAEnabled(m_params.flags.testFlag(DiffParams::Flag::ShowEditLeftSide));
        m_left->setOpenLineNumBEnabled(false);
        m_right->setVisible(true);
        m_right->setOpenLineNumAEnabled(m_params.flags.testFlag(DiffParams::Flag::ShowEditRightSide));
        m_right->setOpenLineNumBEnabled(false);
        openDiff(diff);
    } else if (m_style == Unified) {
        m_left->setVisible(true);
        m_left->setOpenLineNumAEnabled(m_params.flags.testFlag(DiffParams::Flag::ShowEditLeftSide));
        m_left->setOpenLineNumBEnabled(m_params.flags.testFlag(DiffParams::Flag::ShowEditRightSide));
        m_right->setVisible(false);
        openDiff(diff);
    } else if (m_style == Raw) {
        m_left->setVisible(true);
        m_left->setOpenLineNumAEnabled(false);
        m_left->setOpenLineNumBEnabled(false);
        m_right->setVisible(false);
        openDiff(diff);
    } else {
        qWarning("Unexpected diff style value: %d", newStyle);
        Q_UNREACHABLE();
    }

    if (sender() && (sender() == m_left || sender() == m_right)) {
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
        cgGeneral.writeEntry("Diff Show Style", (int)m_style);
    }
}

void DiffWidget::handleStageUnstage(DiffEditor *e, int startLine, int endLine, int actionType, DiffParams::Flag f)
{
    if (m_style == SideBySide) {
        handleStageUnstage_sideBySide(e, startLine, endLine, actionType, f);
    } else if (m_style == Unified) {
        handleStageUnstage_unified(startLine, endLine, actionType, f);
    } else if (m_style == Raw) {
        handleStageUnstage_raw(startLine, endLine, actionType, f);
    }
}

void DiffWidget::handleStageUnstage_unified(int startLine, int endLine, int actionType, DiffParams::Flag f)
{
    handleStageUnstage_sideBySide(m_left, startLine, endLine, actionType, f);
}

void DiffWidget::handleStageUnstage_sideBySide(DiffEditor *e, int startLine, int endLine, int actionType, DiffParams::Flag flags)
{
    bool added = e == m_right;
    int diffLine = -1;
    if (actionType == DiffEditor::Line) {
        for (DiffWidget::ViewLineToDiffLine vToD : std::as_const(m_lineToRawDiffLine)) {
            if (vToD.line == startLine && vToD.added == added) {
                diffLine = vToD.diffLine;
                break;
            }
        }

        // If the selection by user isn't exact i.e.,
        // the start line isn't a changed line but just a context line
        if (diffLine == -1) {
            for (DiffWidget::ViewLineToDiffLine vToD : std::as_const(m_lineToRawDiffLine)) {
                // find the closes line to start line
                if (vToD.line > startLine && vToD.added == added) {
                    // check if the found line is in selection range
                    if (vToD.line <= endLine) {
                        // adjust startline
                        startLine = vToD.line;
                        diffLine = vToD.diffLine;
                    }
                    break;
                }
            }
        }

    } else if (actionType == DiffEditor::Hunk) {
        // find the first hunk smaller than/equal this line
        for (auto it = m_lineToDiffHunkLine.crbegin(); it != m_lineToDiffHunkLine.crend(); ++it) {
            if (it->line <= startLine) {
                diffLine = it->diffLine;
                break;
            }
        }
    }

    if (diffLine == -1) {
        // Nothing found somehow
        return;
    }

    doStageUnStage(diffLine, diffLine + (endLine - startLine), actionType, flags);
}

void DiffWidget::handleStageUnstage_raw(int startLine, int endLine, int actionType, DiffParams::Flag flags)
{
    doStageUnStage(startLine, endLine + (endLine - startLine), actionType, flags);
}

void DiffWidget::doStageUnStage(int startLine, int endLine, int actionType, DiffParams::Flag flags)
{
    VcsDiff d;
    const bool stageOrDiscard = flags == DiffParams::Flag::ShowDiscard || flags == DiffParams::Flag::ShowUnstage;
    const VcsDiff::DiffDirection dir = stageOrDiscard ? VcsDiff::Reverse : VcsDiff::Forward;
    d.setDiff(QString::fromUtf8(m_rawDiff));
    const VcsDiff x = actionType == DiffEditor::Line ? d.subDiff(startLine, startLine + (endLine - startLine), dir) : d.subDiffHunk(startLine, dir);

    if (flags & DiffParams::Flag::ShowStage) {
        applyDiff(x.diff(), ApplyFlags::None);
    } else if (flags & DiffParams::ShowUnstage) {
        applyDiff(x.diff(), ApplyFlags::Staged);
    } else if (flags & DiffParams::ShowDiscard) {
        applyDiff(x.diff(), ApplyFlags::Discard);
    }
}

void DiffWidget::applyDiff(const QString &diff, ApplyFlags flags)
{
    //     const QString diff = getDiff(v, flags & Hunk, flags & (Staged | Discard));
    if (diff.isEmpty()) {
        return;
    }

    auto *file = new QTemporaryFile(this);
    if (!file->open()) {
        //         sendMessage(i18n("Failed to stage selection"), true);
        return;
    }
    file->write(diff.toUtf8());
    file->close();

    Q_ASSERT(!m_params.workingDir.isEmpty());
    auto *git = new QProcess(this);
    QStringList args;
    if (flags & Discard) {
        args = QStringList{QStringLiteral("apply"), file->fileName()};
    } else {
        args = QStringList{QStringLiteral("apply"), QStringLiteral("--index"), QStringLiteral("--cached"), file->fileName()};
    }
    if (!setupGitProcess(*git, m_params.workingDir, args)) {
        gitNotFoundMessage();
        return;
    }

    connect(git, &QProcess::finished, this, [this, git, file](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            onError(git->readAllStandardError(), git->exitCode());
        } else {
            runGitDiff();
            if (m_params.updateStatusCallback) {
                m_params.updateStatusCallback();
            }
        }
        delete file;
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void DiffWidget::runGitDiff()
{
    const QStringList arguments = m_params.arguments;
    const QString workingDir = m_params.workingDir;
    if (workingDir.isEmpty() || arguments.isEmpty()) {
        return;
    }
    m_blockShowEvent = true;

    const int lf = m_left->firstVisibleBlockNumber();
    const int rf = m_right->firstVisibleBlockNumber();

    auto *git = new QProcess(this);
    if (!setupGitProcess(*git, workingDir, arguments)) {
        gitNotFoundMessage();
        return;
    }
    connect(git, &QProcess::finished, this, [this, git, lf, rf](int, QProcess::ExitStatus) {
        const DiffParams params = m_params;
        clearData();
        m_params = params;
        const QByteArray out = git->readAllStandardOutput();
        const QByteArray err = git->readAllStandardError();
        if (!err.isEmpty()) {
            onError(err, git->exitCode());
        }

        if (!out.isEmpty()) {
            openDiff(out);
            QMetaObject::invokeMethod(
                this,
                [this, lf, rf] {
                    m_left->scrollToBlock(lf);
                    m_right->scrollToBlock(rf);
                },
                Qt::QueuedConnection);
        }
        m_blockShowEvent = false;
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void DiffWidget::clearData()
{
    m_left->clearData();
    m_right->clearData();
    m_rawDiff.clear();
    m_lineToRawDiffLine.clear();
    m_lineToDiffHunkLine.clear();
    m_params.clear();
    m_linesWithFileName.clear();
    m_commitInfo->clear();
    m_commitInfo->hide();
}

QStringList DiffWidget::diffDocsGitArgs(KTextEditor::Document *l, KTextEditor::Document *r)
{
    const QString left = l->url().toLocalFile();
    const QString right = r->url().toLocalFile();
    return {QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-index"), left, right};
}

void DiffWidget::showWithFullContextChanged(bool fullContextEnabled)
{
    if (m_params.arguments.size() < 1) {
        return;
    }

    if (fullContextEnabled) {
        m_paramsNoFullContext = m_params;
        int idx = m_params.arguments.indexOf(QLatin1String("--"));
        if (idx != -1) {
            m_params.arguments.insert(idx, QLatin1String("-U5000"));
        } else if (m_params.arguments.size() == 1) {
            m_params.arguments << QLatin1String("-U5000");
        } else {
            m_params.arguments.insert(2, QLatin1String("-U5000"));
        }
    } else {
        m_params = m_paramsNoFullContext;
    }

    const int lineNo = m_left->firstVisibleLineNumber();
    runGitDiff();

    // After the diff runs and we show the result, try to take the user back to where he was
    QPointer<QTimer> delayedSlotTrigger = new QTimer(this);
    delayedSlotTrigger->setSingleShot(true);
    delayedSlotTrigger->setInterval(10);
    delayedSlotTrigger->callOnTimeout(this, [this, lineNo, delayedSlotTrigger, fullContextEnabled] {
        if (delayedSlotTrigger) {
            m_left->scrollToLineNumber(lineNo);
            m_toolbar->setShowFullContext(fullContextEnabled);
            delete delayedSlotTrigger;
        }
    });
    connect(m_left, &QPlainTextEdit::textChanged, delayedSlotTrigger, qOverload<>(&QTimer::start));
}

void DiffWidget::diffDocs(KTextEditor::Document *l, KTextEditor::Document *r)
{
    clearData();
    const auto &repo = KTextEditor::Editor::instance()->repository();
    const auto def = repo.definitionForMimeType(l->mimeType());
    if (l->mimeType() == r->mimeType()) {
        leftHl->setDefinition(def);
        rightHl->setDefinition(def);
    } else {
        leftHl->setDefinition(def);
        rightHl->setDefinition(repo.definitionForMimeType(r->mimeType()));
    }

    QProcess git;
    const QStringList args = diffDocsGitArgs(l, r);
    if (!setupGitProcess(git, qApp->applicationDirPath(), args)) {
        gitNotFoundMessage();
        return;
    }

    m_params.arguments = args;
    m_params.flags.setFlag(DiffParams::ReloadOnShow);
    m_params.workingDir = git.workingDirectory();
    runGitDiff();
}

static void balanceHunkLines(QStringList &left, QStringList &right, int &lineA, int &lineB, std::vector<int> &lineNosA, std::vector<int> &lineNosB)
{
    while (left.size() < right.size()) {
        lineA++;
        left.push_back({});
        lineNosA.push_back(-1);
    }
    while (right.size() < left.size()) {
        lineB++;
        right.push_back({});
        lineNosB.push_back(-1);
    }
}

struct ChangePair {
    Change left;
    Change right;
};

static ChangePair inlineDiff(QStringView l, QStringView r)
{
    auto sitl = l.begin();
    auto sitr = r.begin();
    while (sitl != l.end() || sitr != r.end()) {
        if (*sitl != *sitr) {
            break;
        }
        ++sitl;
        ++sitr;
    }

    int lStart = std::distance(l.begin(), sitl);
    int rStart = std::distance(r.begin(), sitr);

    auto eitl = l.rbegin();
    auto eitr = r.rbegin();
    int lcount = l.size();
    int rcount = r.size();
    while (eitl != l.rend() || eitr != r.rend()) {
        if (*eitl != *eitr) {
            break;
        }
        // do not allow overlap
        if (lcount == lStart || rcount == rStart) {
            break;
        }
        --lcount;
        --rcount;
        ++eitl;
        ++eitr;
    }

    int lEnd = l.size() - std::distance(l.rbegin(), eitl);
    int rEnd = r.size() - std::distance(r.rbegin(), eitr);

    Change cl{.pos = lStart, .len = lEnd - lStart};
    Change cr{.pos = rStart, .len = rEnd - rStart};
    return {.left = cl, .right = cr};
}

// Struct representing a changed line in hunk
struct HunkChangedLine {
    HunkChangedLine(const QString &ln, int lineNum, bool isAdd)
        : line(ln)
        , lineNo(lineNum)
        , added(isAdd)
    {
    }
    // Line Text
    const QString line;
    // Line Number in text editor (not hunk)
    int lineNo;
    // Which portion of the line changed (len, pos)
    bool added;
    Change c = {.pos = -1, .len = -1};
};
using HunkChangedLines = std::vector<HunkChangedLine>;

static void markInlineDiffs(HunkChangedLines &hunkChangedLinesA,
                            HunkChangedLines &hunkChangedLinesB,
                            std::vector<LineHighlight> &leftHlts,
                            std::vector<LineHighlight> &rightHlts)
{
    if (hunkChangedLinesA.size() != hunkChangedLinesB.size()) {
        hunkChangedLinesA.clear();
        hunkChangedLinesB.clear();
        return;
    }
    const int size = (int)hunkChangedLinesA.size();
    for (int i = 0; i < size; ++i) {
        const auto [leftChange, rightChange] = inlineDiff(hunkChangedLinesA.at(i).line, hunkChangedLinesB.at(i).line);
        hunkChangedLinesA[i].c = leftChange;
        hunkChangedLinesB[i].c = rightChange;
    }

    auto addHighlights = [](HunkChangedLines &hunkChangedLines, std::vector<LineHighlight> &hlts) {
        for (int i = 0; i < (int)hunkChangedLines.size(); ++i) {
            auto &change = hunkChangedLines[i];
            for (int j = (int)hlts.size() - 1; j >= 0; --j) {
                if (hlts.at(j).line == change.lineNo && hlts.at(j).added == change.added) {
                    hlts[j].changes.push_back(change.c);
                    break;
                } else if (hlts.at(j).line < change.lineNo) {
                    break;
                }
            }
        }
    };

    addHighlights(hunkChangedLinesA, leftHlts);
    addHighlights(hunkChangedLinesB, rightHlts);
    hunkChangedLinesA.clear();
    hunkChangedLinesB.clear();
}

static std::vector<KSyntaxHighlighting::Definition> defsForFileExtensions(const QSet<QString> &fileExtensions)
{
    const auto &repo = KTextEditor::Editor::instance()->repository();
    if (fileExtensions.size() == 1) {
        const QString name = QStringLiteral("a.") + *fileExtensions.begin();
        return {repo.definitionForFileName(name)};
    }

    std::vector<KSyntaxHighlighting::Definition> defs;
    for (const auto &ext : fileExtensions) {
        const QString name = QStringLiteral("a.") + ext;
        defs.push_back(repo.definitionForFileName(name));
    }
    QSet<QString> seenDefs;
    std::vector<KSyntaxHighlighting::Definition> uniqueDefs;
    for (const auto &def : defs) {
        if (!seenDefs.contains(def.name())) {
            uniqueDefs.push_back(def);
            seenDefs.insert(def.name());
        }
    }
    if (uniqueDefs.size() == 1) {
        return uniqueDefs;
    } else {
        return {repo.definitionForName(QStringLiteral("None"))};
    }
}

void DiffWidget::parseAndShowDiff(const QByteArray &raw)
{
    //     printf("show diff:\n%s\n================================", raw.constData());
    const QStringList text = QString::fromUtf8(raw).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    static const QRegularExpression HUNK_HEADER_RE(QStringLiteral("^@@ -([0-9,]+) \\+([0-9,]+) @@(.*)"));
    static const QRegularExpression DIFF_FILENAME_RE(QStringLiteral("^[-+]{3} [ab]/(.*)"));

    // Actual lines that will get added to the text editor
    QStringList left;
    QStringList right;

    // Highlighting data for modified lines
    std::vector<LineHighlight> leftHlts;
    std::vector<LineHighlight> rightHlts;
    // QList<QPair<int, HunkData>> hunkDatas; // lineNo => HunkData

    // Line numbers that will be shown in the editor
    std::vector<int> lineNumsA;
    std::vector<int> lineNumsB;

    // Changed lines of hunk, used to determine differences between two lines
    HunkChangedLines hunkChangedLinesA;
    HunkChangedLines hunkChangedLinesB;

    QSet<QString> fileExtensions;
    QString srcFile;
    QString tgtFile;

    // viewLine => rawDiffLine
    std::vector<ViewLineToDiffLine> lineToRawDiffLine;
    // for Folding/stage/unstage hunk
    std::vector<ViewLineToDiffLine> linesWithHunkHeading;
    std::vector<int> linesWithFileName;

    int maxLineNoFound = 0;
    int lineA = 0;
    int lineB = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        auto match = DIFF_FILENAME_RE.match(line);
        if ((match.hasMatch() || line == QStringLiteral("--- /dev/null")) && i + 1 < text.size()) {
            srcFile = match.hasMatch() ? match.captured(1) : QString();
            if (!srcFile.isEmpty()) {
                fileExtensions.insert(QFileInfo(srcFile).suffix());
            }
            auto match = DIFF_FILENAME_RE.match(text.at(i + 1));

            if (match.hasMatch() || text.at(i + 1) == QStringLiteral("--- /dev/null")) {
                tgtFile = match.hasMatch() ? match.captured(1) : QString();
                if (!tgtFile.isEmpty()) {
                    fileExtensions.insert(QFileInfo(tgtFile).suffix());
                }
            }
            i++;

            if (m_params.flags.testFlag(DiffParams::ShowFileName)) {
                if (srcFile.isEmpty() && !tgtFile.isEmpty()) {
                    left.append(QLatin1String("---"));
                    right.append(i18n("New file %1", Utils::fileNameFromPath(tgtFile)));
                } else if (!srcFile.isEmpty() && tgtFile.isEmpty()) {
                    left.append(i18n("Deleted file %1", Utils::fileNameFromPath(srcFile)));
                    right.append(QLatin1String("+++"));
                } else {
                    left.append(Utils::fileNameFromPath(srcFile));
                    right.append(Utils::fileNameFromPath(tgtFile));
                }
                Q_ASSERT(left.size() == right.size() && lineA == lineB);
                linesWithFileName.push_back(lineA);
                lineNumsA.push_back(-1);
                lineNumsB.push_back(-1);
                lineA++;
                lineB++;
            }
            continue;
        }

        match = HUNK_HEADER_RE.match(line);
        if (!match.hasMatch())
            continue;

        const DiffRange oldRange = parseRange(match.captured(1));
        const DiffRange newRange = parseRange(match.captured(2));
        const QString headingLeft = QStringLiteral("@@ ") + match.captured(1) + match.captured(3) /* + QStringLiteral(" ") + srcFile*/;
        const QString headingRight = QStringLiteral("@@ ") + match.captured(2) + match.captured(3) /* + QStringLiteral(" ") + tgtFile*/;

        lineNumsA.push_back(-1);
        lineNumsB.push_back(-1);
        left.push_back(headingLeft);
        right.push_back(headingRight);
        linesWithHunkHeading.push_back({lineA, i, /*unused*/ false});
        lineA++;
        lineB++;

        hunkChangedLinesA.clear();
        hunkChangedLinesB.clear();

        int srcLine = oldRange.line;
        const int oldCount = oldRange.count;

        int tgtLine = newRange.line;
        const int newCount = newRange.count;
        maxLineNoFound = qMax(qMax(srcLine + oldCount, tgtLine + newCount), maxLineNoFound);

        for (int j = i + 1; j < text.size(); j++) {
            QString l = text.at(j);
            if (l.startsWith(QLatin1Char(' '))) {
                // Insert dummy lines when left/right are unequal
                balanceHunkLines(left, right, lineA, lineB, lineNumsA, lineNumsB);
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);

                l = l.mid(1);
                left.push_back(l);
                right.push_back(l);
                //                 lineNo++;
                lineNumsA.push_back(srcLine++);
                lineNumsB.push_back(tgtLine++);
                lineA++;
                lineB++;
            } else if (l.startsWith(QLatin1Char('+'))) {
                // qDebug("- line");
                l = l.mid(1);
                LineHighlight h;
                h.line = lineB;
                h.added = true;
                h.changes.push_back({0, Change::FullBlock});
                rightHlts.push_back(h);
                lineNumsB.push_back(tgtLine++);
                lineToRawDiffLine.push_back({lineB, j, true});
                right.push_back(l);

                hunkChangedLinesB.emplace_back(l, lineB, h.added);

                //                 lineNo++;
                lineB++;
            } else if (l.startsWith(QLatin1Char('-'))) {
                l = l.mid(1);
                // qDebug("+ line: %ls", qUtf16Printable(l));
                LineHighlight h;
                h.line = lineA;
                h.added = false;
                h.changes.push_back({0, Change::FullBlock});

                leftHlts.push_back(h);
                lineNumsA.push_back(srcLine++);
                left.push_back(l);
                hunkChangedLinesA.emplace_back(l, lineA, h.added);
                lineToRawDiffLine.push_back({lineA, j, false});

                lineA++;
            } else if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                i = j - 1;

                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);

                // add line number for current line
                lineNumsA.push_back(-1);
                lineNumsB.push_back(-1);

                // add new line
                left.push_back(QString());
                right.push_back(QString());
                lineA += 1;
                lineB += 1;
                break;
            } else if (l.startsWith(QStringLiteral("diff --git "))) {
                balanceHunkLines(left, right, lineA, lineB, lineNumsA, lineNumsB);
                // Start of a new file
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);
                // add new line
                lineNumsA.push_back(-1);
                lineNumsB.push_back(-1);
                left.push_back(QString());
                right.push_back(QString());
                lineA += 1;
                lineB += 1;
                break;
            }
            if (j + 1 >= text.size()) {
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);
                i = j; // ensure outer loop also exits after this
            }
        }
    }

    balanceHunkLines(left, right, lineA, lineB, lineNumsA, lineNumsB);

    QString leftText = left.join(QLatin1Char('\n'));
    QString rightText = right.join(QLatin1Char('\n'));

    Q_ASSERT(lineA == lineB && left.size() == right.size() && lineNumsA.size() == lineNumsB.size());

    m_left->appendData(leftHlts);
    m_right->appendData(rightHlts);
    m_left->appendPlainText(leftText);
    m_right->appendPlainText(rightText);
    m_left->setLineNumberData(lineNumsA, {}, maxLineNoFound);
    m_right->setLineNumberData(lineNumsB, {}, maxLineNoFound);
    m_lineToDiffHunkLine.insert(m_lineToDiffHunkLine.end(), linesWithHunkHeading.begin(), linesWithHunkHeading.end());
    m_lineToRawDiffLine.insert(m_lineToRawDiffLine.end(), lineToRawDiffLine.begin(), lineToRawDiffLine.end());
    m_linesWithFileName.insert(m_linesWithFileName.end(), linesWithFileName.begin(), linesWithFileName.end());

    const auto defs = defsForFileExtensions(fileExtensions);
    leftHl->setDefinition(defs.front());
    rightHl->setDefinition(defs.front());
}

void DiffWidget::parseAndShowDiffUnified(const QByteArray &raw)
{
    //     printf("show diff:\n%s\n================================", raw.constData());
    const QStringList text = QString::fromUtf8(raw).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    static const QRegularExpression HUNK_HEADER_RE(QStringLiteral("^@@ -([0-9,]+) \\+([0-9,]+) @@(.*)"));
    static const QRegularExpression DIFF_FILENAME_RE(QStringLiteral("^[-+]{3} [ab]/(.*)"));

    // Actual lines that will get added to the text editor
    QStringList lines;

    // Highlighting data for modified lines
    std::vector<LineHighlight> hlts;

    // Line numbers that will be shown in the editor
    std::vector<int> lineNumsA;
    std::vector<int> lineNumsB;

    // Changed lines of hunk, used to determine differences between two lines
    HunkChangedLines hunkChangedLinesA;
    HunkChangedLines hunkChangedLinesB;

    // viewLine => rawDiffLine
    std::vector<ViewLineToDiffLine> lineToRawDiffLine;
    // for Folding/stage/unstage hunk
    std::vector<ViewLineToDiffLine> linesWithHunkHeading;
    // Lines containing filename
    std::vector<int> linesWithFileName;

    QSet<QString> fileExtensions;
    QString srcFile;
    QString tgtFile;

    int maxLineNoFound = 0;
    int lineNo = 0;

    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        auto match = DIFF_FILENAME_RE.match(line);
        if ((match.hasMatch() || line == QStringLiteral("--- /dev/null")) && i + 1 < text.size()) {
            srcFile = match.hasMatch() ? match.captured(1) : QString();
            if (!srcFile.isEmpty()) {
                fileExtensions.insert(QFileInfo(srcFile).suffix());
            }
            auto match = DIFF_FILENAME_RE.match(text.at(i + 1));

            if (match.hasMatch() || text.at(i + 1) == QStringLiteral("--- /dev/null")) {
                tgtFile = match.hasMatch() ? match.captured(1) : QString();
                if (!tgtFile.isEmpty()) {
                    fileExtensions.insert(QFileInfo(tgtFile).suffix());
                }
            }
            i++;

            if (m_params.flags.testFlag(DiffParams::ShowFileName)) {
                if (srcFile.isEmpty() && !tgtFile.isEmpty()) {
                    lines.append(i18n("New file %1", Utils::fileNameFromPath(tgtFile)));
                } else if (!srcFile.isEmpty() && tgtFile.isEmpty()) {
                    lines.append(i18n("Deleted file %1", Utils::fileNameFromPath(srcFile)));
                } else if (!srcFile.isEmpty() && !tgtFile.isEmpty()) {
                    lines.append(QStringLiteral("%1 → %2").arg(Utils::fileNameFromPath(srcFile), Utils::fileNameFromPath(tgtFile)));
                }
                lineNumsA.push_back(-1);
                lineNumsB.push_back(-1);
                linesWithFileName.push_back(lineNo);
                lineNo++;
            }
            continue;
        }

        match = HUNK_HEADER_RE.match(line);
        if (!match.hasMatch()) {
            continue;
        }

        const DiffRange oldRange = parseRange(match.captured(1));
        const DiffRange newRange = parseRange(match.captured(2));
        //         const QString headingLeft = QStringLiteral("@@ ") + match.captured(1) + match.captured(3) /* + QStringLiteral(" ") + srcFile*/;
        //         const QString headingRight = QStringLiteral("@@ ") + match.captured(2) + match.captured(3) /* + QStringLiteral(" ") + tgtFile*/;

        lines.push_back(line);
        lineNumsA.push_back(-1);
        lineNumsB.push_back(-1);
        linesWithHunkHeading.push_back({lineNo, i, false});
        lineNo++;

        hunkChangedLinesA.clear();
        hunkChangedLinesB.clear();

        int srcLine = oldRange.line;
        const int oldCount = oldRange.count;

        int tgtLine = newRange.line;
        const int newCount = newRange.count;
        maxLineNoFound = qMax(qMax(srcLine + oldCount, tgtLine + newCount), maxLineNoFound);

        for (int j = i + 1; j < text.size(); j++) {
            QString l = text.at(j);
            if (l.startsWith(QLatin1Char(' '))) {
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);

                l = l.mid(1);
                lines.append(l);
                lineNumsA.push_back(srcLine++);
                lineNumsB.push_back(tgtLine++);
                lineNo++;
                //                 lineB++;
            } else if (l.startsWith(QLatin1Char('+'))) {
                // qDebug("- line");
                l = l.mid(1);
                LineHighlight h;
                h.line = lineNo;
                h.added = true;
                h.changes.push_back({0, Change::FullBlock});
                lines.append(l);
                hlts.push_back(h);
                lineNumsA.push_back(-1);
                lineNumsB.push_back(tgtLine++);
                lineToRawDiffLine.push_back({lineNo, j, false});

                hunkChangedLinesB.emplace_back(l, lineNo, h.added);
                lineNo++;
            } else if (l.startsWith(QLatin1Char('-'))) {
                l = l.mid(1);
                LineHighlight h;
                h.line = lineNo;
                h.added = false;
                h.changes.push_back({0, Change::FullBlock});

                hlts.push_back(h);
                lineNumsA.push_back(srcLine++);
                lineNumsB.push_back(-1);
                lines.append(l);
                hunkChangedLinesA.emplace_back(l, lineNo, h.added);
                lineToRawDiffLine.push_back({lineNo, j, false});

                lineNo++;
            } else if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                i = j - 1;
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);
                // add line number for current line
                lineNumsA.push_back(-1);
                lineNumsB.push_back(-1);

                // add new line
                lines.append(QString());
                lineNo += 1;
                break;
            } else if (l.startsWith(QStringLiteral("diff --git "))) {
                // Start of a new file
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);
                // add new line
                lines.append(QString());
                lineNumsA.push_back(-1);
                lineNumsB.push_back(-1);
                lineNo += 1;
                break;
            }
            if (j + 1 >= text.size()) {
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);
                i = j; // ensure outer loop also exits after this
            }
        }
    }

    m_left->appendData(hlts);
    m_left->appendPlainText(lines.join(QLatin1Char('\n')));
    m_left->setLineNumberData(lineNumsA, lineNumsB, maxLineNoFound);
    m_lineToDiffHunkLine.insert(m_lineToDiffHunkLine.end(), linesWithHunkHeading.begin(), linesWithHunkHeading.end());
    m_lineToRawDiffLine.insert(m_lineToRawDiffLine.end(), lineToRawDiffLine.begin(), lineToRawDiffLine.end());
    m_linesWithFileName.insert(m_linesWithFileName.end(), linesWithFileName.begin(), linesWithFileName.end());

    const std::vector<KSyntaxHighlighting::Definition> defs = defsForFileExtensions(fileExtensions);
    leftHl->setDefinition(defs.front());
}

static QString commitInfoFromDiff(const QByteArray &raw)
{
    if (!raw.startsWith("commit ")) {
        return {};
    }
    int commitEnd = raw.indexOf("diff --git");
    if (commitEnd == -1) {
        return {};
    }
    return QString::fromUtf8(raw.mid(0, commitEnd).trimmed());
}

void DiffWidget::openDiff(const QByteArray &raw)
{
    if ((m_params.flags & DiffParams::ShowCommitInfo) && m_style != DiffStyle::Raw) {
        m_toolbar->setShowCommitActionVisible(true);
        m_commitInfo->setPlainText(commitInfoFromDiff(raw));
        if (m_toolbar->showCommitInfo()) {
            m_commitInfo->show();
        }
    } else {
        m_commitInfo->hide();
        m_toolbar->setShowCommitActionVisible(false);
    }

    m_toolbar->setShowFullContextVisible(m_params.flags & DiffParams::ShowFullContext);

    // Fallback to raw mode if parsing fails
    auto fallback = [&] {
        handleStyleChange(Raw);
        m_left->setPlainText(QString::fromUtf8(raw));
        leftHl->setDefinition(KTextEditor::Editor::instance()->repository().definitionForName(QStringLiteral("Diff")));
    };

    if (m_style == SideBySide) {
        parseAndShowDiff(raw);
        if (m_left->document()->isEmpty() && m_right->document()->isEmpty() && !raw.isEmpty()) {
            fallback();
        }
    } else if (m_style == Unified) {
        parseAndShowDiffUnified(raw);
        if (m_left->document()->isEmpty() && !raw.isEmpty()) {
            fallback();
        }
    } else if (m_style == Raw) {
        m_left->setPlainText(QString::fromUtf8(raw));
        leftHl->setDefinition(KTextEditor::Editor::instance()->repository().definitionForName(QStringLiteral("Diff")));
    }
    m_rawDiff = raw;

    if (m_rawDiff.isEmpty()) {
        m_left->setPlainText(i18n("No differences found"));
        m_right->setPlainText(i18n("No differences found"));
    }

    QMetaObject::invokeMethod(
        this,
        [this] {
            m_left->verticalScrollBar()->setValue(0);
            m_right->verticalScrollBar()->setValue(0);
            m_blockShowEvent = false;
        },
        Qt::QueuedConnection);
}

void DiffWidget::onError(const QByteArray &error, int)
{
    if (!error.isEmpty()) {
        Utils::showMessage(QString::fromUtf8(error), gitIcon(), i18n("Diff"), MessageType::Warning);
    }
    //     printf("Got error: \n%s\n==============\n", error.constData());
}

bool DiffWidget::isHunk(const int line) const
{
    return std::any_of(m_lineToDiffHunkLine.begin(), m_lineToDiffHunkLine.end(), [line](const ViewLineToDiffLine l) {
        return l.line == line;
    });
}

int DiffWidget::hunkLineCount(int hunkLine)
{
    for (size_t i = 0; i < m_lineToDiffHunkLine.size(); ++i) {
        const auto h = m_lineToDiffHunkLine.at(i);
        if (h.line == hunkLine) {
            // last hunk?
            if (i + 1 >= m_lineToDiffHunkLine.size()) {
                return -1;
            }
            auto nextHunk = m_lineToDiffHunkLine.at(i + 1);
            auto count = nextHunk.line - h.line;
            count -= 1; // one separator line is ignored
            return count;
        }
    }

    return 0;
}

void DiffWidget::jumpToNextFile()
{
    const int block = m_left->firstVisibleBlockNumber();
    int nextFileLineNo = 0;
    for (int i : m_linesWithFileName) {
        if (i > block) {
            nextFileLineNo = i;
            break;
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(nextFileLineNo, true);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(nextFileLineNo, true);
    }
}

void DiffWidget::jumpToPrevFile()
{
    const int block = m_left->firstVisibleBlockNumber();
    int prevFileLineNo = 0;
    for (auto i = m_linesWithFileName.crbegin(); i != m_linesWithFileName.crend(); ++i) {
        if (*i < block) {
            prevFileLineNo = *i;
            break;
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(prevFileLineNo, true);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(prevFileLineNo, true);
    }
}

void DiffWidget::jumpToNextHunk()
{
    const bool fullContext = m_params.arguments.contains(u"-U5000");
    int nextHunkLineNo = 0;

    if (fullContext) {
        bool found = false;
        const int block = m_left->firstVisibleBlockNumber();
        int last = block;
        for (DiffWidget::ViewLineToDiffLine i : m_lineToRawDiffLine) {
            if (i.line > block && i.line != last + 1) {
                nextHunkLineNo = i.line;
                found = true;
                break;
            }
            last = i.line;
        }
        if (!found) {
            nextHunkLineNo = m_lineToRawDiffLine.front().line;
        }
    } else {
        const int block = m_left->firstVisibleBlockNumber();
        for (const DiffWidget::ViewLineToDiffLine &i : m_lineToDiffHunkLine) {
            if (i.line > block) {
                nextHunkLineNo = i.line;
                break;
            }
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(nextHunkLineNo, true);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(nextHunkLineNo, true);
    }
}

void DiffWidget::jumpToPrevHunk()
{
    const bool fullContext = m_params.arguments.contains(u"-U5000");
    int prevHunkLineNo = 0;
    if (fullContext) {
        bool found = false;
        const int block = m_left->firstVisibleBlockNumber();
        int last = block;
        for (DiffWidget::ViewLineToDiffLine i : m_lineToRawDiffLine) {
            if (i.line < block && i.line != last - 1) {
                prevHunkLineNo = i.line;
                break;
            }
            last = i.line;
        }
        if (!found) {
            prevHunkLineNo = m_lineToRawDiffLine.back().line;
        }
    } else {
        const int block = m_left->firstVisibleBlockNumber();
        for (auto i = m_lineToDiffHunkLine.crbegin(); i != m_lineToDiffHunkLine.crend(); ++i) {
            if (i->line < block) {
                prevHunkLineNo = i->line;
                break;
            }
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(prevHunkLineNo, true);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(prevHunkLineNo, true);
    }
}

#include "diffwidget.moc"
#include "moc_diffwidget.cpp"
