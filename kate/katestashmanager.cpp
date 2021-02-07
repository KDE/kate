/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katestashmanager.h"

#include "katedebug.h"

#include "katedocmanager.h"
#include "kateviewmanager.h"

#include "kconfiggroup.h"
#include "ksharedconfig.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QUrl>

KateStashManager::KateStashManager()
    : m_stashGroup(KSharedConfig::openConfig()->group("stash"))
{
}

bool KateStashManager::stash(const QList<KTextEditor::Document *> &modifiedDocuments)
{
    qCDebug(LOG_KATE) << "stashing" << modifiedDocuments.length() << "files";
    const QString appDataPath = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();

    int nFile = 0;
    bool success = true;

    QDir dir(appDataPath);
    dir.mkdir(QStringLiteral("stash"));

    for (auto &docu : modifiedDocuments) {
        // Stash changes
        QString stashfileName;
        if (docu->url().isValid()) {
            stashfileName = docu->url().fileName() + QStringLiteral("-") + QString::number(nFile++);
        } else {
            stashfileName = QStringLiteral("Unsaved-") + QString::number(nFile++);
        }
        QString stashedFile = appDataPath + QStringLiteral("/stash/") + stashfileName;

        // save the current document changes to stash
        QSaveFile saveFile(stashedFile);
        saveFile.open(QIODevice::WriteOnly);
        saveFile.write(docu->text().toUtf8());
        if (!saveFile.commit()) {
            qCWarning(LOG_KATE()) << "Could not write to stash file" << stashedFile;
            success = false;
            continue;
        }

        // write stash metadata to config
        auto fileGroup = m_stashGroup.group(stashfileName);
        fileGroup.writeEntry("stashedFile", stashedFile);

        const auto url = docu->url();
        if (url.isValid()) {
            fileGroup.writeEntry("Url", docu->url());
        }
    }

    m_stashGroup.sync();

    return success;
}

void KateStashManager::popStash(KateViewManager *viewManager)
{
    qCDebug(LOG_KATE) << "popping stash";

    for (auto &stashedFileName : m_stashGroup.groupList()) {
        // read metadata
        auto group = m_stashGroup.group(stashedFileName);
        auto stashedFile = group.readEntry(QStringLiteral("stashedFile"));
        auto url = QUrl(group.readEntry(QStringLiteral("Url")));

        // open file with stashed content
        KateDocumentInfo docInfo;
        auto doc = viewManager->openUrl(url, QString(), false, true, docInfo);
        QFile file(stashedFile);
        file.open(QIODevice::ReadOnly);
        QTextStream out(&file);
        doc->setText(out.readAll());

        // clean stashed file
        file.remove();
    }
    // clean metadata
    m_stashGroup.deleteGroup();
    m_stashGroup.sync();
}
