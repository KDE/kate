/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katestashmanager.h"
#include "kateapp.h"
#include "katedebug.h"

#include <KTextEditor/Editor>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

void KateStashManager::clearStashForSession(const KateSession::Ptr &session)
{
    // we should avoid to kill stuff for these, they can't be stashed
    if (session->isAnonymous() || session->file().isEmpty()) {
        return;
    }

    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (dir.exists(QStringLiteral("stash"))) {
        dir.cd(QStringLiteral("stash"));
        const QString stashName = QFileInfo(session->file()).fileName();
        if (dir.exists(stashName)) {
            dir.cd(stashName);
            dir.removeRecursively();
        }
    }
}

void KateStashManager::stashDocuments(KConfig *config, std::span<KTextEditor::Document *const> documents)
{
    if (!canStash()) {
        return;
    }
    qCDebug(LOG_KATE, "%s", __func__);

    // prepare stash directory, create it recursively if needed
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString stashName = QFileInfo(KateApp::self()->sessionManager()->activeSession()->file()).fileName();
    const QString path = QStringLiteral("%1/stash/%2").arg(appDataPath, stashName);
    QDir().mkpath(path);

    // forget documents that are gone
    std::erase_if(m_stashedDocuments, [](const auto &stashed) {
        return !stashed.second.doc;
    });

    for (int i = 0; i < (int)documents.size(); ++i) {
        auto doc = documents[i];
        if (willStashDoc(doc)) {
            // stash the file content
            const QString stashfileName = QStringLiteral("Document %1").arg(i);
            // Stash changes
            QString stashedFile = path + QStringLiteral("/") + stashfileName;

            // only write the content if it changed since it was stashed to this file
            const auto it = m_stashedDocuments.find(doc);
            const bool changed = it == m_stashedDocuments.end() || it->second.doc != doc || it->second.file != stashedFile
                || it->second.revision != doc->revision() || !QFile::exists(stashedFile);
            if (changed) {
                // create a temp doc to stash it. We dont want to change the url of the original doc
                std::unique_ptr<KTextEditor::Document> tmpDoc(KTextEditor::Editor::instance()->createDocument(nullptr));
                tmpDoc->setText(doc->text());

                // the file belongs to this document now
                std::erase_if(m_stashedDocuments, [&stashedFile](const auto &stashed) {
                    return stashed.second.file == stashedFile;
                });

                // save the current document changes to stash
                if (!tmpDoc->saveAs(QUrl::fromLocalFile(stashedFile))) {
                    qCWarning(LOG_KATE, "Could not write to stash file %ls", qUtf16Printable(stashedFile));
                    m_stashedDocuments.erase(doc);
                    continue;
                }
                m_stashedDocuments[doc] = {.doc = doc, .file = stashedFile, .revision = doc->revision()};
            }

            KConfigGroup cg(config, stashfileName);
            // write stash metadata to config
            cg.writeEntry("stashedFile", stashedFile);
            if (doc->url().isValid()) {
                // save checksum for already-saved documents
                cg.writeEntry("checksum", doc->checksum().toHex());
            }
        }
    }
}

bool KateStashManager::willStashDoc(KTextEditor::Document *doc) const
{
    Q_ASSERT(canStash());

    if (doc->isEmpty()) {
        return false;
    }
    if (doc->url().isEmpty()) {
        return stashNewUnsavedFiles;
    }
    if (doc->isModified() && doc->url().isLocalFile()) {
        const QString path = doc->url().toLocalFile();
        if (path.startsWith(QDir::tempPath())) {
            return false; // inside tmp resource, do not stash
        }
        return stashUnsavedChanges;
    }
    return false;
}

bool KateStashManager::canStash() const
{
    const KateSession::Ptr activeSession = KateApp::self()->sessionManager()->activeSession();
    return activeSession && !activeSession->isAnonymous() && !activeSession->file().isEmpty();
}

void KateStashManager::popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig)
{
    if (!(kconfig.hasKey("stashedFile"))) {
        return;
    }
    qCDebug(LOG_KATE, "popping stashed document %ls", qUtf16Printable(doc->url().toString()));

    // read metadata
    const auto stashedFile = kconfig.readEntry("stashedFile");
    const auto url = QUrl(kconfig.readEntry("URL"));

    bool checksumOk = true;
    if (url.isValid()) {
        // hex encoded, older versions stored the raw bytes of the SHA1
        const QByteArray storedSum = kconfig.readEntry("checksum", QByteArray());
        const QByteArray sum = storedSum.size() == 40 ? QByteArray::fromHex(storedSum) : storedSum;
        checksumOk = sum == doc->checksum();
    }

    if (checksumOk) {
        // open file with stashed content
        // use dummy document to have proper encoding and line ending handling, see bug 489360
        std::unique_ptr<KTextEditor::Document> tmpDoc(KTextEditor::Editor::instance()->createDocument(nullptr));
        tmpDoc->setEncoding(doc->encoding());
        tmpDoc->openUrl(QUrl::fromLocalFile(stashedFile));
        doc->setText(tmpDoc->text());

        // clean stashed file
        if (!QFile::remove(stashedFile)) {
            qCWarning(LOG_KATE, "Could not remove stash file %ls", qUtf16Printable(stashedFile));
        }
    }
}
