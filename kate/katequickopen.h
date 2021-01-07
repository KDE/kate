/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2007, 2009 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_QUICK_OPEN_H
#define KATE_QUICK_OPEN_H

#include <QWidget>

class KateMainWindow;
class KLineEdit;

class QModelIndex;
class QStandardItemModel;
class QSortFilterProxyModel;
class QTreeView;
class KateQuickOpenModel;
enum KateQuickOpenModelList : int;

class QuickOpenFilterProxyModel;

class KateQuickOpen : public QWidget
{
    Q_OBJECT
public:
    KateQuickOpen(KateMainWindow *mainWindow);
    /**
     * update state
     * will fill model with current open documents, project documents, ...
     */
    void update();

    KateQuickOpenModelList listMode() const;
    void setListMode(KateQuickOpenModelList mode);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void reselectFirst();

    /**
     * Return pressed, activate the selected document
     * and go back to background
     */
    void slotReturnPressed();

private:
    KateMainWindow *m_mainWindow;
    QTreeView *m_listView;
    KLineEdit *m_inputLine;

    /**
     * our model we search in
     */
    KateQuickOpenModel *m_base_model;

    /**
     * filtered model we search in
     */
    QuickOpenFilterProxyModel *m_model;
    void updateViewGeometry();
};

#endif
