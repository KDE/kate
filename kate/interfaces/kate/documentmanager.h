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

#ifndef _KATE_DOCMANAGER_INCLUDE_
#define _KATE_DOCMANAGER_INCLUDE_

#include <kdebase_export.h>

#include <QObject>
#include <kurl.h>

namespace KTextEditor
{
  class Document;
}

namespace Kate
{
  /**
   * \brief Interface for the document manager.
   *
   * This interface provides access to Kate's document manager. The document
   * manager manages all documents. Use document() get a given document,
   * activeDocument() to retrieve the active document. Check with isOpen()
   * whether an URL is opened and use findDocument() to get it. To get the
   * number of managed documents use documents().
   *
   * Open new documents with openUrl() and close a document with closeDocument()
   * or closeAllDocuments(). Several signals are provided, documentChanged() is
   * emitted whenever the document's content changed, documentCreated() when a
   * new document was created and documentDeleted() when a document was closed.
   *
   * To access the document manager use the global accessor function
   * documentManager() or Application::documentManager(). You should never have
   * to create an instance of this class yourself.
   *
   * \author Christoph Cullmann \<cullmann@kde.org\>
   */
  class KATEINTERFACES_EXPORT DocumentManager : public QObject
  {
      friend class PrivateDocumentManager;

      Q_OBJECT

    public:
      /**
       * Construtor.
       *
       * The constructor is internally used by the Kate application, so it is
       * of no interest for plugin developers. Plugin developers should use the
       * global accessor pluginManager() instead.
       *
       * \param documentManager internal usage
       *
       * \internal
       */
      DocumentManager ( void *documentManager  );
      /**
       * Virtual destructor.
       */
      virtual ~DocumentManager ();

    public:
      /**
       * Get a list of all documents.
       * @return all documents
       */
      const QList<KTextEditor::Document*> &documents () const;

      /**
       * Get the document with the URL \p url.
       * \param url the document's URL
       * \return the document with the given \p url or NULL, if no such document
       *         is in the document manager's internal list.
       */
      KTextEditor::Document *findUrl (const KUrl &url) const;

      /**
       * Open the document \p url with the given \p encoding.
       * \param url the document's url
       * \param encoding the preferred encoding. If encoding is QString() the
       *        encoding will be guessed or the default encoding will be used.
       * \return a pointer to the created document
       */
      KTextEditor::Document *openUrl (const KUrl &url, const QString &encoding = QString());

      /**
       * Close the given \p document.
       * \param document the document to be closed
       * \return \e true on success, otherwise \e false
       */
      bool closeDocument (KTextEditor::Document *document);

      //
      // SIGNALS !!!
      //
#ifndef Q_MOC_RUN
#undef signals
#define signals public
#endif
    signals:
#ifndef Q_MOC_RUN
#undef signals
#define signals protected
#endif

      /**
       * This signal is emitted when the \p document was created.
       */
      void documentCreated (KTextEditor::Document *document);

      /**
       * This signal is emitted before a \p document which should be closed is deleted
       * The document is still accessible and usable, but it will be deleted
       * after this signal was send.
       */
      void documentWillBeDeleted (KTextEditor::Document *document);

      /**
       * This signal is emitted when the \p document was deleted.
       */
      void documentDeleted (KTextEditor::Document *document);

    private:
      class PrivateDocumentManager *d;
  };

  /**
   * Global accessor to the document manager object.
   * \return document manager object
   */
  KATEINTERFACES_EXPORT DocumentManager *documentManager ();

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

