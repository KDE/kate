/* This file is part of the KDE libraries
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ktexteditor_plugin_h__
#define __ktexteditor_plugin_h__

#include <qwidget.h>
#include <kxmlguiclient.h>

/*

THIS IS A TEXT VERSION OF THIS INTERFACE
THIS WILL CHANGE IN A VERY VERY VERY BIC WAY BEFORE 3.1 !!!!!!!!!!!!!
DON'T USE IT ANYWHERE BE THIS COMMENT HAS GONE AWAY

by cullmann

*/

namespace KTextEditor
{

/*
 * basic plugin class
 * this plugin will be bound to the qobject which holds the ktexteditor document interfaces
 * but which must not be necessarily a ktexteditor::document (could be a ktexteditor::editor, too)
 */
class Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    Plugin ( QObject *parent = 0, const char *name = 0 );
    virtual ~Plugin ();
    
    unsigned int pluginNumber () const;
    
    /*
     * This function will be called by the ktexteditor implementation after the plugin is loaded
     * to bound it to a ktexteditor::document
     */
    virtual void setDocument (class Document *) = 0;
  
  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};

/*
 * view plugin class
 * this plugin will be bound to a ktexteditor::view in addition to the ktexteditor::document
 */
class ViewPlugin : public Plugin, virtual public KXMLGUIClient
{
  friend class PrivateViewPlugin;

  Q_OBJECT

  public:
    ViewPlugin ( QObject *parent = 0, const char *name = 0 );
    virtual ~ViewPlugin ();
    
    unsigned int viewPluginNumber () const;
  
    /*
     * This function will be called by the ktexteditor implementation after the plugin is loaded
     * to bound it to a object holding the interfaces defined for ktexteditor view
     */
    virtual void setView (class View *) = 0;

  private:
    class PrivateViewPlugin *d;
    static unsigned int globalViewPluginNumber;
    unsigned int myViewPluginNumber;
};

};

#endif
