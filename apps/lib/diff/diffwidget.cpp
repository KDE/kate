/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "diffwidget.h"
#include "gitdiff.h"
#include "gitprocess.h"
#include "ktexteditor_utils.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMimeDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QSyntaxHighlighter>
#include <QTemporaryFile>
#include <QToolBar>
#include <QToolButton>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KTextEditor/Editor>

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

        KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), "General");
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
    }

    void setShowCommitActionVisible(bool vis)
    {
        if (m_showCommitInfoAction->isVisible() != vis) {
            m_showCommitInfoAction->setVisible(vis);
        }
    }

    bool showCommitInfo()
    {
        return m_showCommitInfoAction->isChecked();
    }

private:
    QAction *m_showCommitInfoAction;
    QAction *m_showNextFile;
    QAction *m_showPrevFile;
    QAction *m_showNextHunk;
    QAction *m_showPrevHunk;

Q_SIGNALS:
    void showCommitInfoChanged(bool);
    void jumpToNextFile();
    void jumpToPrevFile();
    void jumpToNextHunk();
    void jumpToPrevHunk();
};

DiffWidget::DiffWidget(DiffParams p, QWidget *parent)
    : QWidget(parent)
    , m_left(new DiffEditor(p.flags, this))
    , m_right(new DiffEditor(p.flags, this))
    , m_commitInfo(new QPlainTextEdit(this))
    , m_toolbar(new Toolbar(this))
    , m_params(p)
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins({});
    layout->addWidget(m_commitInfo);
    layout->addWidget(m_toolbar);
    auto diffLayout = new QHBoxLayout;
    diffLayout->setContentsMargins({});
    diffLayout->addWidget(m_left);
    diffLayout->addWidget(m_right);
    layout->addLayout(diffLayout);

    leftHl = new DiffSyntaxHighlighter(m_left->document(), this);
    rightHl = new DiffSyntaxHighlighter(m_right->document(), this);
    leftHl->setTheme(KTextEditor::Editor::instance()->theme());
    rightHl->setTheme(KTextEditor::Editor::instance()->theme());

    connect(m_left->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
        if (m_stopScrollSync) {
            return;
        }
        m_right->verticalScrollBar()->setValue(v);
    });
    connect(m_right->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
        if (m_stopScrollSync) {
            return;
        }
        m_left->verticalScrollBar()->setValue(v);
    });

    for (auto *e : {m_left, m_right}) {
        connect(e, &DiffEditor::switchStyle, this, &DiffWidget::handleStyleChange);
        connect(e, &DiffEditor::actionTriggered, this, &DiffWidget::handleStageUnstage);
    }

    m_commitInfo->hide();
    m_commitInfo->setWordWrapMode(QTextOption::WordWrap);
    m_commitInfo->setFont(Utils::editorFont());
    m_commitInfo->setReadOnly(true);
    m_commitInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commitInfo->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_commitInfo->setMaximumHeight(250);

    connect(m_toolbar, &Toolbar::showCommitInfoChanged, this, [this](bool v) {
        m_commitInfo->setVisible(v);
        KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), "General");
        cgGeneral.writeEntry("DiffWidget Show Commit Info", v);
    });
    connect(m_toolbar, &Toolbar::jumpToNextFile, this, &DiffWidget::jumpToNextFile);
    connect(m_toolbar, &Toolbar::jumpToPrevFile, this, &DiffWidget::jumpToPrevFile);
    connect(m_toolbar, &Toolbar::jumpToNextHunk, this, &DiffWidget::jumpToNextHunk);
    connect(m_toolbar, &Toolbar::jumpToPrevHunk, this, &DiffWidget::jumpToPrevHunk);

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");
    handleStyleChange(cgGeneral.readEntry("Diff Show Style", (int)SideBySide));
}

