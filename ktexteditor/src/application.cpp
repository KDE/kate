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

#include <ktexteditor/application.h>

namespace KTextEditor
{

Application::Application (QObject *parent)
  : QObject (parent)
  , d (nullptr)
{
}
    
Application::~Application ()
{
}

QList<KTextEditor::MainWindow *> Application::mainWindows ()
{
  /**
   * null check
   */
  if (!this)
    return QList<KTextEditor::MainWindow *> ();
  
  /**
   * dispatch to parent
   */
  QList<KTextEditor::MainWindow *> mainWindow;
  QMetaObject::invokeMethod (parent()
    , "mainWindows"
    , Qt::DirectConnection
    , Q_RETURN_ARG (QList<KTextEditor::MainWindow *>, mainWindow));
  return mainWindow;
}
      
KTextEditor::MainWindow *Application::activeMainWindow ()
{
  /**
   * null check
   */
  if (!this)
    return nullptr;
  
  /**
   * dispatch to parent
   */
  KTextEditor::MainWindow *window = nullptr;
  QMetaObject::invokeMethod (parent()
    , "activeMainWindow"
    , Qt::DirectConnection
    , Q_RETURN_ARG (KTextEditor::MainWindow *, window));
  return window;
}

} // namespace KTextEditor
