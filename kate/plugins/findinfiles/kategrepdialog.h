/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Dominik Haumann <dhaumann@kde.org>
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _GREPDIALOG_H_
#define _GREPDIALOG_H_

#include <kate/mainwindow.h>

#include "kategrepthread.h"
#include "ui_findwidget.h"

#include <QEvent>

class QToolButton;
class QShowEvent;
class QTreeWidgetItem;
class KConfig;
class KateFindInFilesView;

class KateGrepDialog : public QWidget, private Ui::FindWidget
{
    Q_OBJECT

  public:
    KateGrepDialog(QWidget *parent, Kate::MainWindow *mw, KateFindInFilesView *view);
    ~KateGrepDialog();

    // read and write session config
    void readSessionConfig (const KConfigGroup& config);
    void writeSessionConfig (KConfigGroup& config);

  protected:
    bool eventFilter( QObject *, QEvent * );
    void showEvent(QShowEvent* event);
    void keyPressEvent(QKeyEvent *event);

    void addItems();

  private Q_SLOTS:
    void itemSelected(QTreeWidgetItem *item, int column);
    void slotCloseResultTab();
    void slotCloseResultTab(QWidget*);
    void slotSearch();
    void slotClear();
    void patternTextChanged( const QString &);
    void searchFinished ();
    void searchMatchFound(const QString &filename, const QString &relname, const QList<int> &lines, const QList<int> &columns, const QString &basename, const QStringList &lineContent, QWidget *parentTab);
    void syncDir();

  private:
    void killThread ();

    Kate::MainWindow *m_mw;
    QStringList lastSearchItems;
    QStringList lastSearchPaths;
    QStringList lastSearchFiles;
    QToolButton* btnCloseTab;

    KateGrepThread *m_grepThread;
    KateFindInFilesView *m_view;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
