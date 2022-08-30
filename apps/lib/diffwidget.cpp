/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "diffwidget.h"
#include "gitprocess.h"
#include "ktexteditor_utils.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSyntaxHighlighter>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using IntT = qsizetype;
#else
using IntT = int;
#endif

struct Change {
    IntT pos;
    IntT len;
};

struct LineHilight {
    QVector<Change> changes;
    IntT line;
    bool added;
};

class DiffHighlighter : public QSyntaxHighlighter
{
public:
    DiffHighlighter(QTextDocument *parent)
        : QSyntaxHighlighter(parent)
    {
    }

    void highlightBlock(const QString &) override
    {
        auto block = currentBlock();
        int num = block.blockNumber();
        auto it = std::find_if(data.cbegin(), data.cend(), [num](LineHilight hl) {
            return hl.line == num;
        });
        if (it != data.cend()) {
            QColor color = it->added ? Qt::green : Qt::red;
            const auto changes = it->changes;
            for (const auto c : changes) {
                setFormat(c.pos, c.len, color);
            }
        }
    }

    void clearData()
    {
        data.clear();
    }
    void appendData(const QVector<LineHilight> &newData)
    {
        data.append(newData);
    }

private:
    QVector<LineHilight> data;
};

class DiffEditor : public QPlainTextEdit
{
public:
    DiffEditor(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
        red1 = QColor("#c87872");
        red1.setAlphaF(0.2);
        green1 = QColor("#678528");
        green1.setAlphaF(0.2);

        auto c = QColor(254, 147, 140);
        c.setAlphaF(0.1);
        red2 = c;

        c = QColor(166, 226, 46);
        c.setAlphaF(0.1);
        green2 = c;

        auto updateEditorColors = [this](KTextEditor::Editor *e) {
            if (!e)
                return;
            auto theme = e->theme();
            auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::BackgroundColor));
            auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::TextStyle::Normal));
            auto sel = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::TextSelection));
            auto pal = palette();
            pal.setColor(QPalette::Base, bg);
            pal.setColor(QPalette::Text, fg);
            pal.setColor(QPalette::Highlight, sel);
            pal.setColor(QPalette::HighlightedText, fg);
            setPalette(pal);
        };
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateEditorColors);
        updateEditorColors(KTextEditor::Editor::instance());
    }

    void paintEvent(QPaintEvent *e) override
    {
        bool textPainted = false;
        if (!getPaintContext().selections.isEmpty()) {
            QPlainTextEdit::paintEvent(e);
            textPainted = true;
        }

        QPainter p(viewport());
        QPointF offset(contentOffset());
        QTextBlock block = firstVisibleBlock();
        const auto viewportRect = viewport()->rect();

        while (block.isValid()) {
            QRectF r = blockBoundingRect(block).translated(offset);
            auto layout = block.layout();

            auto hl = dataForLine(block.blockNumber());
            if (hl && layout) {
                const auto changes = hl->changes;
                for (auto c : changes) {
                    // full line background is colored
                    p.fillRect(r, hl->added ? green1 : red1);
                    QTextLine sl = layout->lineForTextPosition(c.pos);
                    QTextLine el = layout->lineForTextPosition(c.pos + c.len);
                    // color any word diffs
                    if (sl.isValid() && sl.lineNumber() == el.lineNumber()) {
                        int sx = sl.cursorToX(c.pos);
                        int ex = el.cursorToX(c.pos + c.len);
                        QRectF r = sl.naturalTextRect();
                        r.setLeft(sx);
                        r.setRight(ex);
                        r.moveTop(offset.y() + (sl.height() * sl.lineNumber()));
                        p.fillRect(r, hl->added ? green2 : red2);
                    } else {
                        QPainterPath path;
                        int i = sl.lineNumber() + 1;
                        int end = el.lineNumber();
                        QRectF rect = sl.naturalTextRect();
                        rect.setLeft(sl.cursorToX(c.pos));
                        rect.moveTop(offset.y() + (sl.height() * sl.lineNumber()));
                        path.addRect(rect);
                        for (; i <= end; ++i) {
                            auto line = layout->lineAt(i);
                            rect = line.naturalTextRect();
                            rect.moveTop(offset.y() + (line.height() * line.lineNumber()));
                            if (i == end) {
                                rect.setRight(el.cursorToX(c.pos + c.len));
                            }
                            path.addRect(rect);
                        }
                        p.fillPath(path, hl->added ? green2 : red2);
                    }
                }
            }

            if (block.text().startsWith(QStringLiteral("@@ "))) {
                p.save();
                p.setPen(Qt::red);
                p.setBrush(Qt::NoBrush);
                QRectF copy = r;
                copy.setRight(copy.right() - 1);
                p.drawRect(copy);
                p.restore();
            }

            offset.ry() += r.height();
            if (offset.y() > viewportRect.height()) {
                break;
            }
            block = block.next();
        }

        if (!textPainted) {
            QPlainTextEdit::paintEvent(e);
        }
    }

    void clearData()
    {
        data.clear();
    }
    void appendData(const QVector<LineHilight> &newData)
    {
        data.append(newData);
    }

    const LineHilight *dataForLine(int line)
    {
        auto it = std::find_if(data.cbegin(), data.cend(), [line](LineHilight hl) {
            return hl.line == line;
        });
        return it == data.cend() ? nullptr : &(*it);
    }

