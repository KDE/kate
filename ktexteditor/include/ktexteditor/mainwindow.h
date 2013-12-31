/* 
 *  This file is part of the KDE project.
 * 
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KTEXTEDITOR_MAINWINDOW_H
#define KTEXTEDITOR_MAINWINDOW_H

#include <ktexteditor/ktexteditor_export.h>

#include <QObject>

class QEvent;
class QIcon;
class QUrl;
class QWidget;

class KXMLGUIFactory;

namespace KTextEditor
{
  
class ApplicationPlugin;
class Document;
class View;
  
/**
 * This class allows the application that embedds the KTextEditor component to
 * allow it to access parts of its main window.
 * 
 * For example the component can get a place to show view bar widgets (e.g. search&replace, goto line, ...).
 * This is useful to e.g. have one place inside the window to show such stuff even if the application allows
 * the user to have multiple split views avaible per window.
 * 
 * The application must pass a pointer to the MainWindow object to the createView method on view creation
 * and ensure that this main window stays valid for the complete lifetime of the view.
 * 
 * It must not reimplement this class but construct an instance and pass a pointer to a QObject that
 * has the required slots to receive the requests.
 * 
 * The interface functions are nullptr safe, this means, you can call them all even if the instance
 * is a nullptr.
 */
class KTEXTEDITOR_EXPORT MainWindow : public QObject
{
  Q_OBJECT
  
  public:
    /**
     * Construct an MainWindow wrapper object.
     * The passed parent is both the parent of this QObject and the receiver of all interface
     * calls via invokeMethod.
     * @param parent object the calls are relayed to
     */
    MainWindow (QObject *parent);
    
    /**
     * Virtual Destructor
     */
    virtual ~MainWindow ();
    
  //
  // Accessors to some window properties and contents
  //
  public:
      /**
       * Get the toplevel widget.
       * \return the real main window widget.
       */
      QWidget *window ();
      
      /**
       * Accessor to the XMLGUIFactory.
       * \return the mainwindow's KXMLGUIFactory.
       */
      KXMLGUIFactory *guiFactory ();

  //
  // Signals related to the main window
  //
  Q_SIGNALS:
      /**
       * This signal is emitted for every unhandled ShortcutOverride in the window
       * @param e responsible event
       */
      void unhandledShortcutOverride (QEvent *e);

  //
  // View access and manipulation interface
  //
  public:
      /**
       * Get a list of all views for this main window.
       * @return all views
       */
      QList<KTextEditor::View *> views ();
      
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
      KTextEditor::View *activateView (KTextEditor::Document *document);

      /**
       * Open the document \p url with the given \p encoding.
       * \param url the document's url
       * \param encoding the preferred encoding. If encoding is QString() the
       *        encoding will be guessed or the default encoding will be used.
       * \return a pointer to the created view for the new document, if a document
       *         with this url is already existing, its view will be activated
       */
      KTextEditor::View *openUrl (const QUrl &url, const QString &encoding = QString());

  //
  // Signals related to view handling
  //
  Q_SIGNALS:
      /**
       * This signal is emitted whenever the active view changes.
       * @param view new active view
       */
      void viewChanged (KTextEditor::View* view);

      /**
       * This signal is emitted whenever a new view is created
       * @param view view that was created
       */
      void viewCreated (KTextEditor::View * view);

  //
  // Interface to allow view bars to be constructed in a central place per window
  //
  public:
    /**
     * Try to create a view bar for the given view.
     * @param view view for which we want an view bar
     * @return suitable widget that can host view bars widgets or nullptr
     */
    QWidget *createViewBar (KTextEditor::View *view);

    /**
     * Delete the view bar for the given view.
     * @param view view for which we want an view bar
     */
    void deleteViewBar (KTextEditor::View *view);

    /**
     * Add a widget to the view bar.
     * @param view view for which the view bar is used
     * @param bar bar widget, shall have the viewBarParent() as parent widget
     */
    void addWidgetToViewBar (KTextEditor::View *view, QWidget *bar);
    
    /**
     * Show the view bar for the given view
     * @param view view for which the view bar is used
     */
    void showViewBar (KTextEditor::View *view);
    
    /**
     * Hide the view bar for the given view
     * @param view view for which the view bar is used
     */
    void hideViewBar (KTextEditor::View *view);

  //
  // ToolView stuff, here all stuff belong which allows to
  // add/remove and manipulate the toolview of this main windows
  //
  public:
    /**
     * Toolview position.
     * A toolview can only be at one side at a time.
     */
    enum ToolViewPosition {
      Left = 0,   /**< Left side. */
      Right = 1,  /**< Right side. */
      Top = 2,    /**< Top side. */
      Bottom = 3  /**< Bottom side. */
    };

    /**
     * Create a new toolview with unique \p identifier at side \p pos
     * with \p icon and caption \p text. Use the returned widget to embedd
     * your widgets.
     * \param plugin which owns this tool view
     * \param identifier unique identifier for this toolview
     * \param pos position for the toolview, if we are in session restore,
     *        this is only a preference
     * \param icon icon to use in the sidebar for the toolview
     * \param text translated text (i18n()) to use in addition to icon
     * \return created toolview on success, otherwise NULL
     */
    QWidget *createToolView (KTextEditor::ApplicationPlugin *plugin, const QString &identifier, KTextEditor::MainWindow::ToolViewPosition pos, const QIcon &icon, const QString &text);

    /**
     * Move the toolview \p widget to position \p pos.
     * \param widget the toolview to move, where the widget was constructed
     *        by createToolView().
     * \param pos new position to move widget to
     * \return \e true on success, otherwise \e false
     */
    bool moveToolView (QWidget *widget, KTextEditor::MainWindow::ToolViewPosition pos);

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
    
  //
  // Application plugin accessors
  //
  public:
    /**
     * Get a plugin view for the plugin with with identifier \p name.
     * \param name the plugin's name
     * \return pointer to the plugin view if a plugin with \p name is loaded and has a view for this mainwindow,
     *         otherwise NULL
     */
    QObject *pluginView (const QString &name);
    
  //
  // Signals related to application plugins
  //
  Q_SIGNALS:
    /**
     * This signal is emitted when the view of some ApplicationPlugin is created for this main window.
     *
     * @param name name of plugin
     * @param pluginView the new plugin view
     */
    void pluginViewCreated (const QString &name, QObject *pluginView);

    /**
     * This signal is emitted when the view of some ApplicationPlugin got deleted.
     *
     * @param name name of plugin
     * @param pluginView the deleted plugin view
     *
     * Warning !!! DO NOT ACCESS THE DATA REFERENCED BY THE POINTER, IT IS ALREADY INVALID
     * Use the pointer only to remove mappings in hash or maps
     */
    void pluginViewDeleted (const QString &name, QObject *pluginView);
    
  private:
    /**
     * Private d-pointer class is our best friend ;)
     */
    friend class MainWindowPrivate;
    
    /**
     * Private d-pointer
     */
    class MainWindowPrivate * const d;
};

} // namespace KTextEditor

#endif
