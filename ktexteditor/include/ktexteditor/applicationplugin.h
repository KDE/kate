/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#ifndef KDELIBS_KTEXTEDITOR_APPLICATION_PLUGIN_H
#define KDELIBS_KTEXTEDITOR_APPLICATION_PLUGIN_H

#include <QObject>

#include <ktexteditor/ktexteditor_export.h>

namespace KTextEditor
{
  
class MainWindow;

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
class KTEXTEDITOR_EXPORT ApplicationPlugin : public QObject
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new application plugin.
     * \param parent parent object
     */
    ApplicationPlugin ( QObject *parent );

    /**
     * Virtual destructor.
     */
    virtual ~ApplicationPlugin ();

    /**
     * Create a new View for this plugin for the given KTextEditor::MainWindow
     * This may be called arbitrary often by the application to create as much
     * views as mainwindows are around, the application will take care to delete
     * this views if mainwindows close, you don't need to handle this yourself in
     * the plugin.
     * \param mainWindow the MainWindow for which a view should be created
     * \return the new created view or NULL
     */
    virtual QObject *createView (KTextEditor::MainWindow *mainWindow) = 0;

  private:
    class ApplicationPluginPrivate* const d;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
