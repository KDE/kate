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

class PluginView : public QObject, virtual public KXMLGUIClient
{
  friend class PrivatePluginView;

  Q_OBJECT

  public:
    PluginView ();
    virtual ~PluginView ();
    
  unsigned int pluginViewNumber () const;

  private:
    class PrivatePluginView *d;
    static unsigned int globalPluginViewNumber;
    unsigned int myPluginViewNumber;
};

class Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    /**
    * Create a new view to the given document. The document must be non-null.
    */
    Plugin ();
    virtual ~Plugin ();
    
    unsigned int pluginNumber () const;
    
    /*
     * Will be called for each new created KTextEditor::View/Editor and view and doc will
     * contain the classes with the view and doc interfaces (use casts to look which kind
       of interfaces are available)
     */
    virtual PluginView *createView (QObject *view, QObject *doc) = 0;
  
  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};

};

#endif
