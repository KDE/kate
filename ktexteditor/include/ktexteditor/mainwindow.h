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

#ifndef KTEXTEDITOR_MAINWINDOW_H
#define KTEXTEDITOR_MAINWINDOW_H

#include <ktexteditor/ktexteditor_export.h>

#include <QObject>

namespace KTextEditor
{
  
class View;
  
/**
 * This class allows the application that embedds the KTextEditor component to
 * allow it to access parts of its main window.
 * 
 * For example the component can get a place to show view bar widgets (e.g. search&replace, goto line, ...).
 * This is useful to e.g. have one place inside the window to show such stuff even if the application allows
 * the user to have multiple split views avaible per window.
 * 
 * The application must pass a pointer to the MainWindow object to the createView method on view creation
 * and ensure that this main window stays valid for the complete lifetime of the view.
 * 
 * It must not reimplement this class but construct an instance and pass a pointer to a QObject that
 * has the required slots to receive the requests.
 * 
 * The interface functions are nullptr safe, this means, you can call them all even if the instance
 * is a nullptr.
 */
class KTEXTEDITOR_EXPORT MainWindow : public QObject
{
  public:
    /**
     * Construct an MainWindow wrapper object.
     * The passed parent is both the parent of this QObject and the receiver of all interface
     * calls via invokeMethod.
     * @param parent object the calls are relayed to
     */
    MainWindow (QObject *parent);
    
    /**
     * Virtual Destructor
     */
    virtual ~MainWindow ();
    
    /**
     * Try to create a view bar for the given view.
     * @param view view for which we want an view bar
     * @return suitable widget that can host view bars widgets or nullptr
    */
    QWidget *createViewBar (KTextEditor::View *view);

    /**
     * Delete the view bar for the given view.
     * @param view view for which we want an view bar
     */
    void deleteViewBar (KTextEditor::View *view);

    /**
     * Add a widget to the view bar.
     * @param view view for which the view bar is used
     * @param bar bar widget, shall have the viewBarParent() as parent widget
     */
    void addWidgetToViewBar (KTextEditor::View *view, QWidget *bar);
    
    /**
     * Show the view bar for the given view
     * @param view view for which the view bar is used
     */
    void showViewBar (KTextEditor::View *view);
    
    /**
     * Hide the view bar for the given view
     * @param view view for which the view bar is used
     */
    void hideViewBar (KTextEditor::View *view);
    
  private:
    /**
     * Private d-pointer class is our best friend ;)
     */
    friend class MainWindowPrivate;
    
    /**
     * Private d-pointer
     */
    class MainWindowPrivate * const d;
};

} // namespace KTextEditor

#endif
