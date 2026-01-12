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
#include <QLabel>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
enum Role {
    VariableReference = Qt::UserRole + 1,
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
        text += QStringLiteral("<em>%1</em>: %2").arg(i18n("type"), variable.type.value());
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

int pendingDataChildIndex(QTreeWidgetItem *item)
{
    const int childCount = item->childCount();
    for (int i = 0; i < childCount; ++i) {
        if (item->child(i)->type() == PendingDataItem) {
            return i;
        }
    }
    return -1;
}

void initItem(QTreeWidgetItem *item, const dap::Variable &variable)
{
    item->setText(Column_Symbol, variable.name);
    formatName(*item, variable);
    if (!variable.value.isEmpty()) {
        item->setText(Column_Value, variable.value);
    }
    item->setData(Column_Value, Qt::UserRole, variable.value);
    if (variable.variablesReference > 0) { // if the is > 0, we can expand this item
        item->setData(Column_Value, VariableReference, variable.variablesReference);
        if (int pendingIndex = pendingDataChildIndex(item); pendingIndex != -1) {
            item->addChild(pendingDataChild(item));
        }
    }

    item->setText(Column_Type, variable.type.value_or(QString()));

    item->setToolTip(Column_Symbol, nameTip(variable));
    item->setToolTip(Column_Value, valueTip(variable));
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
    // clear variableReference -> QTreeWidgetItem cache
    m_variables.clear();

    if (scopes.isEmpty()) {
        m_treeWidget->clear();
        return;
    }

    const int topLevelItemCount = m_treeWidget->topLevelItemCount();
    QVarLengthArray<QString, 8> expandedScopes;
    for (int i = 0; i < topLevelItemCount; i++) {
        auto item = m_treeWidget->topLevelItem(i);
        if (item && item->isExpanded()) {
            expandedScopes.push_back({item->text(0)});
        }
    }

    // Remove extra items
    if (topLevelItemCount > scopes.size()) {
        const int toRemove = topLevelItemCount - scopes.size();
        for (int i = 0; i < toRemove; i++) {
            delete m_treeWidget->takeTopLevelItem(m_treeWidget->topLevelItemCount() - 1);
        }
    } else {
        const int toAdd = scopes.size() - topLevelItemCount;
        QList<QTreeWidgetItem *> topLevelItems;
        for (int i = 0; i < toAdd; i++) {
            auto item = new QTreeWidgetItem(QStringList());
            item->addChild(pendingDataChild(item));
            topLevelItems.push_back(item);
        }
        m_treeWidget->addTopLevelItems(topLevelItems);
    }

    // We don't want to handle expansion outside this loop, so disconnect
    disconnect(m_treeWidget, &QTreeWidget::itemExpanded, this, &LocalsView::onItemExpanded);

    QVarLengthArray<int, 8> scopesToRequestVarsFor;
    // update
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); i++) {
        auto item = m_treeWidget->topLevelItem(i);
        const auto &scope = scopes[i];
        item->setText(0, scope.name);
        item->setData(0, VariableReference, scope.variablesReference);
        // if this scope was expanded previously, then expand it again
        // and request its variables
        if (expandedScopes.contains(scope.name)) {
            scopesToRequestVarsFor.push_back(scope.variablesReference);
            item->setExpanded(true);
        } else if (item->isExpanded()) {
            // else if it was not expanded before, collapse it
            item->setExpanded(false);
        }
    }

    connect(m_treeWidget, &QTreeWidget::itemExpanded, this, &LocalsView::onItemExpanded);

    for (int variablesRef : scopesToRequestVarsFor) {
        Q_EMIT requestVariable(variablesRef);
    }
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

    // if not a top level item, look up in variables
    if (root == nullptr) {
        const auto varIt = m_variables.constFind(variableReference);
        if (varIt == m_variables.cend()) {
            return;
        }
        root = varIt.value();
    }

    // We must have a root by now
    Q_ASSERT(root);

    // Remove the pending data child of this root now, we have retrieved the data
    if (int childIndex = pendingDataChildIndex(root); childIndex >= 0) {
        root->removeChild(root->child(childIndex));
    }

    const int itemChildCount = root->childCount();
    QVarLengthArray<QString, 8> expandedVariables;

    for (int i = 0; i < itemChildCount; i++) {
        auto child = root->child(i);
        if (child && child->isExpanded()) {
            expandedVariables.push_back({child->text(0)});
        }
    }

    // Remove extra items
    if (itemChildCount > variables.size()) {
        const int toRemove = itemChildCount - variables.size();
        for (int i = 0; i < toRemove; i++) {
            root->removeChild(root->child(root->childCount() - 1));
        }
    } else {
        const int toAdd = variables.size() - itemChildCount;
        QList<QTreeWidgetItem *> itemsToAdd;
        for (int i = 0; i < toAdd; i++) {
            auto item = new QTreeWidgetItem(QStringList());
            itemsToAdd.push_back(item);
        }
        root->addChildren(itemsToAdd);
    }

    // We don't want to handle expansion outside this loop, so disconnect
    disconnect(m_treeWidget, &QTreeWidget::itemExpanded, this, &LocalsView::onItemExpanded);

    QVarLengthArray<int, 8> varsToRequestVarsFor;

    // update
    Q_ASSERT(root->childCount() == variables.size());
    for (int i = 0; i < variables.size(); i++) {
        auto item = root->child(i);
        const auto &variable = variables[i];
        initItem(item, variable);

        if (variable.variablesReference > 0) {
            m_variables[variable.variablesReference] = item;
        }

        // If this var was expanded previously, expand it again
        // and request its variables
        if (expandedVariables.contains(variable.name)) {
            item->setExpanded(true);
            varsToRequestVarsFor.push_back(variable.variablesReference);
        }
        // If old var was expanded, but after update we didn't find it in expandedVariables
        // collapse it
        else if (item->isExpanded() && variable.variablesReference > 0) {
            item->setExpanded(false);
        }
        // else if there are no children for this variable, but old var had children
        // clear those children
        else if (item->childCount() > 0 && variable.variablesReference == 0) {
            qDeleteAll(item->takeChildren());
        }
    }

    connect(m_treeWidget, &QTreeWidget::itemExpanded, this, &LocalsView::onItemExpanded);

    for (int variablesRef : varsToRequestVarsFor) {
        Q_EMIT requestVariable(variablesRef);
    }
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
    int index = pendingDataChildIndex(item);
    if (index >= 0) {
        const bool isTopLevel = m_treeWidget->indexOfTopLevelItem(item) != -1;
        if (isTopLevel) {
            Q_EMIT requestVariable(item->data(0, VariableReference).toInt());
        } else {
            Q_EMIT requestVariable(item->data(Column_Value, VariableReference).toInt());
        }
    }
}

#include "moc_localsview.cpp"
