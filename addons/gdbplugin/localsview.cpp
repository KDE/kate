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
};
enum TreeItemType {
    PendingDataItem = QTreeWidgetItem::UserType + 1
};
enum Column {
    Column_Symbol = 0,
    Column_Type = 1,
    Column_Value = 2,
};
}

LocalsView::LocalsView(QWidget *parent)
    : QWidget(parent)
    , m_scopeCombo(new QComboBox(this))
    , m_treeWidget(new QTreeWidget(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    layout->addWidget(m_scopeCombo);
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

    connect(m_scopeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0 && index < m_scopeCombo->count()) {
            Q_EMIT scopeChanged(m_scopeCombo->itemData(index).toInt());
        }
    });
}

LocalsView::~LocalsView()
{
}

void LocalsView::clear()
{
    m_scopeCombo->clear();
    m_treeWidget->clear();
}

void LocalsView::insertScopes(const QList<dap::Scope> &scopes)
{
    const int currentIndex = m_scopeCombo->currentIndex();

    m_scopeCombo->clear();

    for (const auto &scope : scopes) {
        QString name = scope.expensive.value_or(false) ? QStringLiteral("%1!").arg(scope.name) : scope.name;
        m_scopeCombo->addItem(QIcon::fromTheme(QStringLiteral("")).pixmap(10, 10), scope.name, scope.variablesReference);
    }

    if (currentIndex >= 0 && currentIndex < scopes.size()) {
        m_scopeCombo->setCurrentIndex(currentIndex);
    } else if (m_scopeCombo->count() > 0) {
        m_scopeCombo->setCurrentIndex(0);
    }
}

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
    QFont font = item.font(Column::Column_Symbol);
    font.setBold(variable.valueChanged.value_or(false));
    item.setFont(Column::Column_Symbol, font);
}

static QTreeWidgetItem *pendingDataChild(QTreeWidgetItem *parent)
{
    auto item = new QTreeWidgetItem(parent, TreeItemType::PendingDataItem);
    item->setText(Column::Column_Symbol, i18n("Loading..."));
    item->setText(Column::Column_Value, i18n("Loading..."));
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
    m_treeWidget->clear();
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
        item = createWrappedItem(m_treeWidget, variable);
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

void LocalsView::onTreeWidgetContextMenu(QPoint pos)
{
    QMenu menu(this);

    if (auto item = m_treeWidget->currentItem()) {
        auto a = menu.addAction(i18n("Copy Symbol"));
        connect(a, &QAction::triggered, this, [item] {
            qApp->clipboard()->setText(item->text(0).trimmed());
        });

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
