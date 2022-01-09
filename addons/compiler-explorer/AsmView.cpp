#include "AsmView.h"
#include <AsmViewModel.h>

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>

#include <KLocalizedString>
#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>

#include <QDebug>

#include <kfts_fuzzy_match.h>
#include <ktexteditor_utils.h>

class LineNumberDelegate : public QStyledItemDelegate
{
public:
    LineNumberDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
        auto updateColors = [this] {
            auto e = KTextEditor::Editor::instance();
            auto theme = e->theme();
            lineNumColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::LineNumbers));
            borderColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::Separator));
            currentLineColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::CurrentLineNumber));
            iconBorderColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::IconBorder));
        };
        updateColors();
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
    {
        QStyleOptionViewItem option = opt;
        initStyleOption(&option, index);

        painter->save();

        QString text = option.text;
        option.text = QString();

        auto iconBorder = option.rect;

        QColor textColor = lineNumColor;
        // paint background
        if (option.state & QStyle::State_Selected) {
            textColor = currentLineColor;
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(iconBorder, iconBorderColor);
        }

        option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

        iconBorder.setRight(iconBorder.right() - 1); // leave space for separator

        painter->setPen(borderColor);
        painter->drawLine(iconBorder.topRight(), iconBorder.bottomRight());

        auto textRect = option.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget);
        textRect.setRight(textRect.right() - 5); // 4 px padding
        painter->setFont(index.data(Qt::FontRole).value<QFont>());
        painter->setPen(textColor);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);

        painter->restore();
    }

private:
    QColor currentLineColor;
    QColor borderColor;
    QColor lineNumColor;
    QColor iconBorderColor;
};

class CodeDelegate : public QStyledItemDelegate
{
public:
    CodeDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
        auto updateColors = [this] {
            auto e = KTextEditor::Editor::instance();
            auto theme = e->theme();

            normalColor = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));
            keywordColor = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Keyword));
            funcColor = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Function));
            stringColor = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::String));
        };
        updateColors();
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
    {
        QStyleOptionViewItem option = opt;
        initStyleOption(&option, index);

        painter->save();

        QString text = option.text;
        option.text = QString();

        option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

        const AsmViewModel *m = static_cast<const AsmViewModel *>(index.model());
        if (m->hasError()) {
            drawTextWithErrors(painter, option, text);
            painter->restore();
            return;
        }

        QVector<QTextLayout::FormatRange> fmts;

        // is a label?
        if (!text.isEmpty() && !text.at(0).isSpace()) {
            QTextCharFormat f;
            f.setForeground(funcColor);
            int colon = findColon(text);
            if (colon > -1) {
                fmts.append({0, colon + 1, f});
            }

        } else {
            int i = firstNotSpace(text);
            int nextSpace = text.indexOf(QLatin1Char(' '), i + 1);

            // If there is no space then this is the only word on the line
            // e.g "ret"
            if (nextSpace == -1) {
                nextSpace = text.length();
            }

            QTextCharFormat f;

            if (i >= 0 && nextSpace > i) {
                f.setForeground(keywordColor);
                fmts.append({i, nextSpace - i, f});

                i = nextSpace + 1;
            }

            const auto [strOpen, strClose] = getStringPos(text, i);
            if (strOpen >= 0) {
                f = QTextCharFormat();
                f.setForeground(stringColor);
                fmts.append({strOpen, strClose - strOpen, f});

                // move forward
                i = strClose;
            }

            auto labels = this->rowLabels(index);
            if (!labels.isEmpty()) {
                f = QTextCharFormat();
                f.setForeground(funcColor);
                f.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                for (const auto &label : labels) {
                    fmts.append({label.col, label.len, f});
                }
            }
        }

        kfts::paintItemViewText(painter, text, option, fmts);

        painter->restore();
    }

    static int firstNotSpace(const QString &text)
    {
        for (int i = 0; i < text.size(); ++i) {
            if (!text.at(i).isSpace()) {
                return i;
            }
        }
        return 0;
    }

    static std::pair<int, int> getStringPos(const QString &text, int from)
    {
        int open = text.indexOf(QLatin1Char('"'), from);
        if (open == -1)
            return {-1, -1};
        int close = text.indexOf(QLatin1Char('"'), open + 1);
        if (close == -1)
            return {-1, -1};
        return {open, close + 1}; // +1 because we include the quote as well
    }

    static int findColon(const QString &text, int from = 0)
    {
        int colon = text.indexOf(QLatin1Char(':'), from);
        if (colon == -1) {
            return -1;
        }

        if (colon + 1 >= text.length()) {
            return colon;
        }

        if (text.at(colon + 1) != QLatin1Char(':')) {
            return colon;
        }

        colon += 2;

        auto isLabelEnd = [text](int &i) {
            if (text.at(i) == QLatin1Char(':')) {
                // reached end, good enough to be a label
                if (i + 1 >= text.length()) {
                    return true;
                }
                if (text.at(i + 1) != QLatin1Char(':')) {
                    return true;
                }
                i++;
            }
            return false;
        };

        for (int i = colon; i < text.length(); i++) {
            if (isLabelEnd(i)) {
                return i;
            }
        }

        return -1;
    }

    static QVector<LabelInRow> rowLabels(const QModelIndex &index)
    {
        return index.data(AsmViewModel::RowLabels).value<QVector<LabelInRow>>();
    }

    void drawTextWithErrors(QPainter *p, const QStyleOptionViewItem &option, const QString &text) const
    {
        QVector<QTextLayout::FormatRange> fmts;

        int errIdx = text.indexOf(QLatin1String("error:"));
        if (errIdx != -1) {
            QTextCharFormat f;
            f.setForeground(keywordColor);
            fmts.append({errIdx, 5, f});
        }

        kfts::paintItemViewText(p, text, option, fmts);
    }