void DiffWidget::handleStyleChange(int newStyle)
{
    if (newStyle == m_style) {
        return;
    }
    m_style = (DiffStyle)newStyle;
    const auto diff = m_rawDiff;
    const auto params = m_params;
    clearData();
    m_params = params;

    if (m_style == SideBySide) {
        m_left->setVisible(true);
        m_right->setVisible(true);
        openDiff(diff);
    } else if (m_style == Unified) {
        m_left->setVisible(true);
        m_right->setVisible(false);
        openDiff(diff);
    } else if (m_style == Raw) {
        m_left->setVisible(true);
        m_right->setVisible(false);
        openDiff(diff);
    } else {
        qWarning() << "Unexpected diff style value: " << newStyle;
        Q_UNREACHABLE();
    }

    if (sender() && (sender() == m_left || sender() == m_right)) {
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, "General");
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
        for (auto vToD : std::as_const(m_lineToRawDiffLine)) {
            if (vToD.line == startLine && vToD.added == added) {
                diffLine = vToD.diffLine;
                break;
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
    const auto stageOrDiscard = flags == DiffParams::Flag::ShowDiscard || flags == DiffParams::Flag::ShowUnstage;
    const auto dir = stageOrDiscard ? VcsDiff::Reverse : VcsDiff::Forward;
    d.setDiff(QString::fromUtf8(m_rawDiff));
    const auto x = actionType == DiffEditor::Line ? d.subDiff(startLine, startLine + (endLine - startLine), dir) : d.subDiffHunk(startLine, dir);

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

    QTemporaryFile *file = new QTemporaryFile(this);
    if (!file->open()) {
        //         sendMessage(i18n("Failed to stage selection"), true);
        return;
    }
    file->write(diff.toUtf8());
    file->close();

    Q_ASSERT(!m_params.workingDir.isEmpty());
    QProcess *git = new QProcess(this);
    QStringList args;
    if (flags & Discard) {
        args = QStringList{QStringLiteral("apply"), file->fileName()};
    } else {
        args = QStringList{QStringLiteral("apply"), QStringLiteral("--index"), QStringLiteral("--cached"), file->fileName()};
    }
    setupGitProcess(*git, m_params.workingDir, args);

    connect(git, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            onError(git->readAllStandardError(), git->exitCode());
        } else {
            runGitDiff();
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

    auto leftState = m_left->saveState();
    auto rightState = m_right->saveState();

    QProcess *git = new QProcess(this);
    setupGitProcess(*git, workingDir, arguments);
    connect(git, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus es) {
        const auto params = m_params;
        clearData();
        m_params = params;
        m_left->setUpdatesEnabled(false);
        m_right->setUpdatesEnabled(false);
        if (es != QProcess::NormalExit || exitCode != 0) {
            onError(git->readAllStandardError(), git->exitCode());
        } else {
            openDiff(git->readAllStandardOutput());
            m_left->restoreState(leftState);
            m_right->restoreState(rightState);
        }
        m_left->setUpdatesEnabled(true);
        m_right->setUpdatesEnabled(true);
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

    const QString left = l->url().toLocalFile();
    const QString right = r->url().toLocalFile();

    QPointer<QProcess> git = new QProcess(this);
    setupGitProcess(*git, qApp->applicationDirPath(), {QStringLiteral("diff"), QStringLiteral("--no-index"), left, right});

    connect(git, &QProcess::readyReadStandardOutput, this, [this, git]() {
        onTextReceived(git->readAllStandardOutput());
    });
    connect(git, &QProcess::readyReadStandardError, this, [this, git]() {
        onError(git->readAllStandardError(), -1);
    });
    connect(git, &QProcess::finished, this, [this, git] {
        git->deleteLater();
        if (git->exitStatus() != QProcess::NormalExit) {
            onError(git->readAllStandardError(), git->exitCode());
        }
    });
    git->start();
}

static void balanceHunkLines(QStringList &left, QStringList &right, int &lineA, int &lineB, QVector<int> &lineNosA, QVector<int> &lineNosB)
{
    while (left.size() < right.size()) {
        lineA++;
        left.push_back({});
        lineNosA.append(-1);
    }
    while (right.size() < left.size()) {
        lineB++;
        right.push_back({});
        lineNosB.append(-1);
    }
}

static std::pair<Change, Change> inlineDiff(QStringView l, QStringView r)
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

    Change cl;
    cl.pos = lStart;
    cl.len = lEnd - lStart;
    Change cr;
    cr.pos = rStart;
    cr.len = rEnd - rStart;
    return {cl, cr};
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
    Change c = {-1, -1};
};
using HunkChangedLines = std::vector<HunkChangedLine>;

static void
markInlineDiffs(HunkChangedLines &hunkChangedLinesA, HunkChangedLines &hunkChangedLinesB, QVector<LineHighlight> &leftHlts, QVector<LineHighlight> &rightHlts)
{
    if (hunkChangedLinesA.size() != hunkChangedLinesB.size()) {
        hunkChangedLinesA.clear();
        hunkChangedLinesB.clear();
        return;
    }
    const int size = hunkChangedLinesA.size();
    for (int i = 0; i < size; ++i) {
        const auto [leftChange, rightChange] = inlineDiff(hunkChangedLinesA.at(i).line, hunkChangedLinesB.at(i).line);
        hunkChangedLinesA[i].c = leftChange;
        hunkChangedLinesB[i].c = rightChange;
    }

    auto addHighlights = [](HunkChangedLines &hunkChangedLines, QVector<LineHighlight> &hlts) {
        for (int i = 0; i < (int)hunkChangedLines.size(); ++i) {
            auto &change = hunkChangedLines[i];
            for (int j = hlts.size() - 1; j >= 0; --j) {
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

static QVector<KSyntaxHighlighting::Definition> defsForMimeTypes(const QSet<QString> &mimeTypes)
{
    if (mimeTypes.size() == 1) {
        return {KTextEditor::Editor::instance()->repository().definitionForMimeType(*mimeTypes.begin())};
    }

    QVector<KSyntaxHighlighting::Definition> defs;
    for (const auto &mt : mimeTypes) {
        defs.push_back(KTextEditor::Editor::instance()->repository().definitionForMimeType(mt));
    }
    QSet<QString> seenDefs;
    QVector<KSyntaxHighlighting::Definition> uniqueDefs;
    for (const auto &def : defs) {
        if (!seenDefs.contains(def.name())) {
            uniqueDefs.push_back(def);
            seenDefs.insert(def.name());
        }
    }
    if (uniqueDefs.size() == 1) {
        return uniqueDefs;
    } else {
        return {KTextEditor::Editor::instance()->repository().definitionForMimeType(QStringLiteral("None"))};
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
    QVector<LineHighlight> leftHlts;
    QVector<LineHighlight> rightHlts;
    // QVector<QPair<int, HunkData>> hunkDatas; // lineNo => HunkData

    // Line numbers that will be shown in the editor
    QVector<int> lineNumsA;
    QVector<int> lineNumsB;

    // Changed lines of hunk, used to determine differences between two lines
    HunkChangedLines hunkChangedLinesA;
    HunkChangedLines hunkChangedLinesB;

    QSet<QString> mimeTypes;
    QString srcFile;
    QString tgtFile;

    // viewLine => rawDiffLine
    QVector<ViewLineToDiffLine> lineToRawDiffLine;
    // for Folding/stage/unstage hunk
    QVector<ViewLineToDiffLine> linesWithHunkHeading;
    QVector<int> linesWithFileName;

    int maxLineNoFound = 0;
    int lineA = 0;
    int lineB = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        auto match = DIFF_FILENAME_RE.match(line);
        if (match.hasMatch() && i + 1 < text.size()) {
            srcFile = match.captured(1);
            mimeTypes.insert(QMimeDatabase().mimeTypeForFile(match.captured(1), QMimeDatabase::MatchExtension).name());
            auto match = DIFF_FILENAME_RE.match(text.at(i + 1));

            if (match.hasMatch()) {
                tgtFile = match.captured(1);
                mimeTypes.insert(QMimeDatabase().mimeTypeForFile(match.captured(1), QMimeDatabase::MatchExtension).name());
            }
            i++;

            if (m_params.flags.testFlag(DiffParams::ShowFileName)) {
                left.append(Utils::fileNameFromPath(srcFile));
                right.append(Utils::fileNameFromPath(tgtFile));
                Q_ASSERT(left.size() == right.size() && lineA == lineB);
                linesWithFileName.append(lineA);
                lineNumsA.append(-1);
                lineNumsB.append(-1);
                lineA++;
                lineB++;
            }
            continue;
        }

        match = HUNK_HEADER_RE.match(line);
        if (!match.hasMatch())
            continue;

        const auto oldRange = parseRange(match.captured(1));
        const auto newRange = parseRange(match.captured(2));
        const QString headingLeft = QStringLiteral("@@ ") + match.captured(1) + match.captured(3) /* + QStringLiteral(" ") + srcFile*/;
        const QString headingRight = QStringLiteral("@@ ") + match.captured(2) + match.captured(3) /* + QStringLiteral(" ") + tgtFile*/;

        lineNumsA.append(-1);
        lineNumsB.append(-1);
        left.append(headingLeft);
        right.append(headingRight);
        linesWithHunkHeading.append({lineA, i, /*unused*/ false});
        lineA++;
        lineB++;

        hunkChangedLinesA.clear();
        hunkChangedLinesB.clear();

        int srcLine = oldRange.first;
        const int oldCount = oldRange.second;

        int tgtLine = newRange.first;
        const int newCount = newRange.second;
        maxLineNoFound = qMax(qMax(srcLine + oldCount, tgtLine + newCount), maxLineNoFound);

        for (int j = i + 1; j < text.size(); j++) {
            QString l = text.at(j);
            if (l.startsWith(QLatin1Char(' '))) {
                // Insert dummy lines when left/right are unequal
                balanceHunkLines(left, right, lineA, lineB, lineNumsA, lineNumsB);
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);

                l = l.mid(1);
                left.append(l);
                right.append(l);
                //                 lineNo++;
                lineNumsA.append(srcLine++);
                lineNumsB.append(tgtLine++);
                lineA++;
                lineB++;
            } else if (l.startsWith(QLatin1Char('+'))) {
                //                 qDebug() << "- line";
                l = l.mid(1);
                LineHighlight h;
                h.line = lineB;
                h.added = true;
                h.changes.push_back({0, Change::FullBlock});
                rightHlts.push_back(h);
                lineNumsB.append(tgtLine++);
                lineToRawDiffLine.append({lineB, j, true});
                right.append(l);

                hunkChangedLinesB.emplace_back(l, lineB, h.added);

                //                 lineNo++;
                lineB++;
            } else if (l.startsWith(QLatin1Char('-'))) {
                l = l.mid(1);
                //                 qDebug() << "+ line: " << l;
                LineHighlight h;
                h.line = lineA;
                h.added = false;
                h.changes.push_back({0, Change::FullBlock});

                leftHlts.push_back(h);
                lineNumsA.append(srcLine++);
                left.append(l);
                hunkChangedLinesA.emplace_back(l, lineA, h.added);
                lineToRawDiffLine.append({lineA, j, false});

                lineA++;
            } else if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                i = j - 1;

                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);

                // add line number for current line
                lineNumsA.append(-1);
                lineNumsB.append(-1);

                // add new line
                left.append(QString());
                right.append(QString());
                lineA += 1;
                lineB += 1;
                break;
            } else if (l.startsWith(QStringLiteral("diff --git "))) {
                balanceHunkLines(left, right, lineA, lineB, lineNumsA, lineNumsB);
                // Start of a new file
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);
                // add new line
                lineNumsA.append(-1);
                lineNumsB.append(-1);
                left.append(QString());
                right.append(QString());
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
    m_lineToRawDiffLine += lineToRawDiffLine;
    m_lineToDiffHunkLine += linesWithHunkHeading;
    m_linesWithFileName += linesWithFileName;

    const auto defs = defsForMimeTypes(mimeTypes);
    leftHl->setDefinition(defs.constFirst());
    rightHl->setDefinition(defs.constFirst());
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
    QVector<LineHighlight> hlts;

    // Line numbers that will be shown in the editor
    QVector<int> lineNumsA;
    QVector<int> lineNumsB;

    // Changed lines of hunk, used to determine differences between two lines
    HunkChangedLines hunkChangedLinesA;
    HunkChangedLines hunkChangedLinesB;

    // viewLine => rawDiffLine
    QVector<ViewLineToDiffLine> lineToRawDiffLine;
    // for Folding/stage/unstage hunk
    QVector<ViewLineToDiffLine> linesWithHunkHeading;
    // Lines containing filename
    QVector<int> linesWithFileName;

    QSet<QString> mimeTypes;
    QString srcFile;
    QString tgtFile;

    int maxLineNoFound = 0;
    int lineNo = 0;

    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        auto match = DIFF_FILENAME_RE.match(line);
        if (match.hasMatch() && i + 1 < text.size()) {
            srcFile = match.captured(1);
            mimeTypes.insert(QMimeDatabase().mimeTypeForFile(match.captured(1), QMimeDatabase::MatchExtension).name());
            auto match = DIFF_FILENAME_RE.match(text.at(i + 1));

            if (match.hasMatch()) {
                tgtFile = match.captured(1);
                mimeTypes.insert(QMimeDatabase().mimeTypeForFile(match.captured(1), QMimeDatabase::MatchExtension).name());
            }
            i++;

            if (m_params.flags.testFlag(DiffParams::ShowFileName)) {
                lines.append(QStringLiteral("%1 → %2").arg(Utils::fileNameFromPath(srcFile), Utils::fileNameFromPath(tgtFile)));
                lineNumsA.append(-1);
                lineNumsB.append(-1);
                linesWithFileName.append(lineNo);
                lineNo++;
            }
            continue;
        }

        match = HUNK_HEADER_RE.match(line);
        if (!match.hasMatch())
            continue;

        const auto oldRange = parseRange(match.captured(1));
        const auto newRange = parseRange(match.captured(2));
        //         const QString headingLeft = QStringLiteral("@@ ") + match.captured(1) + match.captured(3) /* + QStringLiteral(" ") + srcFile*/;
        //         const QString headingRight = QStringLiteral("@@ ") + match.captured(2) + match.captured(3) /* + QStringLiteral(" ") + tgtFile*/;

        lines.push_back(line);
        lineNumsA.append(-1);
        lineNumsB.append(-1);
        linesWithHunkHeading.append({lineNo, i, false});
        lineNo++;

        hunkChangedLinesA.clear();
        hunkChangedLinesB.clear();

        int srcLine = oldRange.first;
        const int oldCount = oldRange.second;

        int tgtLine = newRange.first;
        const int newCount = newRange.second;
        maxLineNoFound = qMax(qMax(srcLine + oldCount, tgtLine + newCount), maxLineNoFound);

        for (int j = i + 1; j < text.size(); j++) {
            QString l = text.at(j);
            if (l.startsWith(QLatin1Char(' '))) {
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);

                l = l.mid(1);
                lines.append(l);
                lineNumsA.append(srcLine++);
                lineNumsB.append(tgtLine++);
                lineNo++;
                //                 lineB++;
            } else if (l.startsWith(QLatin1Char('+'))) {
                //                 qDebug() << "- line";
                l = l.mid(1);
                LineHighlight h;
                h.line = lineNo;
                h.added = true;
                h.changes.push_back({0, Change::FullBlock});
                lines.append(l);
                hlts.push_back(h);
                lineNumsA.append(-1);
                lineNumsB.append(tgtLine++);
                lineToRawDiffLine.append({lineNo, j, false});

                hunkChangedLinesB.emplace_back(l, lineNo, h.added);
                lineNo++;
            } else if (l.startsWith(QLatin1Char('-'))) {
                l = l.mid(1);
                LineHighlight h;
                h.line = lineNo;
                h.added = false;
                h.changes.push_back({0, Change::FullBlock});

                hlts.push_back(h);
                lineNumsA.append(srcLine++);
                lineNumsB.append(-1);
                lines.append(l);
                hunkChangedLinesA.emplace_back(l, lineNo, h.added);
                lineToRawDiffLine.append({lineNo, j, false});

                lineNo++;
            } else if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                i = j - 1;
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);
                // add line number for current line
                lineNumsA.append(-1);
                lineNumsB.append(-1);

                // add new line
                lines.append(QString());
                lineNo += 1;
                break;
            } else if (l.startsWith(QStringLiteral("diff --git "))) {
                // Start of a new file
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, hlts, hlts);
                // add new line
                lines.append(QString());
                lineNumsA.append(-1);
                lineNumsB.append(-1);
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
    m_lineToDiffHunkLine += linesWithHunkHeading;
    m_lineToRawDiffLine += lineToRawDiffLine;
    m_linesWithFileName += linesWithFileName;

    const auto defs = defsForMimeTypes(mimeTypes);
    leftHl->setDefinition(defs.constFirst());
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

    if (m_style == SideBySide) {
        parseAndShowDiff(raw);
    } else if (m_style == Unified) {
        parseAndShowDiffUnified(raw);
    } else if (m_style == Raw) {
        m_left->setPlainText(QString::fromUtf8(raw));
        leftHl->setDefinition(KTextEditor::Editor::instance()->repository().definitionForName(QStringLiteral("Diff")));
    }
    m_rawDiff = raw;
    m_left->verticalScrollBar()->setValue(0);
    m_right->verticalScrollBar()->setValue(0);
}

void DiffWidget::onTextReceived(const QByteArray &raw)
{
    if (m_style == SideBySide) {
        parseAndShowDiff(raw);
    } else if (m_style == Unified) {
        parseAndShowDiffUnified(raw);
    } else if (m_style == Raw) {
        m_left->appendPlainText(QString::fromUtf8(raw));
    }
    m_rawDiff += raw;
}

void DiffWidget::onError(const QByteArray & /*error*/, int /*code*/)
{
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
    for (int i = 0; i < m_lineToDiffHunkLine.size(); ++i) {
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
    for (const auto &i : m_linesWithFileName) {
        if (i > block) {
            nextFileLineNo = i;
            break;
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(nextFileLineNo);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(nextFileLineNo);
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
    m_left->scrollToBlock(prevFileLineNo);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(prevFileLineNo);
    }
}

void DiffWidget::jumpToNextHunk()
{
    const int block = m_left->firstVisibleBlockNumber();
    int nextHunkLineNo = 0;
    for (const auto &i : m_lineToDiffHunkLine) {
        if (i.line > block) {
            nextHunkLineNo = i.line;
            break;
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(nextHunkLineNo);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(nextHunkLineNo);
    }
}

void DiffWidget::jumpToPrevHunk()
{
    const int block = m_left->firstVisibleBlockNumber();
    int prevHunkLineNo = 0;
    for (auto i = m_lineToDiffHunkLine.crbegin(); i != m_lineToDiffHunkLine.crend(); ++i) {
        if (i->line < block) {
            prevHunkLineNo = i->line;
            break;
        }
    }

    QScopedValueRollback r(m_stopScrollSync, true);
    m_left->scrollToBlock(prevHunkLineNo);
    if (m_style == SideBySide) {
        m_right->scrollToBlock(prevHunkLineNo);
    }
}

#include "diffwidget.moc"
