//
// Description: Widget that local variables of the gdb inferior
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#pragma once

#include <QWidget>

namespace dap
{
struct Variable;
struct Scope;
}
class QComboBox;
class QTreeWidgetItem;
class QTreeWidget;

class LocalsView : public QWidget
{
    Q_OBJECT
public:
    LocalsView(QWidget *parent = nullptr);
    ~LocalsView() override;

    void clear();
    void insertScopes(const QList<dap::Scope> &scopes);
    void addVariables(int variableReference, const QList<dap::Variable> &variables);

Q_SIGNALS:
    void localsVisible(bool visible);
    void requestVariable(int variableReference);
    void scopeChanged(int scope);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void onTreeWidgetContextMenu(QPoint pos);
    void onItemExpanded(QTreeWidgetItem *);

    QHash<int, QTreeWidgetItem *> m_variables;
    QComboBox *const m_scopeCombo;
    QTreeWidget *const m_treeWidget;
};
