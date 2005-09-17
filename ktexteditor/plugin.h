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

#ifndef KDELIBS_KTEXTEDITOR_PLUGIN_H
#define KDELIBS_KTEXTEDITOR_PLUGIN_H

#include <qobject.h>

#include <kdelibs_export.h>

class KConfig;

namespace KTextEditor
{

class Document;
class View;

/**
 * The class @p Plugin is the basic KTextEditor plugin class.
 * This plugin will be bound to a single document, i.e. for every document
 * a single instance of a plugin exists.
 */
class KTEXTEDITOR_EXPORT Plugin : public QObject
{
  Q_OBJECT

  public:
    /**
     * Plugin constructor.
     * @param parent parent object
     */
    Plugin ( QObject *parent ) : QObject (parent) {}

    /**
     * virtual destructor
     */
    virtual ~Plugin () {}

  /**
   * Following methodes allow the plugin to react on view and document
   * creation.
   */
  public:
    /**
     * This method is called if the plugin gui should be added
     * to the @e document.
     * @param document document to hang the gui in
     */
    virtual void addDocument (Document *document) { Q_UNUSED(document); }

    /**
     * This method is called if the plugin gui should be removed
     * from the @e document.
     * @param document document to hang the gui out from
     */
    virtual void removeDocument (Document *document) { Q_UNUSED(document); }

    /**
     * This method is called if the plugin gui should be added
     * to the @e view.
     * @param view view to hang the gui in
     */
    virtual void addView (View *view) { Q_UNUSED(view); }

    /**
     * This method is called if the plugin gui should be removed
     * from the @e view.
     * @param view view to hang the gui out from
     */
    virtual void removeView (View *view) { Q_UNUSED(view); }

  /**
   * Configuration management.
   * Default implementation just for convenience, does nothing
   * and says this plugin supports no config dialog.
   */
  public:
    /**
     * Read the editor configuration from its standard config.
     */
    virtual void readConfig () {}

    /**
     * Write the editor configuration to its standard config.
     */
    virtual void writeConfig () {}

    /**
     * Read the editor configuration from the KConfig @e config.
     * @param config config object
     */
    virtual void readConfig (KConfig *config) { Q_UNUSED(config); }

    /**
     * Write the editor configuration to the KConfig @e config.
     * @param config config object
     */
    virtual void writeConfig (KConfig *config) { Q_UNUSED(config); }

    /**
     * Check, whether the plugin has support for a config dialog.
     * @return @e true, if the plugin has a config dialog, otherwise @e false
     */
    virtual bool configDialogSupported () const { return false; }

    /**
     * Show the config dialog for the part, changes will be applied to the
     * editor, but not saved anywhere automagically, call @p writeConfig()
     * to save them.
     * @param parent parent widget
     */
    virtual void configDialog (QWidget *parent) { Q_UNUSED(parent); }
};

KTEXTEDITOR_EXPORT Plugin *createPlugin ( const char *libname, QObject *parent );

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
