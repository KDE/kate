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

#include <QtCore/QObject>

#include <ktexteditor/ktexteditor_export.h>
#include <kservice.h>

class KConfig;

namespace KTextEditor
{

class Document;
class View;

/**
 * \brief KTextEditor Plugin interface.
 *
 * Topics:
 *  - \ref plugin_intro
 *  - \ref plugin_config
 *  - \ref plugin_sessions
 *  - \ref plugin_arch
 *
 * \section plugin_intro Introduction
 *
 * The Plugin class provides methods to create loadable plugins for all
 * KTextEditor implementations. A plugin can handle several documents and
 * views. For every document the plugin should handle addDocument() is called
 * and for every view addView().
 *
 * \section plugin_config Configuration Management
 *
 * @todo write docu about config pages (new with kpluginmanager)
 * @todo write docu about save/load settings (related to config page)
 *
 * \section plugin_sessions Session Management
 *
 * As an extension a Plugin can implement the SessionConfigInterface. This
 * interface provides functions to read and write session related settings.
 * If you have session dependent data additionally derive your Plugin from
 * this interface and implement the session related functions, for example:
 * \code
 * class MyPlugin : public KTextEditor::Plugin,
 *                  public KTextEditor::SessionConfigInterface
 * {
 *   Q_OBJECT
 *   Q_INTERFACES(KTextEditor::SessionConfigInterface)
 *
 *   // ...
 *   virtual void readSessionConfig (const KConfigGroup& config);
 *   virtual void writeSessionConfig (KConfigGroup& config);
 * };
 * \endcode
 *
 * \section plugin_arch Plugin Architecture
 *
 * After the plugin is loaded the editor implementation should call
 * addDocument() and addView() for all documents and views the plugin should
 * handle. If your plugin has a GUI it is common to add an extra class, like:
 * \code
 * class PluginView : public QObject, public KXMLGUIClient
 * {
 *     Q_OBJECT
 * public:
 *     // Constructor and other methods
 *     PluginView( KTextEditor::View* view )
 *       : QObject( view ), KXMLGUIClient( view ), m_view( view )
 *     { ... }
 *     // ...
 * private:
 *     KTextEditor::View* m_view;
 * };
 * \endcode
 * Your KTextEditor::Plugin derived class then will create a new PluginView
 * for every View, i.e. for every call of addView().
 *
 * The method removeView() will be called whenever a View is removed/closed.
 * If you have a PluginView for every view this is the place to remove it.
 *
 * \see KTextEditor::Editor, KTextEditor::Document, KTextEditor::View,
 *      KTextEditor::SessionConfigInterface
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Plugin : public QObject
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new plugin.
     * \param parent parent object
     */
    Plugin ( QObject *parent );

    /**
     * Virtual destructor.
     */
    virtual ~Plugin ();

  /*
   * Following methodes allow the plugin to react on view and document
   * creation.
   */
  public:
    /**
     * Add a new \p document to the plugin.
     * This method is called whenever the plugin should handle another
     * \p document.
     *
     * For every call of addDocument() will finally follow a call of
     * removeDocument(), i.e. the number of calls are identic.
     *
     * \param document new document to handle
     * \see removeDocument(), addView()
     */
    virtual void addDocument (Document *document) { Q_UNUSED(document); }

    /**
     * Remove the \p document from the plugin.
     * This method is called whenever the plugin should stop handling
     * the \p document.
     *
     * For every call of addDocument() will finally follow a call of
     * removeDocument(), i.e. the number of calls are identic.
     *
     * \param document document to hang the gui out from
     * \see addDocument(), removeView()
     */
    virtual void removeDocument (Document *document) { Q_UNUSED(document); }

    /**
     * This method is called whenever the plugin should add its GUI to
     * \p view.
     * The process for the Editor can be roughly described as follows:
     *   - add documents the plugin should handle via addDocument()
     *   - for every document \e doc call addView() for \e every view for
     *     \e doc.
     *
     * For every call of addView() will finally follow a call of
     * removeView(), i.e. the number of calls are identic.
     *
     * \note As addView() is called for \e every view in which the plugin's
     *       GUI should be visible you must \e not add the GUI by
     *       iterating over all Document::views() yourself neither use the
     *       signal Document::viewCreated().
     *
     * \param view view to hang the gui in
     * \see removeView(), addDocument()
     */
    virtual void addView (View *view) { Q_UNUSED(view); }

    /**
     * This method is called whenever the plugin should remove its GUI from
     * \p view.
     *
     * For every call of addView() will finally follow a call of
     * removeView(), i.e. the number of calls are identic.
     *
     * \param view view to hang the gui out from
     * \see addView(), removeDocument()
     */
    virtual void removeView (View *view) { Q_UNUSED(view); }

  private:
    class PluginPrivate* const d;
};

/**
 * Create a plugin represented by \p service with parent object \p parent.
 * To get the KService object you usually use KServiceTypeTrader. Example
 * \code
 * KService::List list = KServiceTypeTrader::self()->query("KTextEditor/Plugin");
 *
 * foreach(const KService::Ptr &service, list) {
 *   // do something with service
 * }
 * \endcode
 * \return the plugin or NULL if it could not be loaded
 */
KTEXTEDITOR_EXPORT Plugin *createPlugin ( KService::Ptr service, QObject *parent );

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
