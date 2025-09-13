/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "kateprivate_export.h"

#include <KTextEditor/Document>

#include <QList>
#include <QObject>

#include <KConfig>

#include <span>

class KateMainWindow;

class KateDocumentInfo
{
public:
    enum CustomRoles {
        RestoreOpeningFailedRole
    };

public:
    KateDocumentInfo() = default;
    QUrl normalizedUrl;

    KTextEditor::Document::ModifiedOnDiskReason modifiedOnDiscReason = KTextEditor::Document::OnDiskUnmodified;
    KTextEditor::Cursor startCursor = KTextEditor::Cursor::invalid(); // initial cursor position. This can be specified in the command line or as a url query
    // id of this document from the last session restore or as set by the last session save
    // -1 if not valid
    int sessionConfigId = -1;

    bool openedByUser = false;
    bool openSuccess = true;
    bool wasDocumentEverModified = false;
    bool modifiedOnDisc = false;
};

class KATE_PRIVATE_EXPORT KateDocManager : public QObject
{
    Q_OBJECT

    friend class KateDocManagerTests;

public:
    explicit KateDocManager(QObject *parent);
    ~KateDocManager() override;

    KTextEditor::Document *createDoc(const KateDocumentInfo &docInfo = KateDocumentInfo());

    KateDocumentInfo *documentInfo(KTextEditor::Document *doc);

    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    KTextEditor::Document *findDocument(const QUrl &url) const;

    /**
     * Find the document matching the sessionConfigId you pass.
     */
    KTextEditor::Document *findDocumentForSessionConfigId(int sessionConfigId) const;

    const QList<KTextEditor::Document *> &documentList() const
    {
        return m_docList;
    }

    KTextEditor::Document *openUrl(const QUrl &, const QString &encoding = QString(), const KateDocumentInfo &docInfo = KateDocumentInfo());

    std::vector<KTextEditor::Document *>
    openUrls(std::span<const QUrl>, const QString &encoding = QString(), const KateDocumentInfo &docInfo = KateDocumentInfo());

    QList<QUrl> popRecentlyClosedUrls();

    bool closeDocument(KTextEditor::Document *, bool closeUrl = true);
    bool closeDocuments(std::span<KTextEditor::Document *const> documents, bool closeUrl = true);
    bool closeDocumentList(std::span<KTextEditor::Document *const> documents, KateMainWindow *window);
    bool closeAllDocuments(bool closeUrl = true);
    bool closeOtherDocuments(KTextEditor::Document *);

    std::vector<KTextEditor::Document *> modifiedDocumentList();
    bool queryCloseDocuments(KateMainWindow *w);

    void saveDocumentList(KConfig *config);
    void restoreDocumentList(KConfig *config);

    inline bool getSaveMetaInfos() const
    {
        return m_saveMetaInfos;
    }
    inline void setSaveMetaInfos(bool b)
    {
        m_saveMetaInfos = b;
    }

    inline int getDaysMetaInfos() const
    {
        return m_daysMetaInfos;
    }
    inline void setDaysMetaInfos(int i)
    {
        m_daysMetaInfos = i;
    }

    void togglePinDocument(KTextEditor::Document *document);
    bool isDocumentPinned(KTextEditor::Document *document) const;
    const QList<QUrl> &pinnedDocumentUrls() const
    {
        return m_pinnedDocuments;
    }

public:
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

Q_SIGNALS:
    /**
     * This signal is emitted when the \p document was created.
     */
    void documentCreated(KTextEditor::Document *document);

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
     * This signal is emitted before the documents batch is going to be deleted
     *
     * note that the batch can be interrupted in the middle and only some
     * of the documents may be actually deleted. See documentsDeleted() signal.
     */
    void aboutToDeleteDocuments();

    /**
     * This signal is emitted after the documents batch was deleted
     *
     * This is the batch closing signal for aboutToDeleteDocuments
     * @param documents the documents that weren't deleted after all
     */
    void documentsDeleted();

private:
    void slotModifiedOnDisc(KTextEditor::Document *doc, bool b, KTextEditor::Document::ModifiedOnDiskReason reason);
    void slotModChanged(KTextEditor::Document *doc);
    void slotModChanged1(KTextEditor::Document *doc);
    void slotUrlChanged(const QUrl &newUrl);
    void documentOpened();

private:
    bool loadMetaInfos(KTextEditor::Document *doc, const QUrl &url);
    void saveMetaInfos(std::span<KTextEditor::Document *const> docs);

    QList<KTextEditor::Document *> m_docList;
    std::vector<KateDocumentInfo> m_docInfos;

    KConfig m_metaInfos;
    bool m_saveMetaInfos;
    int m_daysMetaInfos;

    QList<QUrl> m_recentlyClosedUrls;
    QList<QUrl> m_pinnedDocuments;
};
