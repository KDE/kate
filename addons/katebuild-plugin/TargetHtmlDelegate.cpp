/***************************************************************************
 *   This file is part of Kate search plugin                               *
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "TargetHtmlDelegate.h"
#include "TargetModel.h"

#include <KLocalizedString>
#include <QAbstractTextDocumentLayout>
#include <QCompleter>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPainter>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QToolButton>

#include "UrlInserter.h"

#include <QDebug>
#include <qnamespace.h>

TargetHtmlDelegate::TargetHtmlDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_isEditing(false)
{
    connect(this, &TargetHtmlDelegate::sendEditStart, this, &TargetHtmlDelegate::editStarted);
}

TargetHtmlDelegate::~TargetHtmlDelegate() = default;

void TargetHtmlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QTextDocument doc;

    QString str;
    int rowtype = index.data(TargetModel::RowTypeRole).toInt();
    if (rowtype == TargetModel::RootRow) {
        str = QStringLiteral("<B>%1</B>").arg(index.data().toString().toHtmlEscaped());
    } else if (rowtype == TargetModel::TargetSetRow) {
        if (index.column() == 0) {
            str = i18nc("T as in Target set", "<B>T:</B> %1", index.data().toString().toHtmlEscaped());
        } else if (index.column() == 1) {
            str = i18nc("Dir as in working Directory", "<B>Dir:</B> %1", index.data().toString().toHtmlEscaped());
        }
    } else {
        str = index.data().toString().toHtmlEscaped();
    }

    if (option.state & QStyle::State_Selected) {
        str = QStringLiteral("<font color=\"%1\">").arg(option.palette.highlightedText().color().name()) + str + QStringLiteral("</font>");
    }
    doc.setHtml(str);
    doc.setDocumentMargin(2);

    painter->save();

    // paint background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else {
        painter->fillRect(option.rect, option.palette.base());
    }

    painter->setClipRect(option.rect);

    options.text = QString(); // clear old text
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    // draw text
    painter->translate(option.rect.x(), option.rect.y());
    doc.drawContents(painter);

    painter->restore();
}

QSize TargetHtmlDelegate::sizeHint(const QStyleOptionViewItem & /* option */, const QModelIndex &index) const
{
    QTextDocument doc;
    doc.setHtml(index.data().toString().toHtmlEscaped());
    doc.setDocumentMargin(2);
    if (index.column() == 1 && !index.parent().isValid()) {
        return doc.size().toSize() + QSize(38, 0); // add space for "Dir"
    }
    return doc.size().toSize();
}

QWidget *TargetHtmlDelegate::createEditor(QWidget *dparent, const QStyleOptionViewItem & /* option */, const QModelIndex &index) const
{
    QWidget *editor;
    if (index.internalId() == TargetModel::InvalidIndex && index.column() == 1) {
        auto *requester = new UrlInserter(parent()->property("docUrl").toUrl(), dparent);
        requester->setReplace(true);
        editor = requester;
        editor->setToolTip(i18n("Leave empty to use the directory of the current document.\nAdd search directories by adding paths separated by ';'"));
    } else if (index.column() == 1 || index.column() == 2) {
        auto *urlEditor = new UrlInserter(parent()->property("docUrl").toUrl(), dparent);
        editor = urlEditor;
        int const rowtype = index.data(TargetModel::RowTypeRole).toInt();
        if (rowtype == TargetModel::TargetSetRow) {
            // Working directory
            editor->setToolTip(i18n("Use:\n\"%B\" for project base directory\n\"%b\" for name of project base directory"));
        } else {
            // Command
            editor->setToolTip(
                i18n("Use:\n"
                     "\"%f\" for current file\n"
                     "\"%d\" for directory of current file\n"
                     "\"%n\" for current file name without suffix\n"
                     "\"%B\" for current project's base directory\n"
                     "\"%w\" for the working directory of the target"));
        }
    } else {
        auto *e = new QLineEdit(dparent);
        auto *completer = new QCompleter(e);
        auto *model = new QFileSystemModel(e);
        model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
        completer->setModel(model);
        e->setCompleter(completer);
        editor = e;
    }
    editor->setAutoFillBackground(true);
    Q_EMIT sendEditStart();
    connect(editor, &QWidget::destroyed, this, &TargetHtmlDelegate::editEnded, Qt::QueuedConnection);
    return editor;
}

void TargetHtmlDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();

    UrlInserter *urlIns = qobject_cast<UrlInserter *>(editor);
    if (urlIns) {
        urlIns->lineEdit()->setText(value);
        return;
    }
    QLineEdit *ledit = qobject_cast<QLineEdit *>(editor);
    if (ledit) {
        ledit->setText(value);
    }
}

void TargetHtmlDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString value;
    UrlInserter *urlIns = qobject_cast<UrlInserter *>(editor);
    if (urlIns) {
        value = urlIns->lineEdit()->text();
    } else if (auto *ledit = qobject_cast<QLineEdit *>(editor)) {
        value = ledit->text();
    }
    model->setData(index, value, Qt::EditRole);
}

void TargetHtmlDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    QRect rect = option.rect;
    int heightDiff = QToolButton().sizeHint().height() - rect.height();
    int half = heightDiff / 2;
    rect.adjust(0, -half, 0, heightDiff - half);
    editor->setGeometry(rect);
}

void TargetHtmlDelegate::editStarted()
{
    m_isEditing = true;
}
void TargetHtmlDelegate::editEnded()
{
    m_isEditing = false;
}
bool TargetHtmlDelegate::isEditing() const
{
    return m_isEditing;
}

#include "moc_TargetHtmlDelegate.cpp"
