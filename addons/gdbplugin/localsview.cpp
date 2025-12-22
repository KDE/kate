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
#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
enum Role {
    VariableReference = Qt::UserRole + 1,
    IsResolved,
};
enum TreeItemType {
    PendingDataItem = QTreeWidgetItem::UserType + 1
};
enum Column {
    Column_Symbol = 0,
    Column_Type = 1,
    Column_Value = 2,
};

QString nameTip(const dap::Variable &variable)
{
    QString text = QStringLiteral("<qt>%1<qt>").arg(variable.name);
    if (variable.type && !variable.type->isEmpty()) {
        text += QStringLiteral("<em>%1</em>: %2").arg(i18n("type")).arg(variable.type.value());
    }
    return text;
}

QString valueTip(const dap::Variable &variable)
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

void formatName(QTreeWidgetItem &item, const dap::Variable &variable)
{
    QFont font = item.font(Column::Column_Symbol);
    font.setBold(variable.valueChanged.value_or(false));
    item.setFont(Column::Column_Symbol, font);
}

QTreeWidgetItem *pendingDataChild(QTreeWidgetItem *parent)
{
    auto item = new QTreeWidgetItem(parent, TreeItemType::PendingDataItem);
    item->setText(Column::Column_Symbol, i18n("Loading..."));
    item->setText(Column::Column_Value, i18n("Loading..."));
    return item;
}

QTreeWidgetItem *createWrappedItem(const dap::Variable &variable)
{
    auto *item = new QTreeWidgetItem(QStringList(variable.name));
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
}

LocalsView::LocalsView(QWidget *parent)
    : QWidget(parent)
    , m_treeWidget(new QTreeWidget(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    layout->addWidget(m_treeWidget);

    QStringList headers;
    headers << i18n("Symbol");
    headers << i18n("Type");
    headers << i18n("Value");
    m_treeWidget->setHeaderLabels(headers);
    m_treeWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_treeWidget->setUniformRowHeights(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &LocalsView::onTreeWidgetContextMenu);
    connect(m_treeWidget, &QTreeWidget::itemExpanded, this, &LocalsView::onItemExpanded);
}

LocalsView::~LocalsView() = default;

void LocalsView::clear()
{
    m_treeWidget->clear();
}

void LocalsView::insertScopes(const QList<dap::Scope> &scopes)
{
    m_treeWidget->clear();
    m_variables.clear();

    QList<QTreeWidgetItem *> topLevelItems;
    topLevelItems.reserve(scopes.size());

    for (const auto &scope : scopes) {
        auto item = new QTreeWidgetItem(QStringList(scope.name));
        item->setData(0, VariableReference, scope.variablesReference);
        item->addChild(pendingDataChild(item));
        topLevelItems.push_back(item);
    }

    m_treeWidget->addTopLevelItems(topLevelItems);
}

void LocalsView::showEvent(QShowEvent *)
{
    Q_EMIT localsVisible(true);
}

void LocalsView::hideEvent(QHideEvent *)
{
    Q_EMIT localsVisible(false);
}

void LocalsView::addVariables(int variableReference, const QList<dap::Variable> &variables)
{
    QTreeWidgetItem *root = nullptr;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); i++) {
        auto item = m_treeWidget->topLevelItem(i);
        if (item && item->data(0, VariableReference).toInt() == variableReference) {
            root = item;
            break;
        }
    }

    const bool isTopLevel = root != nullptr;
    if (isTopLevel) {
        // resolved now
        root->setData(0, Role::IsResolved, true);
        // We should have only 1 item i.e., pendingDataChild
        Q_ASSERT(root->childCount() == 1);
        root->removeChild(root->child(0));
    } else {
        const auto varIt = m_variables.constFind(variableReference);
        if (varIt == m_variables.cend()) {
            return;
        }
        root = varIt.value();
    }

    QList<QTreeWidgetItem *> items;
    items.reserve(variables.size());
    for (const auto &variable : variables) {
        QTreeWidgetItem *item = createWrappedItem(variable);
        if (variable.variablesReference > 0) {
            m_variables[variable.variablesReference] = item;
        }
        items.push_back(item);
    }
    root->addChildren(items);
}

void LocalsView::onTreeWidgetContextMenu(QPoint pos)
{
    QMenu menu(this);

    if (auto item = m_treeWidget->currentItem()) {
        {
            auto a = menu.addAction(i18n("Copy Symbol"));
            connect(a, &QAction::triggered, this, [item] {
                qApp->clipboard()->setText(item->text(0).trimmed());
            });
        }

        QString value = item->data(Column_Value, Qt::UserRole).toString();
        if (value.isEmpty()) {
            if (m_treeWidget->itemWidget(item, Column_Value)) {
                auto label = qobject_cast<QLabel *>(m_treeWidget->itemWidget(item, 1));
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

    menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
}

void LocalsView::onItemExpanded(QTreeWidgetItem *item)
{
    const bool isTopLevel = m_treeWidget->indexOfTopLevelItem(item) != -1;
    if (isTopLevel && !item->data(0, Role::IsResolved).toBool()) {
        int scope = item->data(0, Role::VariableReference).toInt();
        Q_EMIT scopeChanged(scope);
    } else {
        const int childCount = item->childCount();
        for (int i = 0; i < childCount; ++i) {
            if (item->child(i)->type() == PendingDataItem) {
                item->removeChild(item->child(i));
                Q_EMIT requestVariable(item->data(Column_Value, VariableReference).toInt());
                break;
            }
        }
    }
}

#include "moc_localsview.cpp"
