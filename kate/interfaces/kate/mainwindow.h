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

#ifndef _KATE_MAINWINDOW_INCLUDE_
#define _KATE_MAINWINDOW_INCLUDE_

#include <kate_export.h>

#include <QtCore/QObject>
#include <QtGui/QPixmap>

#include <kxmlguifactory.h>
#include <kurl.h>

class QWidget;

namespace KTextEditor
{
  class View;
  class Document;
}

namespace Kate
{

  /**
   * \brief Interface to a mainwindow.
   *
   * \section mainwindow_intro Introduction
   * The class MainWindow represents a toplevel window, with menu bar,
   * statusbar etc, get it with window(). A mainwindow usually has an active
   * View, access it with activeView(). To set another active view use
   * activateView().
   *
   * \section mainwindow_toolviews Toolviews
   *
   * It is possible to embedd new toolviews into a mainwindow. To create a
   * toolview use createToolView(), then you can move, hide or show the toolview
   * by using moveToolView(), hideToolView() or showToolView().
   *
   * With guiFactory() you can access the KXMLGUIFactory framework and add
   * gui clients.
   *
   * To access a mainwindow use the Application object.
   * You should never have to create an instance of this class yourself.
   *
   * \author Christoph Cullmann \<cullmann@kde.org\>
   * \see KXMLGUIFactory
   */
  class KATEINTERFACES_EXPORT MainWindow : public QObject
  {
      friend class PrivateMainWindow;

      Q_OBJECT

    public:
      /**
       * Constructor.
       *
       * Create a new mainwindow. You as a plugin developer should never have
       * to create a new mainwindow yourself. Access the mainwindows via the
       * global Application.
       * \param mainWindow internal usage
       * \internal
       */
      MainWindow (void *mainWindow);
      /**
       * Virtual destructor.
       */
      virtual ~MainWindow ();

    public: /*these are slots for kjs*/
      /**
       * Accesstor to the XMLGUIFactory.
       * \return the mainwindow's KXMLGUIFactory.
       */
      KXMLGUIFactory *guiFactory() const;

      /**
       * Get the toplevel widget.
       * \return the kate main window.
       */
      class QWidget *window() const;

      /**
       * Access the widget (in the middle of the four sidebars) in which the
       * editor component and the KateTabBar are embedded. This widget is a
       * KVBox, so other child widgets can be embedded themselves under the
       * editor widget.
       *
       * \return the central widget
       */
      class QWidget *centralWidget() const;

      /*
       * View stuff, here all stuff belong which allows to
       * access and manipulate the KTextEditor::View's we have in this windows
       */
    public:
      /**
       * Access the active view.
       * \return active view
       */
      KTextEditor::View *activeView ();

      /**
       * Activate the view with the corresponding \p document.
       * If none exist for this document, create one
       * \param document the document
       * \return activated view of this document
       */
      KTextEditor::View *activateView ( KTextEditor::Document *document );

      /**
       * Open the document \p url with the given \p encoding.
       * \param url the document's url
       * \param encoding the preferred encoding. If encoding is QString() the
       *        encoding will be guessed or the default encoding will be used.
       * \return a pointer to the created view for the new document, if a document
       *         with this url is already existing, it's view will be activated
       */
      KTextEditor::View *openUrl (const KUrl &url, const QString &encoding = QString());

      //
      // SIGNALS !!!
      //
#ifndef Q_MOC_RUN
#undef signals
#define signals public
#endif
    Q_SIGNALS:
#ifndef Q_MOC_RUN
#undef signals
#define signals protected
#endif

      /**
       * This signal is emitted whenever the active view changes.
       */
      void viewChanged ();

      /**
       * This signal is emitted whenever a new view is created
       * @since 4.2
       */

      void viewCreated(KTextEditor::View * view);

      /*
       * ToolView stuff, here all stuff belong which allows to
       * add/remove and manipulate the toolview of this main windows
       */
    public:
      /**
       * Toolview position.
       * A toolview can only be at one side at a time.
       */
      enum Position {
        Left = 0,   /**< Left side. */
        Right = 1,  /**< Right side. */
        Top = 2,    /**< Top side. */
        Bottom = 3  /**< Bottom side. */
    };

      /**
       * Create a new toolview with unique \p identifier at side \p pos
       * with \p icon and caption \p text. Use the returned widget to embedd
       * your widgets.
       * \param identifier unique identifier for this toolview
       * \param pos position for the toolview, if we are in session restore,
       *        this is only a preference
       * \param icon icon to use in the sidebar for the toolview
       * \param text translated text (i18n()) to use in addition to icon
       * \return created toolview on success, otherwise NULL
       */
      QWidget *createToolView (const QString &identifier, MainWindow::Position pos, const QPixmap &icon, const QString &text);

      /**
       * Move the toolview \p widget to position \p pos.
       * \param widget the toolview to move, where the widget was constructed
       *        by createToolView().
       * \param pos new position to move widget to
       * \return \e true on success, otherwise \e false
       */
      bool moveToolView (QWidget *widget, MainWindow::Position pos);

      /**
       * Show the toolview \p widget.
       * \param widget the toolview to show, where the widget was constructed
       *        by createToolView().
       * \return \e true on success, otherwise \e false
       * \todo add focus parameter: bool showToolView (QWidget *widget, bool giveFocus );
       */
      bool showToolView (QWidget *widget);

      /**
       * Hide the toolview \p widget.
       * \param widget the toolview to hide, where the widget was constructed
       *        by createToolView().
       * \return \e true on success, otherwise \e false
       */
      bool hideToolView (QWidget *widget);

    private:
      class PrivateMainWindow *d;
  };

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

