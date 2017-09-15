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

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/modificationinterface.h>

#include <QList>
#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QPair>
#include <QDateTime>

#include <KConfig>

class KateMainWindow;

class KateDocumentInfo
{
public:
    enum CustomRoles {RestoreOpeningFailedRole };

public:
    KateDocumentInfo()
        : modifiedOnDisc(false)
        , modifiedOnDiscReason(KTextEditor::ModificationInterface::OnDiskUnmodified)
        , openedByUser(false)
        , openSuccess(true)
    {}

    bool modifiedOnDisc;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason modifiedOnDiscReason;

    bool openedByUser;
    bool openSuccess;
};

class KateDocManager : public QObject
{
    Q_OBJECT

public:
    KateDocManager(QObject *parent);
    ~KateDocManager() override;

    KTextEditor::Document *createDoc(const KateDocumentInfo &docInfo = KateDocumentInfo());

    KateDocumentInfo *documentInfo(KTextEditor::Document *doc);

    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    KTextEditor::Document *findDocument(const QUrl &url) const;

    const QList<KTextEditor::Document *> &documentList() const {
        return m_docList;
    }

    KTextEditor::Document *openUrl(const QUrl &,
                                   const QString &encoding = QString(),
                                   bool isTempFile = false,
                                   const KateDocumentInfo &docInfo = KateDocumentInfo());

    QList<KTextEditor::Document *> openUrls(const QList<QUrl> &,
                                            const QString &encoding = QString(),
                                            bool isTempFile = false,
                                            const KateDocumentInfo &docInfo = KateDocumentInfo());

    bool closeDocument(KTextEditor::Document *, bool closeUrl = true);
    bool closeDocuments(const QList<KTextEditor::Document *> documents, bool closeUrl = true);
    bool closeDocumentList(QList<KTextEditor::Document *> documents);
    bool closeAllDocuments(bool closeUrl = true);
    bool closeOtherDocuments(KTextEditor::Document *);

    QList<KTextEditor::Document *> modifiedDocumentList();
    bool queryCloseDocuments(KateMainWindow *w);

    void saveDocumentList(KConfig *config);
    void restoreDocumentList(KConfig *config);

    inline bool getSaveMetaInfos() {
        return m_saveMetaInfos;
    }
    inline void setSaveMetaInfos(bool b) {
        m_saveMetaInfos = b;
    }

    inline int getDaysMetaInfos() {
        return m_daysMetaInfos;
    }
    inline void setDaysMetaInfos(int i) {
        m_daysMetaInfos = i;
    }

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

    /**
     * close all documents, which could not be reopened
     */
    void closeOrphaned();

    /**
     * save selected documents from the File List
     */
    void saveSelected(const QList<KTextEditor::Document *> &);

Q_SIGNALS:
    /**
     * This signal is emitted when the \p document was created.
     */
    void documentCreated(KTextEditor::Document *document);

    /**
     * This signal is emitted when the \p document was created.
     * This is emitted after the initial documentCreated for internal use in view manager
     */
    void documentCreatedViewManager(KTextEditor::Document *document);

    /**
     * This signal is emitted before a \p document which should be closed is deleted
     * The document is still accessible and usable, but it will be deleted
     * after this signal was send.
     *
     * @param document document that will be deleted
     */
    void documentWillBeDeleted(KTextEditor::Document *document);

    /**
     * This signal is emitted when the \p document has been deleted.
     *
     *  Warning !!! DO NOT ACCESS THE DATA REFERENCED BY THE POINTER, IT IS ALREADY INVALID
     *  Use the pointer only to remove mappings in hash or maps
     */
    void documentDeleted(KTextEditor::Document *document);

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
     * note that the batch can be interrupted in the middle and only some
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

private Q_SLOTS:
    void slotModifiedOnDisc(KTextEditor::Document *doc, bool b, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);
    void slotModChanged(KTextEditor::Document *doc);
    void slotModChanged1(KTextEditor::Document *doc);

    void showRestoreErrors();
private:
    bool loadMetaInfos(KTextEditor::Document *doc, const QUrl &url);
    void saveMetaInfos(const QList<KTextEditor::Document *> &docs);

    QList<KTextEditor::Document *> m_docList;
    QHash<KTextEditor::Document *, KateDocumentInfo *> m_docInfos;

    KConfig m_metaInfos;
    bool m_saveMetaInfos;
    int m_daysMetaInfos;

    typedef QPair<QUrl, QDateTime> TPair;
    QMap<KTextEditor::Document *, TPair> m_tempFiles;
    QString m_openingErrors;
    int m_documentStillToRestore;

private Q_SLOTS:
    void documentOpened();
};

#endif
