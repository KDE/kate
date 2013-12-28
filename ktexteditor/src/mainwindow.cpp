/* 
 *  This file is part of the KDE project.
 * 
 *  Copyright (C) 2007 Philippe Fremy (phil at freehackers dot org)
 *  Copyright (C) 2008 Joseph Wenninger (jowenn@kde.org)
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
 *
 *  Based on code of the SmartCursor/Range by:
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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
