/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#ifndef __KATE_DOCMANAGER_H__
#define __KATE_DOCMANAGER_H__

#include "katemain.h"
#include <kate/documentmanager.h>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/ModificationInterface>

#include <QPointer>
#include <QList>
#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QPair>
#include <QDateTime>
#include <QStandardItemModel>

namespace KParts
{
  class Factory;
}

class KConfig;
class KateMainWindow;

class KateDocumentInfo
{
  public:
    KateDocumentInfo ()
        : modifiedOnDisc (false),
        modifiedOnDiscReason (KTextEditor::ModificationInterface::OnDiskUnmodified)
    {}

    bool modifiedOnDisc;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason modifiedOnDiscReason;
};

class KateDocManager : public QStandardItemModel
{
    Q_OBJECT

  public:
    KateDocManager (QObject *parent);
    ~KateDocManager ();

    /**
     * should the document manager suppress all opening error dialogs on openUrl?
     + @param suppress suppress dialogs?
     */
    void setSuppressOpeningErrorDialogs (bool suppress);

    enum CustomRoles {DocumentRole = Qt::UserRole + 1, OpeningOrderRole, CustomOrderRole };

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    static KateDocManager *self ();

    Kate::DocumentManager *documentManager ()
    {
      return m_documentManager;
    }
    KTextEditor::Editor *editor()
    {
      return m_editor;
    }

    KTextEditor::Document *createDoc ();
    void deleteDoc (KTextEditor::Document *doc);

    KTextEditor::Document *document (uint n);

    const KateDocumentInfo *documentInfo (KTextEditor::Document *doc);

    int findDocument (KTextEditor::Document *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    KTextEditor::Document *findDocument (const KUrl &url) const;

    bool isOpen(KUrl url);

    QString dbusObjectPath()
    {
      return m_dbusObjectPath;
    }
    uint documents ();

    const QList<KTextEditor::Document*> &documentList () const
    {
      return m_docList;
    }

    KTextEditor::Document *openUrl(const KUrl&, const QString &encoding = QString(), bool isTempFile = false);

    bool closeDocument(class KTextEditor::Document *, bool closeUrl = true);
    bool closeDocument(uint);
    bool closeAllDocuments(bool closeUrl = true);
    bool closeOtherDocuments(KTextEditor::Document*);
    bool closeOtherDocuments(uint);

    QList<KTextEditor::Document*> modifiedDocumentList(KTextEditor::Document* excludedDoc = NULL);
    bool queryCloseDocuments(KateMainWindow *w);

    void saveDocumentList (class KConfig *config);
    void restoreDocumentList (class KConfig *config);

    inline bool getSaveMetaInfos()
    {
      return m_saveMetaInfos;
    }
    inline void setSaveMetaInfos(bool b)
    {
      m_saveMetaInfos = b;
    }

    inline int getDaysMetaInfos()
    {
      return m_daysMetaInfos;
    }
    inline void setDaysMetaInfos(int i)
    {
      m_daysMetaInfos = i;
    }

    QModelIndex indexForDocument(KTextEditor::Document *document);

  public Q_SLOTS:
    /**
     * saves all documents that has at least one view.
     * documents with no views are ignored :P
     */
    void saveAll();

	/**
	 * reloads all documents that has at least one view.
	 * documents with no views are ignored :P
	 */
	void reloadAll();

  Q_SIGNALS:

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

    void initialDocumentReplaced ();

  private Q_SLOTS:
    void slotModifiedOnDisc (KTextEditor::Document *doc, bool b, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);
    void slotModChanged(KTextEditor::Document *doc);
    void slotModChanged1(KTextEditor::Document *doc);
    void slotDocumentNameChanged(KTextEditor::Document *document);
    void slotDocumentUrlChanged(KTextEditor::Document *doc);

    void showRestoreErrors ();
  private:
    bool loadMetaInfos(KTextEditor::Document *doc, const KUrl &url);
    void saveMetaInfos(KTextEditor::Document *doc);
    bool computeUrlMD5(const KUrl &url, QByteArray &result);

    Kate::DocumentManager *m_documentManager;
    QList<KTextEditor::Document*> m_docList;
    QHash<KTextEditor::Document*, KateDocumentInfo*> m_docInfos;

    KConfig *m_metaInfos;
    bool m_saveMetaInfos;
    int m_daysMetaInfos;


    //KParts::Factory *m_factory;
    KTextEditor::Editor *m_editor;

    typedef QPair<KUrl, QDateTime> TPair;
    QMap<KTextEditor::Document *, TPair> m_tempFiles;
    QString m_dbusObjectPath;
    QMap<KTextEditor::Document*, QStandardItem*> m_documentItemMapping;
    bool m_restoringDocumentList;
    QString m_openingErrors;
    int m_documentStillToRestore;

    // suppress error dialogs while opening?
    bool m_suppressOpeningErrorDialogs;

  private Q_SLOTS:
    void documentOpened();
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
