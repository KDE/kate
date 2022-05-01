//
// Description: Widget that local variables of the gdb inferior
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#ifndef LOCALSVIEW_H
#define LOCALSVIEW_H

#include "dap/entities.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>

class LocalsView : public QTreeWidget
{
    Q_OBJECT
public:
    LocalsView(QWidget *parent = nullptr);
    ~LocalsView() override;

public Q_SLOTS:
    // An empty value string ends the locals
    void openVariableScope();
    void closeVariableScope();
    void addVariableLevel(int parentId, const dap::Variable &variable);

Q_SIGNALS:
    void localsVisible(bool visible);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    QTreeWidgetItem *createWrappedItem(QTreeWidgetItem *parent, const dap::Variable &variable);
    QTreeWidgetItem *createWrappedItem(QTreeWidget *parent, const dap::Variable &variable);

    QHash<int, QTreeWidgetItem *> m_variables;
};

#endif
