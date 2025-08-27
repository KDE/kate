/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2007 Flavio Castelli <flavio.castelli@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katedocmanager.h"

#include "kateapp.h"
#include "katedebug.h"
#include "katemainwindow.h"
#include "katesavemodifieddialog.h"
#include "kateviewmanager.h"
#include "ktexteditor_utils.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

#include <QFileDialog>
#include <QProgressDialog>

KateDocManager::KateDocManager(QObject *parent)
    : QObject(parent)
    , m_metaInfos(KateApp::isKate() ? QStringLiteral("katemetainfos") : QStringLiteral("kwritemetainfos"), KConfig::NoGlobals)
    , m_saveMetaInfos(true)
    , m_daysMetaInfos(0)
{
    // set our application wrapper
    KTextEditor::Editor::instance()->setApplication(KateApp::self()->wrapper());
}

KateDocManager::~KateDocManager()
{
    // write metainfos?
    if (m_saveMetaInfos) {
        // saving meta-infos when file is saved is not enough, we need to do it once more at the end
        saveMetaInfos(m_docList);

        // purge saved filesessions
        if (m_daysMetaInfos > 0) {
            const QStringList groups = m_metaInfos.groupList();
            QDateTime def(QDate(1970, 1, 1).startOfDay());

            for (const auto &group : groups) {
                QDateTime last = m_metaInfos.group(group).readEntry("Time", def);
                if (last.daysTo(QDateTime::currentDateTimeUtc()) > m_daysMetaInfos) {
                    m_metaInfos.deleteGroup(group);
                }
            }
        }
    }
}

void KateDocManager::slotUrlChanged(const QUrl &newUrl)
{
    auto *doc = qobject_cast<KTextEditor::Document *>(sender());
    if (doc) {
        m_docInfos.at(doc).normalizedUrl = Utils::normalizeUrl(newUrl);
    }
}

KTextEditor::Document *KateDocManager::createDoc(const KateDocumentInfo &docInfo)
{
    KTextEditor::Document *doc = KTextEditor::Editor::instance()->createDocument(this);

    // turn off the editorpart's own modification dialog, we have our own one, too!
    const KConfigGroup generalGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
    bool ownModNotification = generalGroup.readEntry("Modified Notification", false);
    doc->setModifiedOnDiskWarning(!ownModNotification);

    m_docList.push_back(doc);
    m_docInfos.emplace(doc, docInfo);

    // connect internal signals...
    connect(doc, &KTextEditor::Document::modifiedChanged, this, &KateDocManager::slotModChanged1);
    connect(doc, &KTextEditor::Document::modifiedOnDisk, this, &KateDocManager::slotModifiedOnDisc);
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, [this](KTextEditor::Document *doc) {
        slotUrlChanged(doc->url());
    });
    connect(doc, &KParts::ReadOnlyPart::urlChanged, this, &KateDocManager::slotUrlChanged);

    // we have a new document, show it the world
    Q_EMIT documentCreated(doc);

    // return our new document
    return doc;
}

KateDocumentInfo *KateDocManager::documentInfo(KTextEditor::Document *doc)
{
    auto it = m_docInfos.find(doc);
    if (it != m_docInfos.end()) {
        return &it->second;
    }
    return nullptr;
}

KTextEditor::Document *KateDocManager::findDocument(const QUrl &url) const
{
    auto it = std::find_if(m_docInfos.begin(), m_docInfos.end(), [u = Utils::normalizeUrl(url)](const auto &p) {
        return p.second.normalizedUrl == u;
    });

    return it == m_docInfos.end() ? nullptr : it->first;
}

std::vector<KTextEditor::Document *> KateDocManager::openUrls(std::span<const QUrl> urls, const QString &encoding, const KateDocumentInfo &docInfo)
{
    std::vector<KTextEditor::Document *> docs;
    docs.reserve(urls.size());
    for (const QUrl &url : urls) {
        docs.push_back(openUrl(url, encoding, docInfo));
    }
    return docs;
}

