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

#ifndef KTEXTEDITOR_APPLICATION_H
#define KTEXTEDITOR_APPLICATION_H

#include <ktexteditor/ktexteditor_export.h>

#include <QObject>

namespace KTextEditor
{
  
class MainWindow;
  
/**
 * This class allows the application that embedds the KTextEditor component to
 * allow it access to application wide information and interactions.
 * 
 * For example the component can get the current active main window of the application.
 * 
 * The application must pass a pointer to the Application object to the setApplication method of the
 * global editor instance and ensure that this object stays valid for the complete lifetime of the editor.
 * 
 * It must not reimplement this class but construct an instance and pass a pointer to a QObject that
 * has the required slots to receive the requests.
 * 
 * The interface functions are nullptr safe, this means, you can call them all even if the instance
 * is a nullptr.
 */
class KTEXTEDITOR_EXPORT Application : public QObject
{
  Q_OBJECT
  
  public:
    /**
     * Construct an Application wrapper object.
     * The passed parent is both the parent of this QObject and the receiver of all interface
     * calls via invokeMethod.
     * @param parent object the calls are relayed to
     */
    Application (QObject *parent);
    
    /**
     * Virtual Destructor
     */
    virtual ~Application ();
    
  //
  // MainWindow related accessors
  //
  public:
      /**
       * Get a list of all main windows.
       * @return all main windows
       */
      QList<KTextEditor::MainWindow *> mainWindows ();
      
      /**
       * Accessor to the active main window.
       * \return a pointer to the active mainwindow
       */
      KTextEditor::MainWindow *activeMainWindow ();

  private:
    /**
     * Private d-pointer class is our best friend ;)
     */
    friend class ApplicationPrivate;
    
    /**
     * Private d-pointer
     */
    class ApplicationPrivate * const d;
};

} // namespace KTextEditor

#endif
