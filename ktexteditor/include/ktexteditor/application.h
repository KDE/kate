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

#ifndef KTEXTEDITOR_APPLICATION_H
#define KTEXTEDITOR_APPLICATION_H

#include <ktexteditor/ktexteditor_export.h>

#include <QObject>

namespace KTextEditor
{

class ApplicationPlugin;
class Document;
class MainWindow;
  
/**
 * This class allows the application that embedds the KTextEditor component to
 * allow it access to application wide information and interactions.
 * 
 * For example the component can get the current active main window of the application.
 * 
 * The application must pass a pointer to the Application object to the setApplication method of the
 * global editor instance and ensure that this object stays valid for the complete lifetime of the editor.
 * 
 * It must not reimplement this class but construct an instance and pass a pointer to a QObject that
 * has the required slots to receive the requests.
 * 
 * The interface functions are nullptr safe, this means, you can call them all even if the instance
 * is a nullptr.
 */
class KTEXTEDITOR_EXPORT Application : public QObject
{
  Q_OBJECT
  
  public:
    /**
     * Construct an Application wrapper object.
     * The passed parent is both the parent of this QObject and the receiver of all interface
     * calls via invokeMethod.
     * @param parent object the calls are relayed to
     */
    Application (QObject *parent);
    
    /**
     * Virtual Destructor
     */
    virtual ~Application ();
    
  //
  // MainWindow related accessors
  //
  public:
      /**
       * Get a list of all main windows.
       * @return all main windows
       */
      QList<KTextEditor::MainWindow *> mainWindows ();
      
      /**
       * Accessor to the active main window.
       * \return a pointer to the active mainwindow
       */
      KTextEditor::MainWindow *activeMainWindow ();
      
  //
  // Document related accessors
  //
  public:
      /**
       * Get a list of all documents that are managed by the application.
       * This might contain less documents than the editor has in his documents () list.
       * @return all documents the application manages
       */
      QList<KTextEditor::Document *> documents ();

      /**
       * Get the document with the URL \p url.
       * if multiple documents match the searched url, return the first found one...
       * \param url the document's URL
       * \return the document with the given \p url or NULL, if none found
       */
      KTextEditor::Document *findUrl (const QUrl &url);

      /**
       * Open the document \p url with the given \p encoding.
       * if the url is empty, a new empty document will be created
       * \param url the document's url
       * \param encoding the preferred encoding. If encoding is QString() the
       *        encoding will be guessed or the default encoding will be used.
       * \return a pointer to the created document
       */
      KTextEditor::Document *openUrl (const QUrl &url, const QString &encoding = QString());

      /**
       * Close the given \p document. If the document is modified, user will be asked if he wants that.
       * \param document the document to be closed
       * \return \e true on success, otherwise \e false
       */
      bool closeDocument (KTextEditor::Document *document);

      /**
       * Close a list of documents. If any of them are modified, user will be asked if he wants that.
       * Use this, if you want to close multiple documents at once, as the application might
       * be able to group the "do you really want that" dialogs into one.
       * \param documents list of documents to be closed
       * \return \e true on success, otherwise \e false
       */
      bool closeDocuments (const QList<KTextEditor::Document *> &documents);

    Q_SIGNALS:
      /**
       * This signal is emitted when the \p document was created.
       * 
       * @param document document that was created
       */
      void documentCreated (KTextEditor::Document *document);

      /**
       * This signal is emitted before a \p document which should be closed is deleted
       * The document is still accessible and usable, but it will be deleted
       * after this signal was send.
       * 
       * @param document document that will be deleted
       */
      void documentWillBeDeleted (KTextEditor::Document *document);

      /**
       * This signal is emitted when the \p document has been deleted.
       *
       * Warning !!! DO NOT ACCESS THE DATA REFERENCED BY THE POINTER, IT IS ALREADY INVALID
       * Use the pointer only to remove mappings in hash or maps
       * 
       * @param document document that is deleted
       */
      void documentDeleted (KTextEditor::Document *document);

      /**
       * This signal is emitted before the batch of documents is being created.
       *
       * You can use it to pause some updates.
       */
      void aboutToCreateDocuments();

      /**
       * This signal is emitted after the batch of ducuments is created.
       *
       * @param documents list of documents that have been created
       */
      void documentsCreated(const QList<KTextEditor::Document *> &documents);

      /**
       * This signal is emitted before the documents batch is going to be deleted
       *
       * note that the batch can be interupted in the middle and only some
       * of the documents may be actually deleted. See documentsDeleted() signal.
       */
      void aboutToDeleteDocuments(const QList<KTextEditor::Document *> &);

      /**
       * This singnal is emitted after the documents batch was deleted
       *
       * This is the batch closing signal for aboutToDeleteDocuments
       * @param documents the documents that weren't deleted after all
       */
      void documentsDeleted(const QList<KTextEditor::Document *> &documents);

  //
  // Application plugin accessors
  //
  public:
    /**
     * Get a plugin for the plugin with with identifier \p name.
     * \param name the plugin's name
     * \return pointer to the plugin if a plugin with \p name is loaded, otherwise nullptr
     */
    KTextEditor::ApplicationPlugin *plugin (const QString &name);
    
  //
  // Signals related to application plugins
  //
  Q_SIGNALS:
    /**
     * This signal is emitted when an ApplicationPlugin was loaded.
     *
     * @param name name of plugin
     * @param plugin the new plugin
     */
    void pluginCreated (const QString &name, KTextEditor::ApplicationPlugin *plugin);

    /**
     * This signal is emitted when an ApplicationPlugin got deleted.
     *
     * @param name name of plugin
     * @param plugin the deleted plugin
     *
     * Warning !!! DO NOT ACCESS THE DATA REFERENCED BY THE POINTER, IT IS ALREADY INVALID
     * Use the pointer only to remove mappings in hash or maps
     */
    void pluginDeleted (const QString &name, KTextEditor::ApplicationPlugin *plugin);

  private:
    /**
     * Private d-pointer class is our best friend ;)
     */
    friend class ApplicationPrivate;
    
    /**
     * Private d-pointer
     */
    class ApplicationPrivate * const d;
};

} // namespace KTextEditor

#endif
