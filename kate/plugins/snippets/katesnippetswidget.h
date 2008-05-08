/* This file is part of the KDE project
   Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
   Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KATESNIPPETSWIDGET_H
#define KATESNIPPETSWIDGET_H

#include <KateSnippetsWidgetBase.h>

#include "katesnippet.h"

#include <kate/mainwindow.h>
#include <q3ptrlist.h>


class KateSnippetsWidget : public KateSnippetsWidgetBase
{
  Q_OBJECT

  public:
    explicit KateSnippetsWidget( Kate::MainWindow *mainWindow = 0,
                                 QWidget* parent = 0, const char* name = 0,
                                 Qt::WFlags fl = 0 );
    virtual ~KateSnippetsWidget();

    KateSnippet* findSnippetByListViewItem( Q3ListViewItem *item );

    void readConfig( KConfigBase* config, const QString& groupPrefix );
    void writeConfig( KConfigBase* config, const QString& groupPrefix );

    Kate::MainWindow* mainWindow() { return m_mainWin; }

  public Q_SLOTS:
    void slotSnippetSelectionChanged( Q3ListViewItem  *item );
    void slotSnippetItemClicked ( Q3ListViewItem *item );
    void slotSnippetItemRenamed( Q3ListViewItem *lvi, int col, const QString& text );
    void slotNewClicked();
    void slotSaveClicked();
    void slotDeleteClicked();

  Q_SIGNALS:
    void saveRequested();

  private:
    Kate::MainWindow *m_mainWin;
    QList<KateSnippet *> m_snippets;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
