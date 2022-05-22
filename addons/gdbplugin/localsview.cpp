//
// Description: Widget that local variables of the gdb inferior
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "localsview.h"
#include <KLocalizedString>
#include <QDebug>
#include <QLabel>

LocalsView::LocalsView(QWidget *parent)
    : QTreeWidget(parent)
{
    QStringList headers;
    headers << i18n("Symbol");
    headers << i18n("Value");
    setHeaderLabels(headers);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

LocalsView::~LocalsView()
{
}

void LocalsView::showEvent(QShowEvent *)
{
    Q_EMIT localsVisible(true);
}

void LocalsView::hideEvent(QHideEvent *)
{
    Q_EMIT localsVisible(false);
}

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

QTreeWidgetItem *LocalsView::createWrappedItem(QTreeWidgetItem *parent, const dap::Variable &variable)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, QStringList(variable.name));
    QLabel *label = new QLabel(variable.value);
    label->setWordWrap(true);
    setItemWidget(item, 1, label);
    item->setData(1, Qt::UserRole, variable.value);
    item->setToolTip(0, nameTip(variable));
    item->setToolTip(1, valueTip(variable));

    return item;
}

QTreeWidgetItem *LocalsView::createWrappedItem(QTreeWidget *parent, const dap::Variable &variable)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, QStringList(variable.name));
    QLabel *label = new QLabel(variable.value);
    label->setWordWrap(true);
    setItemWidget(item, 1, label);
    item->setToolTip(0, nameTip(variable));
    item->setToolTip(1, valueTip(variable));

    return item;
}

void LocalsView::openVariableScope()
{
    clear();
    m_variables.clear();
}

void LocalsView::closeVariableScope()
{
}

void LocalsView::addVariableLevel(int parentId, const dap::Variable &variable)
{
    QTreeWidgetItem *item = nullptr;

    if (parentId == 0) {
        item = createWrappedItem(this, variable);
    } else {
        if (!m_variables.contains(parentId)) {
            qDebug() << "unknown variable reference:" << parentId;
            return;
        }
        item = createWrappedItem(m_variables[parentId], variable);
    }

    if (variable.variablesReference > 0) {
        m_variables[variable.variablesReference] = item;
    }
}
