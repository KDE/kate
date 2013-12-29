/* 
 *  This file is part of the KDE project.
 * 
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <ktexteditor/mainwindow.h>

namespace KTextEditor
{

MainWindow::MainWindow (QObject *parent)
  : QObject (parent)
  , d (nullptr)
{
}
    
MainWindow::~MainWindow ()
{
}

QWidget *MainWindow::window () const
{
  /**
   * null check
   */
  if (!this)
    return nullptr;
  
  /**
   * dispatch to parent
   */
  QWidget *window = nullptr;
  QMetaObject::invokeMethod (parent()
    , "window"
    , Qt::DirectConnection
    , Q_RETURN_ARG (QWidget *, window));
  return window;
}

QList<KTextEditor::View *> MainWindow::views () const
{
  /**
   * null check
   */
  if (!this)
    return QList<KTextEditor::View *> ();
  
  /**
   * dispatch to parent
   */
  QList<KTextEditor::View *> views;
  QMetaObject::invokeMethod (parent()
    , "views"
    , Qt::DirectConnection
    , Q_RETURN_ARG (QList<KTextEditor::View *>, views));
  return views;
}
      
KTextEditor::View *MainWindow::activeView () const
{
  /**
   * null check
   */
  if (!this)
    return nullptr;
  
  /**
   * dispatch to parent
   */
  KTextEditor::View *view = nullptr;
  QMetaObject::invokeMethod (parent()
    , "activeView"
    , Qt::DirectConnection
    , Q_RETURN_ARG (KTextEditor::View *, view));
  return view;
}

KTextEditor::View *MainWindow::activateView (KTextEditor::Document *document)
{
  /**
   * null check
   */
  if (!this)
    return nullptr;
  
  /**
   * dispatch to parent
   */
  KTextEditor::View *view = nullptr;
  QMetaObject::invokeMethod (parent()
    , "activeView"
    , Qt::DirectConnection
    , Q_RETURN_ARG (KTextEditor::View *, view)
    , Q_ARG (KTextEditor::Document *, document));
  return view;
}

KTextEditor::View *MainWindow::openUrl (const QUrl &url, const QString &encoding)
{
  /**
   * null check
   */
  if (!this)
    return nullptr;
  
  /**
   * dispatch to parent
   */
  KTextEditor::View *view = nullptr;
  QMetaObject::invokeMethod (parent()
    , "openUrl"
    , Qt::DirectConnection
    , Q_RETURN_ARG (KTextEditor::View *, view)
    , Q_ARG (const QUrl &, url)
    , Q_ARG (const QString &, encoding));
  return view;
}
 
QWidget *MainWindow::createViewBar (KTextEditor::View *view)
{
  /**
   * null check
   */
  if (!this)
    return nullptr;
  
  /**
   * dispatch to parent
   */
  QWidget *viewBar = nullptr;
  QMetaObject::invokeMethod (parent()
    , "createViewBar"
    , Qt::DirectConnection
    , Q_RETURN_ARG (QWidget *, viewBar)
    , Q_ARG (KTextEditor::View *, view));
  return viewBar;
}

void MainWindow::deleteViewBar (KTextEditor::View *view)
{
  /**
   * null check
   */
  if (!this)
    return;
  
  /**
   * dispatch to parent
   */
  QMetaObject::invokeMethod (parent()
    , "deleteViewBar"
    , Qt::DirectConnection
    , Q_ARG (KTextEditor::View *, view));
}

void MainWindow::addWidgetToViewBar (KTextEditor::View *view, QWidget *bar)
{
  /**
   * null check
   */
  if (!this)
    return;
  
  /**
   * dispatch to parent
   */
  QMetaObject::invokeMethod (parent()
    , "addWidgetToViewBar"
    , Qt::DirectConnection
    , Q_ARG (KTextEditor::View *, view)
    , Q_ARG (QWidget *, bar));
}
 
void MainWindow::showViewBar (KTextEditor::View *view)
{
  /**
   * null check
   */
  if (!this)
    return;
  
  /**
   * dispatch to parent
   */
  QMetaObject::invokeMethod (parent()
    , "showViewBar"
    , Qt::DirectConnection
    , Q_ARG (KTextEditor::View *, view));
}
    
void MainWindow::hideViewBar (KTextEditor::View *view)
{
  /**
   * null check
   */
  if (!this)
    return;
  
  /**
   * dispatch to parent
   */
  QMetaObject::invokeMethod (parent()
    , "hideViewBar"
    , Qt::DirectConnection
    , Q_ARG (KTextEditor::View *, view));
}

} // namespace KTextEditor
