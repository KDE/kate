/* This file is part of the KDE project
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_APPLICATION_INCLUDE_
#define _KATE_APPLICATION_INCLUDE_

#include <kate_export.h>

#include <QObject>
#include <kurl.h>


namespace KTextEditor
{
  class Editor;
}

/**
 * \brief Namespace for Kate application interfaces.
 *
 * This namespace contains interfaces for plugin developers for the Kate
 * application.
 */
namespace Kate
{

  class DocumentManager;
  class PluginManager;
    class MainWindow;

  /**
   * \brief Central interface to the application.
   *
   * The class Application is central as it provides access to the core
   * application, which includes the
   * - document manager, access it with documentManager()
   * - plugin manager, access it with pluginManager()
   * - main windows, access a main window with mainWindow() or activeMainWindow()
   * - used editor component, access it with editor().
   *
   * To access the application use the global accessor function application().
   * You should never have to create an instance of this class yourself.
   *
   * \author Christoph Cullmann \<cullmann@kde.org\>
   */
  class KATEINTERFACES_EXPORT Application : public QObject
  {
      friend class PrivateApplication;

      Q_OBJECT

    public:
      /**
       * Construtor.
       *
       * The constructor is internally used by the Kate application, so it is
       * of no interest for plugin developers. Plugin developers should use the
       * global accessor application() instead.
       *
       * \internal
       */
      Application (void *application);

      /**
       * Virtual desctructor.
       */
      virtual ~Application ();

    public:
      /**
       * Accessor to the document manager.
       * There is always an document manager, so it is not necessary
       * to test the returned value for NULL.
       * \return a pointer to the document manager
       */
      Kate::DocumentManager *documentManager ();

      /**
       * Accessor to the plugin manager.
       * There is always an plugin manager, so it is not necessary
       * to test the returned value for NULL.
       * \return a pointer to the plugin manager
       */
      Kate::PluginManager *pluginManager ();

      /**
       * Accessor to the global editor part.
       * There is always an editor part, so it is not necessary
       * to test the returned value for NULL.
       * \return KTextEditor component
       */
      KTextEditor::Editor *editor();

      /**
       * Accessor to the active mainwindow.
       * There is always an active mainwindow, so it is not necessary
       * to test the returned value for NULL.
       * \return a pointer to the active mainwindow
       */
      Kate::MainWindow *activeMainWindow ();

      /**
       * Get a list of all mainwindows. At least one entry, always.
       * @return all mainwindows
       * @see activeMainWindow()
       */
      const QList<Kate::MainWindow*> &mainWindows () const;

    private:
      class PrivateApplication *d;
  };

  /**
   * Global accessor to the application object. Always existing during
   * lifetime of a plugin.
   * \return application object
   */
  KATEINTERFACES_EXPORT Application *application ();

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