KTextEditor::Document *KateDocManager::openUrl(const QUrl &url, const QString &encoding, const KateDocumentInfo &docInfo)
{
    // We want to work on absolute urls
    const QUrl u(Utils::absoluteUrl(url));

    // try to find already open document
    if (!u.isEmpty()) {
        if (auto doc = findDocument(u)) {
            return doc;
        }
    }

    // else: create new document
    auto doc = createDoc(docInfo);
    if (!encoding.isEmpty()) {
        doc->setEncoding(encoding);
    }
    if (!u.isEmpty()) {
        doc->openUrl(u);
        loadMetaInfos(doc, u);
    }
    return doc;
}

QList<QUrl> KateDocManager::popRecentlyClosedUrls()
{
    const auto recentlyClosedUrls = m_recentlyClosedUrls;
    m_recentlyClosedUrls.clear();
    return recentlyClosedUrls;
}

bool KateDocManager::closeDocuments(std::span<KTextEditor::Document *const> documents, bool closeUrl)
{
    // if we have nothing to close, that always succeeds
    if (documents.empty()) {
        return true;
    }

    m_recentlyClosedUrls.clear();
    for (const auto document : documents) {
        if (const auto docInfoItr = m_docInfos.find(document); docInfoItr != m_docInfos.end()) {
            const auto &docInfo = docInfoItr->second;

            if (!docInfo.normalizedUrl.isEmpty()) {
                m_recentlyClosedUrls.push_back(docInfo.normalizedUrl);
            }
        }
    }

    saveMetaInfos(documents);

    Q_EMIT aboutToDeleteDocuments();

    bool success = true;
    for (KTextEditor::Document *doc : documents) {
        if (closeUrl && !doc->closeUrl()) {
            success = false; // get out on first error
            break;
        }

        // document will be deleted, soon
        Q_EMIT documentWillBeDeleted(doc);

        // really delete the document and its infos
        disconnect(doc, &KParts::ReadOnlyPart::urlChanged, this, &KateDocManager::slotUrlChanged);
        m_docInfos.erase(doc);
        delete m_docList.takeAt(m_docList.indexOf(doc));

        // document is gone, emit our signals
        Q_EMIT documentDeleted(doc);
    }

    Q_EMIT documentsDeleted();

    return success;
}

bool KateDocManager::closeDocument(KTextEditor::Document *doc, bool closeUrl)
{
    if (!doc) {
        return false;
    }

    return closeDocuments({&doc, 1}, closeUrl);
}

bool KateDocManager::closeDocumentList(std::span<KTextEditor::Document *const> documents, KateMainWindow *window)
{
    std::vector<KTextEditor::Document *> modifiedDocuments;
    for (KTextEditor::Document *document : documents) {
        if (document->isModified()) {
            modifiedDocuments.push_back(document);
        }
    }

    if (!modifiedDocuments.empty() && !KateSaveModifiedDialog::queryClose(window, modifiedDocuments)) {
        return false;
    }

    return closeDocuments(documents, false); // Do not show save/discard dialog
}

bool KateDocManager::closeAllDocuments(bool closeUrl)
{
    /**
     * just close all documents
     */
    return closeDocuments(m_docList, closeUrl);
}

bool KateDocManager::closeOtherDocuments(KTextEditor::Document *doc)
{
    /**
     * close all documents beside the passed one
     */
    QList<KTextEditor::Document *> documents;
    documents.reserve(m_docList.size() - 1);
    for (auto document : std::as_const(m_docList)) {
        if (document != doc) {
            documents.push_back(document);
        }
    }

    return closeDocuments(documents);
}

/**
 * Find all modified documents.
 * @return Return the list of all modified documents.
 */
std::vector<KTextEditor::Document *> KateDocManager::modifiedDocumentList()
{
    std::vector<KTextEditor::Document *> modified;
    std::copy_if(m_docList.begin(), m_docList.end(), std::back_inserter(modified), [](KTextEditor::Document *doc) {
        return doc->isModified();
    });
    return modified;
}

