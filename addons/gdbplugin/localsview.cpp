//
// Description: Widget that local variables of the gdb inferior
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "localsview.h"
#include "dap/entities.h"

#include <KLocalizedString>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QLabel>
#include <QMenu>

LocalsView::LocalsView(QWidget *parent)
    : QTreeWidget(parent)
{
    QStringList headers;
    headers << i18n("Symbol");
    headers << i18n("Type");
    headers << i18n("Value");
    setHeaderLabels(headers);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformRowHeights(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &LocalsView::onContextMenu);
    connect(this, &QTreeWidget::itemExpanded, this, &LocalsView::onItemExpanded);
}

LocalsView::~LocalsView() = default;

void LocalsView::showEvent(QShowEvent *)
{
    Q_EMIT localsVisible(true);
}

void LocalsView::hideEvent(QHideEvent *)
{
    Q_EMIT localsVisible(false);
}

static QString nameTip(const dap::Variable &variable)
{
    QString text = QStringLiteral("<qt>%1<qt>").arg(variable.name);
    if (variable.type && !variable.type->isEmpty()) {
        text += QStringLiteral("<em>%1</em>: %2").arg(i18n("type")).arg(variable.type.value());
    }
    return text;
}

static QString valueTip(const dap::Variable &variable)
{
    QString text;
    if (variable.indexedVariables.value_or(0) > 0) {
        text + QStringLiteral("<em>%1</em>: %2").arg(i18n("indexed items")).arg(variable.indexedVariables.value());
    }
    if (variable.namedVariables.value_or(0) > 0) {
        text + QStringLiteral("<em>%1</em>: %2").arg(i18n("named items")).arg(variable.namedVariables.value());
    }
    text += QStringLiteral("<qt>%1<qt>").arg(variable.value);
    return text;
}

static void formatName(QTreeWidgetItem &item, const dap::Variable &variable)
{
    QFont font = item.font(LocalsView::Column_Symbol);
    font.setBold(variable.valueChanged.value_or(false));
    item.setFont(LocalsView::Column_Symbol, font);
}

static QTreeWidgetItem *pendingDataChild(QTreeWidgetItem *parent)
{
    auto item = new QTreeWidgetItem(parent, LocalsView::PendingDataItem);
    item->setText(LocalsView::Column_Symbol, i18n("Loading..."));
    item->setText(LocalsView::Column_Value, i18n("Loading..."));
    return item;
}

QTreeWidgetItem *LocalsView::createWrappedItem(QTreeWidgetItem *parent, const dap::Variable &variable)
{
    auto *item = new QTreeWidgetItem(parent, QStringList(variable.name));
    formatName(*item, variable);
    if (!variable.value.isEmpty()) {
        item->setText(Column_Value, variable.value);
    }
    item->setData(Column_Value, Qt::UserRole, variable.value);
    if (variable.variablesReference > 0) { // if the is > 0, we can expand this item
        item->setData(Column_Value, VariableReference, variable.variablesReference);
        item->addChild(pendingDataChild(item));
    }
    item->setText(Column_Type, variable.type.value_or(QString()));

    item->setToolTip(Column_Symbol, nameTip(variable));
    item->setToolTip(Column_Value, valueTip(variable));

    return item;
}

QTreeWidgetItem *LocalsView::createWrappedItem(QTreeWidget *parent, const dap::Variable &variable)
{
    auto *item = new QTreeWidgetItem(parent, QStringList(variable.name));
    formatName(*item, variable);
    if (!variable.value.isEmpty()) {
        item->setText(Column_Value, variable.value);
    }
    item->setText(Column_Type, variable.type.value_or(QString()));

    item->setToolTip(Column_Symbol, nameTip(variable));
    item->setToolTip(Column_Value, valueTip(variable));

    if (variable.variablesReference > 0) { // if the is > 0, we can expand this item
        item->setData(Column_Value, VariableReference, variable.variablesReference);
        item->addChild(pendingDataChild(item));
    }

    return item;
}

void LocalsView::openVariableScope()
{
    clear();
    m_variables.clear();
}

void LocalsView::closeVariableScope()
{
    if (m_variables.count() == 1) {
        // Auto-expand if there is a single item
        QTreeWidgetItem *item = m_variables.begin().value();
        item->setExpanded(true);
    }
}

void LocalsView::addVariableLevel(int parentId, const dap::Variable &variable)
{
    QTreeWidgetItem *item = nullptr;

    if (parentId == 0) {
        item = createWrappedItem(this, variable);
    } else {
        if (!m_variables.contains(parentId)) {
            qDebug("unknown variable reference: %d", parentId);
            return;
        }
        item = createWrappedItem(m_variables[parentId], variable);
    }

    if (variable.variablesReference > 0) {
        m_variables[variable.variablesReference] = item;
    }
}

void LocalsView::onContextMenu(QPoint pos)
{
    QMenu menu(this);

    if (auto item = currentItem()) {
        auto a = menu.addAction(i18n("Copy Symbol"));
        connect(a, &QAction::triggered, this, [item] {
            qApp->clipboard()->setText(item->text(0).trimmed());
        });

        QString value = item->data(Column_Value, Qt::UserRole).toString();
        if (value.isEmpty()) {
            if (itemWidget(item, Column_Value)) {
                auto label = qobject_cast<QLabel *>(itemWidget(item, 1));
                value = label ? label->text() : QString();
            }
        }

        if (!value.isEmpty()) {
            auto a = menu.addAction(i18n("Copy Value"));
            connect(a, &QAction::triggered, this, [value] {
                qApp->clipboard()->setText(value);
            });
        }
    }

    menu.exec(viewport()->mapToGlobal(pos));
}

void LocalsView::onItemExpanded(QTreeWidgetItem *item)
{
    const int childCount = item->childCount();
    for (int i = 0; i < childCount; ++i) {
        if (item->child(i)->type() == PendingDataItem) {
            item->removeChild(item->child(i));
            Q_EMIT requestVariable(item->data(Column_Value, VariableReference).toInt());
            break;
        }
    }
}

#include "moc_localsview.cpp"
