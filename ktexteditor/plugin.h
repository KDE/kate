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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __ktexteditor_plugin_h__
#define __ktexteditor_plugin_h__

#include <qobject.h>

#include <kdelibs_export.h>

namespace KTextEditor
{

class Document;
class View;

/**
 * Basic KTextEditor plugin class.
 * This plugin will be bound to a Document.
 */                     
class KTEXTEDITOR_EXPORT Plugin : public QObject
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
   
KTEXTEDITOR_EXPORT Plugin *createPlugin ( const char* libname, Document *document = 0, const char *name = 0 );

/**
 * View plugin class.
 * This plugin will be bound to a View
 */
class KTEXTEDITOR_EXPORT PluginViewInterface
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

KTEXTEDITOR_EXPORT PluginViewInterface *pluginViewInterface (Plugin *plugin);

}

#endif
