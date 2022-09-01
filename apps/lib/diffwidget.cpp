/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "diffwidget.h"
#include "diffeditor.h"
#include "ktexteditor_utils.h"

#include "gitprocess.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMimeDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSyntaxHighlighter>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

std::pair<uint, uint> parseRange(const QString &range)
{
    int commaPos = range.indexOf(QLatin1Char(','));
    if (commaPos > -1) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        return {range.midRef(0, commaPos).toInt(), range.midRef(commaPos + 1).toInt()};
#else
        return {QStringView(range).sliced(0, commaPos).toInt(), QStringView(range).sliced(commaPos + 1).toInt()};
#endif
    }
    return {range.toInt(), 1};
}

DiffWidget::DiffWidget(QWidget *parent)
    : QWidget(parent)
    , m_left(new DiffEditor(this))
    , m_right(new DiffEditor(this))
{
    auto layout = new QHBoxLayout(this);
    layout->addWidget(m_left);
    layout->addWidget(m_right);

    leftHl = new KSyntaxHighlighting::SyntaxHighlighter(m_left->document());
    rightHl = new KSyntaxHighlighting::SyntaxHighlighter(m_right->document());
    leftHl->setTheme(KTextEditor::Editor::instance()->theme());
    rightHl->setTheme(KTextEditor::Editor::instance()->theme());

    connect(m_left->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
        const QSignalBlocker b(m_left);
        m_right->verticalScrollBar()->setValue(v);
    });
    connect(m_right->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
        const QSignalBlocker b(m_right);
        m_left->verticalScrollBar()->setValue(v);
    });
}

