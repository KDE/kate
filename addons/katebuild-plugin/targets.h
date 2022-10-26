//
// Description: Widget for configuring build targets
//
// SPDX-FileCopyrightText: 2011-2022 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#ifndef TARGETS_H
#define TARGETS_H

#include "TargetFilterProxyModel.h"
#include "TargetHtmlDelegate.h"
#include "TargetModel.h"
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

class TargetsUi : public QWidget
{
    Q_OBJECT

public:
    TargetsUi(QObject *view, QWidget *parent = nullptr);

    QLabel *targetLabel = nullptr;
    QComboBox *targetCombo = nullptr;
    QLineEdit *targetFilterEdit = nullptr;
    QToolButton *newTarget = nullptr;
    QToolButton *copyTarget = nullptr;
    QToolButton *moveTargetUp = nullptr;
    QToolButton *moveTargetDown = nullptr;
    QToolButton *deleteTarget = nullptr;

    QTreeView *targetsView = nullptr;
    TargetModel targetsModel;
    TargetFilterProxyModel proxyModel;

    QToolButton *addButton = nullptr;
    QToolButton *buildButton = nullptr;
    QToolButton *runButton = nullptr;

    void updateTargetsButtonStates();

public Q_SLOTS:
    void targetActivated(const QModelIndex &index);

Q_SIGNALS:
    void enterPressed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:

    TargetHtmlDelegate *m_delegate = nullptr;
};

#endif
