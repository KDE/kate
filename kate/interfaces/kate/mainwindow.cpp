/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
 
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

#include "mainwindow.h"
#include "mainwindow.moc"

#include "documentmanager.h"
#include "plugin.h"
#include "pluginmanager.h"

#include "../app/katemainwindow.h"
#include "../app/kateviewmanager.h"

namespace Kate
{

  class PrivateMainWindow
  {
    public:
      PrivateMainWindow ()
      {}

      ~PrivateMainWindow ()
      {
      }

      KateMainWindow *win;
  };

  MainWindow::MainWindow (void *mainWindow) : QObject ((KateMainWindow*) mainWindow)
  {
    d = new PrivateMainWindow;
    d->win = (KateMainWindow*) mainWindow;
  }

  MainWindow::~MainWindow ()
  {
    delete d;
  }

  KXMLGUIFactory *MainWindow::guiFactory() const
  {
    return d->win->guiFactory();
  }

  QWidget *MainWindow::window() const
  {
    return d->win;
  }

  QWidget *MainWindow::centralWidget() const
  {
    return d->win->centralWidget();
  }

  KTextEditor::View *MainWindow::activeView()
  {
    return d->win->viewManager()->activeView();
  }

  KTextEditor::View *MainWindow::activateView ( KTextEditor::Document *doc )
  {
    return d->win->viewManager()->activateView( doc );
  }

  KTextEditor::View *MainWindow::openUrl (const KUrl &url, const QString &encoding)
  {
    return d->win->viewManager()->openUrlWithView (url, encoding);
  }

  QWidget *MainWindow::createToolView (const QString &identifier, MainWindow::Position pos, const QPixmap &icon, const QString &text)
  {
    return d->win->createToolView (identifier, (KMultiTabBar::KMultiTabBarPosition)pos, icon, text);
  }

  bool MainWindow::moveToolView (QWidget *widget, MainWindow::Position  pos)
  {
    if (!widget || !qobject_cast<KateMDI::ToolView*>(widget))
      return false;

    return d->win->moveToolView (qobject_cast<KateMDI::ToolView*>(widget), (KMultiTabBar::KMultiTabBarPosition)pos);
  }

  bool MainWindow::showToolView(QWidget *widget)
  {
    if (!widget || !qobject_cast<KateMDI::ToolView*>(widget))
      return false;

    return d->win->showToolView (qobject_cast<KateMDI::ToolView*>(widget));
  }

  bool MainWindow::hideToolView(QWidget *widget)
  {
    if (!widget || !qobject_cast<KateMDI::ToolView*>(widget))
      return false;

    return d->win->hideToolView (qobject_cast<KateMDI::ToolView*>(widget));
  }


}

// kate: space-indent on; indent-width 2; replace-tabs on;