void DiffWidget::diffDocs(KTextEditor::Document *l, KTextEditor::Document *r)
{
    m_left->clear();
    m_right->clear();
    m_left->clearData();
    m_right->clearData();

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
    setupGitProcess(*git,
                    qApp->applicationDirPath(),
                    {QStringLiteral("diff"), /*QStringLiteral("--word-diff=porcelain"),*/ QStringLiteral("--no-index"), left, right});

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

struct HunkChangedLine {
    const QString line;
    bool added;
    int lineNo;
    Change c = {-1, -1};
};

static void markInlineDiffs(QVector<HunkChangedLine> &hunkChangedLinesA,
                            QVector<HunkChangedLine> &hunkChangedLinesB,
                            QVector<LineHighlight> &leftHlts,
                            QVector<LineHighlight> &rightHlts)
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

    auto addHighlights = [](QVector<HunkChangedLine> &hunkChangedLines, QVector<LineHighlight> &hlts) {
        for (int i = 0; i < hunkChangedLines.size(); ++i) {
            auto &change = hunkChangedLines[i];
            for (int j = hlts.size() - 1; j >= 0; --j) {
                if (hlts.at(j).line == change.lineNo) {
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

void DiffWidget::openDiff(const QByteArray &raw)
{
    //     printf("show diff:\n%s\n================================", raw.constData());
    const QStringList text = QString::fromUtf8(raw).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    static const QRegularExpression HUNK_HEADER_RE(QStringLiteral("^@@ -([0-9,]+) \\+([0-9,]+) @@(.*)"));
    static const QRegularExpression DIFF_FILENAME_RE(QStringLiteral("^[-+]{3} [ab]/(.*)"));

    QStringList left;
    QStringList right;

    QVector<LineHighlight> leftHlts;
    QVector<LineHighlight> rightHlts;
    // QVector<QPair<int, HunkData>> hunkDatas; // lineNo => HunkData

    QVector<int> lineToLineNumLeft;
    QVector<int> lineToLineNumRight;

    QVector<HunkChangedLine> hunkChangedLinesA;
    QVector<HunkChangedLine> hunkChangedLinesB;

    QSet<QString> mimeTypes;
    QString srcFile;
    QString tgtFile;

    int maxLineNoFound = 0;
    int lineA = 0;
    int lineB = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        auto match = DIFF_FILENAME_RE.match(line);
        if (match.hasMatch()) {
            if (line.startsWith(QLatin1Char('-'))) {
                srcFile = match.captured(1);
            } else if (line.startsWith(QLatin1Char('+'))) {
                tgtFile = match.captured(1);
            }
            if (!match.captured(1).isEmpty()) {
                mimeTypes.insert(QMimeDatabase().mimeTypeForFile(match.captured(1), QMimeDatabase::MatchExtension).name());
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

        lineToLineNumLeft.append(-1);
        lineToLineNumRight.append(-1);
        left.append(headingLeft);
        right.append(headingRight);
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
                balanceHunkLines(left, right, lineA, lineB, lineToLineNumLeft, lineToLineNumRight);
                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);

                l = l.mid(1);
                left.append(l);
                right.append(l);
                //                 lineNo++;
                lineToLineNumLeft.append(srcLine++);
                lineToLineNumRight.append(tgtLine++);
                lineA++;
                lineB++;
            } else if (l.startsWith(QLatin1Char('+'))) {
                //                 qDebug() << "- line";
                l = l.mid(1);
                LineHighlight h;
                h.line = lineB;
                h.added = true;
                h.changes.push_back({0, l.size()});
                rightHlts.push_back(h);
                lineToLineNumRight.append(tgtLine++);
                right.append(l);

                hunkChangedLinesB.append({l, true, lineB});

                //                 lineNo++;
                lineB++;
            } else if (l.startsWith(QLatin1Char('-'))) {
                l = l.mid(1);
                //                 qDebug() << "+ line: " << l;
                LineHighlight h;
                h.line = lineA;
                h.added = false;
                h.changes.push_back({0, l.size()});

                leftHlts.push_back(h);
                lineToLineNumLeft.append(srcLine++);
                left.append(l);
                hunkChangedLinesA.append({l, false, lineA});

                lineA++;
            } else if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                i = j - 1;

                markInlineDiffs(hunkChangedLinesA, hunkChangedLinesB, leftHlts, rightHlts);

                // add line number for current line
                lineToLineNumLeft.append(-1);
                lineToLineNumRight.append(-1);

                // add new line
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

    balanceHunkLines(left, right, lineA, lineB, lineToLineNumLeft, lineToLineNumRight);

    QString leftText = left.join(QLatin1Char('\n'));
    QString rightText = right.join(QLatin1Char('\n'));

    m_left->appendData(leftHlts);
    m_right->appendData(rightHlts);
    m_left->appendPlainText(leftText);
    m_right->appendPlainText(rightText);
    m_left->setLineNumberData(lineToLineNumLeft, maxLineNoFound);
    m_right->setLineNumberData(lineToLineNumRight, maxLineNoFound);
    // Only do highlighting if there is one mimetype found, multiple different files not supported
    if (mimeTypes.size() == 1) {
        const auto def = KTextEditor::Editor::instance()->repository().definitionForMimeType(*mimeTypes.begin());
        leftHl->setDefinition(def);
        rightHl->setDefinition(def);
    } else {
        const auto def = KTextEditor::Editor::instance()->repository().definitionForMimeType(QStringLiteral("None"));
        leftHl->setDefinition(def);
        rightHl->setDefinition(def);
    }
}

void DiffWidget::openWordDiff(const QByteArray &raw)
{
    //     printf("show diff:\n%s\n================================", raw.constData());
    openDiff(raw);
    return;
    const QStringList text = QString::fromUtf8(raw).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    static const QRegularExpression HUNK_HEADER_RE(QStringLiteral("^@@ -([0-9,]+) \\+([0-9,]+) @@(.*)"));

    QStringList left;
    QStringList right;

    QVector<LineHighlight> leftHlts;
    QVector<LineHighlight> rightHlts;
    // QVector<QPair<int, HunkData>> hunkDatas; // lineNo => HunkData

    QVector<int> lineToLineNumLeft;
    QVector<int> lineToLineNumRight;

    int maxFound = 0;
    int lineNo = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        const auto match = HUNK_HEADER_RE.match(line);
        if (!match.hasMatch())
            continue;

        const auto oldRange = parseRange(match.captured(1));
        const auto newRange = parseRange(match.captured(2));

        lineToLineNumLeft.append(-1);
        lineToLineNumRight.append(-1);
        left.append(line);
        right.append(line);
        lineNo++;
        left.append(QString());
        right.append(QString());

        const int srcStart = oldRange.first;
        int srcLine = oldRange.first;
        const int oldCount = oldRange.second;
        const int tgtStart = newRange.first;
        int tgtLine = newRange.first;
        const int newCount = newRange.second;
        maxFound = qMax(qMax(srcStart + oldCount, tgtStart + newCount), maxFound);
        const int oldSize = left.size();

        for (int j = i + 1; j < text.size(); j++) {
            QString l = text.at(j);
            if (l.startsWith(QLatin1Char(' '))) {
                l = l.mid(1);
                left.back().append(l);
                right.back().append(l);
            } else if (l.startsWith(QLatin1Char('+'))) {
                //                 qDebug() << "- line";
                l = l.mid(1);
                LineHighlight h;
                h.line = lineNo;
                h.added = true;
                h.changes.push_back({right.back().size(), l.size()});

                if (!rightHlts.isEmpty() && rightHlts.back().line == lineNo) {
                    rightHlts.back().changes.append(h.changes);
                } else {
                    rightHlts.push_back(h);
                }

                right.back().append(l);
            } else if (l.startsWith(QLatin1Char('-'))) {
                l = l.mid(1);
                //                 qDebug() << "+ line: " << l;
                LineHighlight h;
                h.line = lineNo;
                h.added = false;
                h.changes.push_back({left.back().size(), l.size()});

                if (!leftHlts.isEmpty() && leftHlts.back().line == lineNo) {
                    leftHlts.back().changes.append(h.changes);
                } else {
                    leftHlts.push_back(h);
                }
                left.back().append(l);
            } else if (l.startsWith(QLatin1Char('~'))) {
                left.append(QString());
                right.append(QString());
                lineToLineNumLeft.append(tgtLine++);
                lineToLineNumRight.append(srcLine++);
                lineNo++;
            } else if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                i = j - 1;

                // add line number for current line
                lineToLineNumLeft.append(tgtStart);
                lineToLineNumRight.append(srcStart);

                // add new line
                left.append(QString());
                right.append(QString());
                lineNo++;
                // line number for this line
                lineToLineNumLeft.append(-1);
                lineToLineNumRight.append(-1);

                lineNo++;
                break;
            }

            if (j + 1 >= text.size()) {
                lineToLineNumLeft.append(tgtLine++);
                lineToLineNumRight.append(srcLine++);
                i = j; // ensure outer loop also exits after this
            }
        }

        // Adjust line numbers
        Q_ASSERT(left.size() == right.size() && left.size() == lineToLineNumLeft.size() && right.size() == lineToLineNumRight.size());
        const int newSize = left.size();
        int s = srcStart;
        int t = tgtStart;
        for (int i = oldSize - 1; i < newSize; ++i) {
            if (left.at(i).isNull() || t > tgtStart + newCount) {
                lineToLineNumLeft[i] = -1;
            } else {
                lineToLineNumLeft[i] = t++;
            }
            if (right.at(i).isNull() || s > srcStart + oldCount) {
                lineToLineNumRight[i] = -1;
            } else {
                lineToLineNumRight[i] = s++;
            }
        }
        qDebug() << srcStart << tgtStart << s << t << oldCount << newCount;
    }

    Q_ASSERT(left.size() == right.size() && left.size() == lineToLineNumLeft.size() && right.size() == lineToLineNumRight.size());

    QString leftText = left.join(QLatin1Char('\n'));
    QString rightText = right.join(QLatin1Char('\n'));

    //     printf("(%d), lt %d ln %d -- rt %d rn %d\n", lineNo, left.size(), lineToLineNumLeft.size(), right.size(), lineToLineNumRight.size());
    m_left->appendData(leftHlts);
    m_right->appendData(rightHlts);
    m_left->appendPlainText(leftText);
    m_right->appendPlainText(rightText);
    m_left->setLineNumberData(lineToLineNumLeft, maxFound);
    m_right->setLineNumberData(lineToLineNumRight, maxFound);
}

void DiffWidget::onTextReceived(const QByteArray &raw)
{
    //     printf("Got Text: \n%s\n==============\n", raw.constData());
    openDiff(raw);
    //     openWordDiff(raw);
}

void DiffWidget::onError(const QByteArray &error, int /*code*/)
{
    //     printf("Got error: \n%s\n==============\n", error.constData());
}
