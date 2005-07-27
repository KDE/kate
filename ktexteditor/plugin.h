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

class KConfig;

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
  Q_OBJECT

  public:
    /**
     * Plugin constructor
     * @param parent parent object
     */
    Plugin ( QObject *parent ) : QObject (parent) {}

    /**
     * virtual destructor
     */
    virtual ~Plugin () {}

  /**
   * Following methodes allow the plugin to react on view and document
   * creation
   */
  public:
    /**
     * this method is called if the plugin gui should be added
     * to the given KTextEditor::Document
     * @param document document to hang the gui in
     */
    virtual void addDocument (Document *document) { Q_UNUSED(document); }

    /**
     * this method is called if the plugin gui should be removed
     * from the given KTextEditor::Document
     * @param document document to hang the gui out from
     */
    virtual void removeDocument (Document *document) { Q_UNUSED(document); }

    /**
     * this method is called if the plugin gui should be added
     * to the given KTextEditor::View
     * @param view view to hang the gui in
     */
    virtual void addView (View *view) { Q_UNUSED(view); }

    /**
     * this method is called if the plugin gui should be removed
     * from the given KTextEditor::View
     * @param view view to hang the gui out from
     */
    virtual void removeView (View *view) { Q_UNUSED(view); }

  /**
   * Configuration management
   * Default implementation just for convenience, does nothing
   * and says this plugin supports no config dialog
   */
  public:
    /**
     * Read editor configuration from it's standard config
     */
    virtual void readConfig () {}

    /**
     * Write editor configuration to it's standard config
     */
    virtual void writeConfig () {}

    /**
     * Read editor configuration from given config object
     * @param config config object
     */
    virtual void readConfig (KConfig *config) { Q_UNUSED(config); }

    /**
     * Write editor configuration to given config object
     * @param config config object
     */
    virtual void writeConfig (KConfig *config) { Q_UNUSED(config); }

    /**
     * Does this plugin support a config dialog
     * @return does this plugin have a config dialog?
     */
    virtual bool configDialogSupported () const { return false; }

    /**
     * Shows a config dialog for the part, changes will be applied
     * to the editor, but not saved anywhere automagically, call
     * writeConfig to save them
     * @param parent parent widget
     */
    virtual void configDialog (QWidget *parent) { Q_UNUSED(parent); }
};

KTEXTEDITOR_EXPORT Plugin *createPlugin ( const char *libname, QObject *parent );

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