bool KateDocManager::queryCloseDocuments(KateMainWindow *w)
{
    const auto docCount = m_docList.size();
    for (KTextEditor::Document *doc : std::as_const(m_docList)) {
        if (doc->url().isEmpty() && doc->isModified()) {
            int msgres = KMessageBox::warningTwoActionsCancel(w,
                                                              i18n("<p>The document '%1' has been modified, but not saved.</p>"
                                                                   "<p>Do you want to save your changes or discard them?</p>",
                                                                   doc->documentName()),
                                                              i18n("Close Document"),
                                                              KStandardGuiItem::save(),
                                                              KStandardGuiItem::discard());

            if (msgres == KMessageBox::Cancel) {
                return false;
            }

            if (msgres == KMessageBox::PrimaryAction) {
                const QUrl url = QFileDialog::getSaveFileUrl(w, i18n("Save As"));
                if (!url.isEmpty()) {
                    if (!doc->saveAs(url)) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        } else {
            if (!doc->queryClose()) {
                return false;
            }
        }
    }

    // document count changed while queryClose, abort and notify user
    if (m_docList.size() > docCount) {
        KMessageBox::information(w, i18n("New file opened while trying to close Kate, closing aborted."), i18n("Closing Aborted"));
        return false;
    }

    return true;
}

void KateDocManager::saveAll()
{
    for (KTextEditor::Document *doc : std::as_const(m_docList)) {
        if (doc->isModified()) {
            doc->documentSave();
        }
    }
}

void KateDocManager::reloadAll()
{
    // reload all docs that are NOT modified on disk
    for (KTextEditor::Document *doc : std::as_const(m_docList)) {
        if (!documentInfo(doc)->modifiedOnDisc) {
            doc->documentReload();
        }
    }

    // take care of all documents that ARE modified on disk
    KateApp::self()->activeKateMainWindow()->showModOnDiskPrompt(KateMainWindow::PromptAll);
}

void KateDocManager::closeOrphaned()
{
    QList<KTextEditor::Document *> documents;

    for (KTextEditor::Document *doc : std::as_const(m_docList)) {
        KateDocumentInfo *info = documentInfo(doc);
        if (info && !info->openSuccess) {
            documents.push_back(doc);
        }
    }

    closeDocuments(documents);
}

void KateDocManager::saveDocumentList(KConfig *config)
{
    KConfigGroup openDocGroup(config, QStringLiteral("Open Documents"));

    openDocGroup.writeEntry("Count", (int)m_docList.size());
    qCDebug(LOG_KATE, "KateDocManager::%s: Count: %d", __func__, (int)m_docList.size());

    int i = 0;
    for (KTextEditor::Document *doc : std::as_const(m_docList)) {
        const QString entryName = QStringLiteral("Document %1").arg(i);
        KConfigGroup cg(config, entryName);
        doc->writeSessionConfig(cg);
        m_docInfos[doc].sessionConfigId = i;
        i++;
    }
}

void KateDocManager::restoreDocumentList(KConfig *config)
{
    KConfigGroup openDocGroup(config, QStringLiteral("Open Documents"));
    unsigned int count = openDocGroup.readEntry("Count", 0);
    qCDebug(LOG_KATE, "KateDocManager::%s: Count: %d", __func__, (int)m_docList.size());

    // kill the old stored id mappings
    for (auto &info : m_docInfos) {
        info.second.sessionConfigId = -1;
    }

    if (count == 0) {
        return;
    }

    QProgressDialog progress;
    progress.setWindowTitle(i18n("Starting Up"));
    progress.setLabelText(i18n("Reopening files from the last session..."));
    progress.setModal(true);
    progress.setCancelButton(nullptr);
    progress.setRange(0, count);

    for (unsigned int i = 0; i < count; i++) {
        KConfigGroup cg(config, QStringLiteral("Document %1").arg(i));
        KTextEditor::Document *doc = createDoc();
        m_docInfos[doc].sessionConfigId = i;

        connect(doc, &KTextEditor::Document::completed, this, &KateDocManager::documentOpened);
        connect(doc, &KParts::ReadOnlyPart::canceled, this, &KateDocManager::documentOpened);

        doc->readSessionConfig(cg);

        KateStashManager::popDocument(doc, cg);

        progress.setValue(i);
    }
}

void KateDocManager::slotModifiedOnDisc(KTextEditor::Document *doc, bool b, KTextEditor::Document::ModifiedOnDiskReason reason)
{
    auto it = m_docInfos.find(doc);
    if (it != m_docInfos.end()) {
        it->second.modifiedOnDisc = b;
        it->second.modifiedOnDiscReason = reason;
        slotModChanged1(doc);
    }
}

/**
 * Load file's meta-information if the checksum didn't change since last time.
 */
bool KateDocManager::loadMetaInfos(KTextEditor::Document *doc, const QUrl &url)
{
    if (!m_saveMetaInfos) {
        return false;
    }

    if (!m_metaInfos.hasGroup(url.toDisplayString())) {
        return false;
    }

    const QByteArray checksum = doc->checksum().toHex();
    bool ok = true;
    if (!checksum.isEmpty()) {
        KConfigGroup urlGroup(&m_metaInfos, url.toDisplayString());
        const QString old_checksum = urlGroup.readEntry("Checksum");

        if (QString::fromLatin1(checksum) == old_checksum) {
            QSet<QString> flags;
            if (documentInfo(doc)->openedByUser) {
                flags << QStringLiteral("SkipEncoding");
            }
            flags << QStringLiteral("SkipUrl");
            doc->readSessionConfig(urlGroup, flags);
        } else {
            urlGroup.deleteGroup();
            ok = false;
        }
    }

    return ok && doc->url() == url;
}

/**
 * Save file's meta-information if doc is in 'unmodified' state
 */

void KateDocManager::saveMetaInfos(std::span<KTextEditor::Document *const> documents)
{
    /**
     * skip work if no meta infos wanted
     */
    if (!m_saveMetaInfos) {
        return;
    }

    const QSet<QString> flags{QStringLiteral("SkipUrl")};

    /**
     * store meta info for all non-modified documents which have some checksum
     */
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (KTextEditor::Document *doc : documents) {
        /**
         * skip modified docs
         */
        if (doc->isModified()) {
            continue;
        }

        const QByteArray checksum = doc->checksum().toHex();
        if (!checksum.isEmpty()) {
            /**
             * write the group with checksum and time
             */
            const QString url = doc->url().toString();
            KConfigGroup urlGroup(&m_metaInfos, url);

            /**
             * write document session config
             */
            doc->writeSessionConfig(urlGroup, flags);
            if (!urlGroup.keyList().isEmpty()) {
                urlGroup.writeEntry("URL", url);
                urlGroup.writeEntry("Checksum", QString::fromLatin1(checksum));
                urlGroup.writeEntry("Time", now);
            } else {
                urlGroup.deleteGroup();
            }
        }
    }
}

void KateDocManager::slotModChanged(KTextEditor::Document *doc)
{
    saveMetaInfos({&doc, 1});
}

void KateDocManager::slotModChanged1(KTextEditor::Document *doc)
{
    auto mw = KateApp::self()->activeKateMainWindow();
    QMetaObject::invokeMethod(
        mw,
        [mw, doc] {
            mw->queueModifiedOnDisc(doc);
        },
        Qt::QueuedConnection);

    if (doc->isModified()) {
        KateDocumentInfo *info = documentInfo(doc);
        if (info) {
            info->wasDocumentEverModified = true;
        }
    }
}

void KateDocManager::documentOpened()
{
    auto *doc = qobject_cast<KTextEditor::Document *>(sender());
    if (!doc) {
        return; // should never happen, but who knows
    }
    disconnect(doc, &KTextEditor::Document::completed, this, &KateDocManager::documentOpened);
    disconnect(doc, &KParts::ReadOnlyPart::canceled, this, &KateDocManager::documentOpened);

    // Only set "no success" when doc is empty to avoid close of files
    // with other trouble when do closeOrphaned()
    if (doc->openingError() && doc->isEmpty()) {
        KateDocumentInfo *info = documentInfo(doc);
        if (info) {
            info->openSuccess = false;
        }
    }
}

KTextEditor::Document *KateDocManager::findDocumentForSessionConfigId(int sessionConfigId) const
{
    if (sessionConfigId < 0) {
        return nullptr;
    }

    for (const auto &info : m_docInfos) {
        if (info.second.sessionConfigId == sessionConfigId) {
            return info.first;
        }
    }

    return nullptr;
}

#include "moc_katedocmanager.cpp"
