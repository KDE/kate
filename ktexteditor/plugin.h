/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
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
 * KTextEditor Plugin interface.
 *
 * <b>Introduction</b>\n
 *
 * The Plugin class provides methods to create loadable plugins for all
 * KTextEditor implementations. A plugin can handle several documents and
 * views. For every document the plugin should handle addDocument() is called
 * and for every view addView().
 *
 * <b>Configuration Management</b>\n
 *
 * If your plugin supports a config dialog overwrite configDialogSupported()
 * and return @e true (The default implementation returns @e false and
 * indicates that the plugin does not support a config dialog). The config
 * dialog can be shown by calling configDialog(), it can also be called from
 * a KTextEditor implementation (e.g. the KTextEditor implementation might
 * have a listview that shows all available plugins along with a
 * @e Configure... button). To save or load plugin settings writeConfig() and
 * readConfig() is called by the Editor part. Usually you do not have to call
 * readConfig() and writeConfig() yourself.
 *
 * <b>Session Management</b>\n
 *
 * As an extension a Plugin can implement the SessionConfigInterface. This
 * interface provides functions to read and write session related settings.
 * If you have session dependant data additionally derive your Plugin from
 * this interface and implement the session related functions, for example:
 * @code
 *   class MyPlugin : public KTextEditor::Plugin,
 *                    public KTextEditor::SessionConfigInterface
 *   {
 *     // ...
 *     virtual void readSessionConfig (KConfig *config);
 *     virtual void writeSessionConfig (KConfig *config);
 *   };
 * @endcode
 *
 * <b>Plugin Architecture</b>\n
 *
 * After the plugin is loaded the editor implementation should first call
 * readConfig() with a given KConfig object. After this it will call
 * addDocument() and addView() for all documents and views the plugin should
 * handle. If your plugin has a GUI it is common to add an extra class, like:
 * @code
 *   class PluginView : public QObject, public KXMLGUIClient
 *   {
 *       Q_OBJECT
 *   public:
 *       // Constructor and other methods
 *       PluginView( KTextEditor::View* view )
 *         : QObject( view ), KXMLGUIClient( view )
 *       { ... }
 *       // ...
 *   private:
 *       KTextEditor::View* m_view;
 *   };
 * @endcode
 * Your KTextEditor::Plugin derived class then will create a new PluginView
 * for every View, i.e. for every call of addView().
 *
 * The method removeView() will be called whenever a View is removed/closed.
 * If you have a PluginView for every view this is the place to remove it.
 *
 * @see KTextEditor::Editor, KTextEditor::Document, KTextEditor::View,
 *      KTextEditor::SessionConfigInterface
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Plugin : public QObject
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new plugin.
     * @param parent parent object
     */
    Plugin ( QObject *parent ) : QObject (parent) {}

    /**
     * Virtual destructor.
     */
    virtual ~Plugin () {}

  /*
   * Following methodes allow the plugin to react on view and document
   * creation.
   */
  public:
    /**
     * Add a new @p document to the plugin.
     * This method is called whenever the plugin should handle another
     * @p document.
     *
     * For every call of addDocument() will finally follow a call of
     * removeDocument(), i.e. the number of calls are identic.
     *
     * @param document new document to handle
     * @see removeDocument(), addView()
     */
    virtual void addDocument (Document *document) { Q_UNUSED(document); }

    /**
     * Remove the @p document from the plugin.
     * This method is called whenever the plugin should stop handling
     * the @p document.
     *
     * For every call of addDocument() will finally follow a call of
     * removeDocument(), i.e. the number of calls are identic.
     *
     * @param document document to hang the gui out from
     * @see addDocument(), removeView()
     */
    virtual void removeDocument (Document *document) { Q_UNUSED(document); }

    /**
     * This method is called whenever the plugin should add its GUI to
     * @p view.
     * The process for the Editor can be roughly described as follows:
     *   - add documents the plugin should handle via addDocument()
     *   - for every document @e doc call addView() for @e every view for
     *     @e doc.
     *
     * For every call of addView() will finally follow a call of
     * removeView(), i.e. the number of calls are identic.
     *
     * @note As addView() is called for @e every view in which the plugin's
     *       GUI should be visible you must @e not add the the GUI by
     *       iterating over all Document::views() yourself neither use the
     *       signal Document::viewCreated().
     *
     * @param view view to hang the gui in
     * @see removeView(), addDocument()
     */
    virtual void addView (View *view) { Q_UNUSED(view); }

    /**
     * This method is called whenever the plugin should remove its GUI from
     * @p view.
     *
     * For every call of addView() will finally follow a call of
     * removeView(), i.e. the number of calls are identic.
     *
     * @param view view to hang the gui out from
     * @see addView(), removeDocument()
     */
    virtual void removeView (View *view) { Q_UNUSED(view); }

  /*
   * Configuration management.
   * Default implementation just for convenience, does nothing
   * and says this plugin supports no config dialog.
   */
  public:
    /**
     * Read the editor configuration from the KConfig @p config.
     * If @p config is NULL you can kapp->config() as fallback solution.
     * readConfig() is called from the Editor implementation, so you
     * never have to call it yourself.
     *
     * @param config config object
     * @see writeConfig()
     */
    virtual void readConfig (KConfig *config = 0) { Q_UNUSED(config); }

    /**
     * Write the editor configuration to the KConfig @p config.
     * If @p config is NULL you can kapp->config() as fallback solution.
     * writeConfig() is called from the Editor implementation, so you
     * never have to call it yourself.
     *
     * @param config config object
     * @see readConfig()
     */
    virtual void writeConfig (KConfig *config = 0) { Q_UNUSED(config); }

    /**
     * Check, whether the plugin has support for a config dialog.
     * @return @e true, if the plugin has a config dialog, otherwise @e false
     * @see configDialog()
     */
    virtual bool configDialogSupported () const { return false; }

    /**
     * Show the config dialog for the plugin.
     * Changes should be applied to the plugin, but not saved anywhere
     * immediately. writeConfig() is called by the Editor implementation to
     * save the plugin's settings, e.g. when a plugin is unloaded.
     *
     * @param parent parent widget
     * @see configDialogSupported()
     */
    virtual void configDialog (QWidget *parent) { Q_UNUSED(parent); }
};

/**
 * Create a plugin called @p libname with parent object @p parent.
 * @return the plugin or NULL if it could not be loaded
 */
KTEXTEDITOR_EXPORT Plugin *createPlugin ( const char *libname, QObject *parent );

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
