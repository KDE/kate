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

#include <qobject.h>

namespace KTextEditor
{                       

class Document;
class View;

/*
 * basic plugin class
 * this plugin will be bound to a ktexteditor::document
 */                     
class Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    Plugin ( Document *document = 0, const char *name = 0 );
    virtual ~Plugin ();
    
    unsigned int pluginNumber () const;
      
    Document *document () const;
    
  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};
   
Plugin *createPlugin ( const char* libname, Document *document = 0, const char *name = 0 );

/*
 * view plugin class
 * this plugin will be bound to a ktexteditor::view
 */
class PluginViewInterface
{
  friend class PrivatePluginViewInterface;

  public:
    PluginViewInterface ();
    virtual ~PluginViewInterface ();
    
    unsigned int pluginViewInterfaceNumber () const;
  
    /*
     * will be called from the part to bound the plugin to a view
     */
    virtual void addView (View *) = 0;
    virtual void removeView (View *) = 0;

  private:
    class PrivatePluginViewInterface *d;
    static unsigned int globalPluginViewInterfaceNumber;
    unsigned int myPluginViewInterfaceNumber;
};         

PluginViewInterface *pluginViewInterface (Plugin *plugin);

};

#endif