private:
    QVector<LineHilight> data;
    QColor red1;
    QColor red2;
    QColor green1;
    QColor green2;
};

DiffWidget::DiffWidget(QWidget *parent)
    : QWidget(parent)
    , m_left(new DiffEditor(this))
    , m_right(new DiffEditor(this))
{
    auto layout = new QHBoxLayout(this);
    layout->addWidget(m_left);
    layout->addWidget(m_right);

    m_left->setFont(Utils::editorFont());
    m_right->setFont(Utils::editorFont());
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
                    {QStringLiteral("diff"), QStringLiteral("--word-diff=porcelain"), QStringLiteral("--no-index"), left, right});

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

void DiffWidget::openWordDiff(const QByteArray &raw)
{
    //     printf("show diff:\n%s\n================================", raw.constData());
    const QStringList text = QString::fromUtf8(raw).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).split(QLatin1Char('\n'));

    static const QRegularExpression HUNK_HEADER_RE(QStringLiteral("^@@ -([0-9,]+) \\+([0-9,]+) @@(.*)"));

    QStringList left;
    QStringList right;

    left.append(QString());
    right.append(QString());

    QVector<LineHilight> leftHlts;
    QVector<LineHilight> rightHlts;

    int lineNo = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QString &line = text.at(i);
        const auto match = HUNK_HEADER_RE.match(line);
        if (!match.hasMatch())
            continue;
        //         printf("new hunk");

        for (int j = i + 1; j < text.size(); j++) {
            QString l = text.at(j);
            if (l.startsWith(QLatin1Char(' '))) {
                l = l.mid(1);
                left.back().append(l);
                right.back().append(l);
            } else if (l.startsWith(QLatin1Char('+'))) {
                //                 qDebug() << "- line";
                l = l.mid(1);
                LineHilight h;
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
                LineHilight h;
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
                lineNo++;
            }

            if (l.startsWith(QStringLiteral("@@ ")) && HUNK_HEADER_RE.match(l).hasMatch()) {
                //                 printf("break: %s\n", l.toUtf8().constData());
                i = j - 1;

                // 2 empty lines for hunk
                left.append(QString());
                right.append(QString());
                left.append(l);
                right.append(l);
                lineNo += 2;
                break;
            }
        }
    }

    QString leftText = left.join(QLatin1Char('\n'));
    QString rightText = right.join(QLatin1Char('\n'));

    m_left->appendData(leftHlts);
    m_right->appendData(rightHlts);
    m_left->appendPlainText(leftText);
    m_right->appendPlainText(rightText);
}

void DiffWidget::onTextReceived(const QByteArray &raw)
{
    //     printf("Got Text: \n%s\n==============\n", raw.constData());
    openWordDiff(raw);
}

void DiffWidget::onError(const QByteArray &error, int /*code*/)
{
    //     printf("Got error: \n%s\n==============\n", error.constData());
}
