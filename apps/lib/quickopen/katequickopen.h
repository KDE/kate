/*
    SPDX-FileCopyrightText: 2007, 2009 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "katequickopenlineedit.h"

#include <QPointer>

class KateMainWindow;

class QModelIndex;
class QStandardItemModel;
class QSortFilterProxyModel;
class QuickOpenStyleDelegate;
class QTreeView;
class KateQuickOpenModel;
enum KateQuickOpenModelList : int;

class QuickOpenFilterProxyModel;

class KateQuickOpen : public QFrame
{
public:
    KateQuickOpen(KateMainWindow *mainWindow);
    ~KateQuickOpen() override;

    /**
     * update state
     * will fill model with current open documents, project documents, ...
     */
    void updateState();
    void updateViewGeometry();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reselectFirst();

    /**
     * Return pressed, activate the selected document
     * and go back to background
     */
    void slotReturnPressed();

    void slotListModeChanged(KateQuickOpenModelList mode);

    void setFilterMode();

private:
    KateMainWindow *m_mainWindow;
    QTreeView *m_listView;
    QuickOpenLineEdit *m_inputLine;
    QuickOpenStyleDelegate *m_styleDelegate;

    /**
     * our model we search in
     */
    KateQuickOpenModel *m_model = nullptr;

    /**
     * fuzzy filter model
     */
    QuickOpenFilterProxyModel *m_proxyModel = nullptr;
    QPointer<QWidget> m_previouslyFocusedWidget;
};
