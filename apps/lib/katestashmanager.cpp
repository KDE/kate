/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katestashmanager.h"

#include "katedebug.h"

#include "kateapp.h"
#include "kateviewmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringDecoder>
#include <QUrl>

void KateStashManager::clearStashForSession(const KateSession::Ptr session)
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

void KateStashManager::stashDocuments(KConfig *config, std::span<KTextEditor::Document *const> documents) const
{
    if (!canStash()) {
        return;
    }

    // prepare stash directory
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    dir.mkdir(QStringLiteral("stash"));
    dir.cd(QStringLiteral("stash"));

    const QString stashName = QFileInfo(KateApp::self()->sessionManager()->activeSession()->file()).fileName();
    dir.mkdir(stashName);
    dir.cd(stashName);

    int i = 0;
    for (KTextEditor::Document *doc : documents) {
        // stash the file content
        if (doc->isModified()) {
            const QString entryName = QStringLiteral("Document %1").arg(i);
            KConfigGroup cg(config, entryName);
            stashDocument(doc, entryName, cg, dir.path());
        }

        i++;
    }
}

bool KateStashManager::willStashDoc(KTextEditor::Document *doc) const
{
    Q_ASSERT(canStash());

    if (doc->isEmpty()) {
        return false;
    }
    if (doc->url().isEmpty()) {
        return m_stashNewUnsavedFiles;
    }
    if (doc->isModified() && doc->url().isLocalFile()) {
        const QString path = doc->url().toLocalFile();
        if (path.startsWith(QDir::tempPath())) {
            return false; // inside tmp resource, do not stash
        }
        return m_stashUnsavedChanges;
    }
    return false;
}

void KateStashManager::stashDocument(KTextEditor::Document *doc, const QString &stashfileName, KConfigGroup &kconfig, const QString &path) const
{
    if (!willStashDoc(doc)) {
        return;
    }

    // Stash changes
    QString stashedFile = path + QStringLiteral("/") + stashfileName;

    // save the current document changes to stash
    if (!doc->saveAs(QUrl::fromLocalFile(stashedFile))) {
        qCWarning(LOG_KATE) << "Could not write to stash file" << stashedFile;
        return;
    }

    // write stash metadata to config
    kconfig.writeEntry("stashedFile", stashedFile);
    if (doc->url().isValid()) {
        // save checksum for already-saved documents
        kconfig.writeEntry("checksum", doc->checksum());
    }

    kconfig.sync();
}

bool KateStashManager::canStash() const
{
    const auto activeSession = KateApp::self()->sessionManager()->activeSession();
    return activeSession && !activeSession->isAnonymous() && !activeSession->file().isEmpty();
}

void KateStashManager::popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig)
{
    if (!(kconfig.hasKey("stashedFile"))) {
        return;
    }
    qCDebug(LOG_KATE) << "popping stashed document" << doc->url();

    // read metadata
    const auto stashedFile = kconfig.readEntry("stashedFile");
    const auto url = QUrl(kconfig.readEntry("URL"));

    bool checksumOk = true;
    if (url.isValid()) {
        const auto sum = kconfig.readEntry(QStringLiteral("checksum")).toLatin1().constData();
        checksumOk = sum != doc->checksum();
    }

    if (checksumOk) {
        // open file with stashed content
        QFile input(stashedFile);
        input.open(QIODevice::ReadOnly);

        auto decoder = QStringDecoder(kconfig.readEntry("Encoding").toUtf8().constData());
        QString text = decoder.isValid() ? decoder.decode(input.readAll()) : QString::fromLocal8Bit(input.readAll());

        // normalize line endings, to e.g. catch issues with \r\n on Windows
        text.replace(QRegularExpression(QStringLiteral("\r\n?")), QStringLiteral("\n"));

        doc->setText(text);

        // clean stashed file
        if (!input.remove()) {
            qCWarning(LOG_KATE) << "Could not remove stash file" << stashedFile;
        }
    }
}
