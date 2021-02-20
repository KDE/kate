/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katestashmanager.h"

#include "katedebug.h"

#include "kateapp.h"
#include "katedocmanager.h"
#include "kateviewmanager.h"

#include "kconfiggroup.h"
#include "ksharedconfig.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QUrl>

KateStashManager::KateStashManager(QObject *parent)
    : QObject(parent)
{
}

bool KateStashManager::willStashDoc(KTextEditor::Document *doc)
{
    if (m_stashUnsaveChanges == 0) {
        return false;
    }
    if (!KateApp::self()->sessionManager()->activeSession()) {
        return false;
    }
    if (doc->text().isEmpty()) {
        return false;
    }
    if (doc->url().isEmpty()) {
        return true;
    }
    if (doc->url().isLocalFile()) {
        const QString path = doc->url().toLocalFile();
        if (path.startsWith(QDir::tempPath())) {
            return false; // inside tmp resource, do not stash
        }
    }
    return m_stashUnsaveChanges == 2;
}

void KateStashManager::stashDocument(KTextEditor::Document *doc, const QString &stashfileName, KConfigGroup &kconfig, const QString &path)
{
    if (!willStashDoc(doc)) {
        return;
    }
    // Stash changes
    QString stashedFile = path + QStringLiteral("/") + stashfileName;

    // save the current document changes to stash
    if (!doc->saveAs(QUrl::fromLocalFile(stashedFile))) {
        qCWarning(LOG_KATE()) << "Could not write to stash file" << stashedFile;
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

bool KateStashManager::popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig)
{
    if (!(kconfig.hasKey("stashedFile"))) {
        return false;
    }
    qCDebug(LOG_KATE) << "popping stashed document" << doc->url();

    // read metadata
    auto stashedFile = kconfig.readEntry("stashedFile");
    auto url = QUrl(kconfig.readEntry("URL"));

    bool checksumOk = true;
    if (url.isValid()) {
        auto sum = kconfig.readEntry(QStringLiteral("checksum")).toLatin1().constData();
        checksumOk = sum != doc->checksum();
    }

    if (checksumOk) {
        // open file with stashed content
        QFile file(stashedFile);
        file.open(QIODevice::ReadOnly);
        QTextStream out(&file);
        doc->setText(out.readAll());

        // clean stashed file
        file.remove();

        return true;
    } else {
        return false;
    }
}
