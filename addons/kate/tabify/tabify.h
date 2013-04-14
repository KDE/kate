/***************************************************************************
 *   Copyright (C) 2010 by Abhishek Patil <abhishekworld@gmail.com>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/
#ifndef TABIFY_H
#define TABIFY_H

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/document.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <klocalizedstring.h>
#include <ktabbar.h>

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtGui/QTabBar>

class TabBarPluginView
      : public Kate::PluginView
{
  Q_OBJECT

public:
  explicit TabBarPluginView(Kate::MainWindow* mainwindow);
  virtual ~TabBarPluginView();

public slots:
  void slotDocumentCreated(KTextEditor::Document* doc);
  void slotTabChanged(int);
  void slotDocumentDeleted(KTextEditor::Document*);
  void slotViewChanged();
  void slotMiddleMouseButtonPressed(int);
  void slotTabCloseRequest(int);
  void slotDocumentChanged(KTextEditor::Document*);
  void slotModifiedOnDisc(KTextEditor::Document*, bool,
                          KTextEditor::ModificationInterface::ModifiedOnDiskReason);
  void slotNameChanged(KTextEditor::Document*);
  void slotWheelDelta(int);
  void slotTabMoved(int, int);

private:
  KTabBar* m_tabBar;
  QMap<int, KTextEditor::Document*> m_tabDocMap;
  QMap<KTextEditor::Document*, int> m_docTabMap;
  QList<KTextEditor::Document*> m_docList;
  QMap<KTextEditor::Document*, bool> m_modifiedMap;
  bool m_tabIsDeleting;
  void rebuildMaps();
};

class TabBarPlugin
      : public Kate::Plugin
{
  Q_OBJECT


public:
  explicit TabBarPlugin(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());
  virtual ~TabBarPlugin();

  Kate::PluginView *createView(Kate::MainWindow *mainWindow);

private:
  QList<TabBarPluginView*> m_views;
};

#endif // TABIFY_H