private:
    QColor keywordColor;
    QColor funcColor;
    QColor normalColor;
    QColor stringColor;
};

AsmView::AsmView(QWidget *parent)
    : QTreeView(parent)
{
    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setHeaderHidden(true);
    setSelectionMode(QAbstractItemView::ContiguousSelection);

    setItemDelegateForColumn(0, new LineNumberDelegate(this));
    setItemDelegateForColumn(1, new CodeDelegate(this));

    auto updateColors = [this] {
        auto e = KTextEditor::Editor::instance();
        auto theme = e->theme();
        auto palette = this->palette();
        QColor c = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::CurrentLine));
        palette.setColor(QPalette::Highlight, c);
        c = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));
        palette.setColor(QPalette::Text, c);
        c = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
        palette.setColor(QPalette::Base, c);
        setPalette(palette);

        auto model = static_cast<AsmViewModel *>(this->model());
        model->setFont(Utils::editorFont());

    };
    updateColors();
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
}

void AsmView::contextMenuEvent(QContextMenuEvent *e)
{
    QPoint p = e->pos();

    QMenu menu(this);
    menu.addAction(i18n("Scroll to source"), this, [this, p] {
        auto model = static_cast<AsmViewModel *>(this->model());
        int line = model->sourceLineForAsmLine(indexAt(p));
        Q_EMIT scrollToLineRequested(line);
    });

    QModelIndex index = indexAt(e->pos());
    if (index.isValid()) {
        auto labels = index.data(AsmViewModel::RowLabels).value<QVector<LabelInRow>>();
        if (!labels.isEmpty()) {
            menu.addAction(i18n("Jump to label"), this, [this, index] {
                auto model = static_cast<AsmViewModel *>(this->model());

                const auto labels = index.data(AsmViewModel::RowLabels).value<QVector<LabelInRow>>();
                if (labels.isEmpty()) {
                    return;
                }

                const QString asmLine = index.data().toString();

                auto labelInRow = labels.first();
                QString label = asmLine.mid(labelInRow.col, labelInRow.len);
                int line = model->asmLineForLabel(label);

                if (line != -1) {
                    auto labelIdx = model->index(line - 1, 1);
                    scrollTo(labelIdx, ScrollHint::PositionAtCenter);
                    if (selectionModel()) {
                        selectionModel()->select(labelIdx, QItemSelectionModel::ClearAndSelect);
                    }
                }
            });
        }
    }

    if (!selectedIndexes().isEmpty()) {
        menu.addAction(i18n("Copy"), this, [this] {
            const auto selected = selectedIndexes();
            QString text;
            for (const auto idx : selected) {
                if (idx.column() == AsmViewModel::Column_LineNo)
                    continue;
                text += idx.data().toString() + QStringLiteral("\n");
            }
            qApp->clipboard()->setText(text);
        });
    }

    menu.addAction(i18n("Select All"), this, [this] {
        if (auto sm = selectionModel()) {
            QItemSelection sel;
            auto start = model()->index(0, 0);
            auto end = model()->index(model()->rowCount() - 1, model()->columnCount() - 1);
            sel.select(start, end);
            sm->select(sel, QItemSelectionModel::ClearAndSelect);
        }
    });

    menu.exec(mapToGlobal(p));
}
