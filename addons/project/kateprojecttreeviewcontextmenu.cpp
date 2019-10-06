/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Dominik Haumann <dhaumann.org>
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

#include "kateprojecttreeviewcontextmenu.h"

#include <KIO/OpenFileManagerWindowJob>
#include <KMimeTypeTrader>
#include <KNS3/KMoreTools>
#include <KNS3/KMoreToolsMenuFactory>
#include <KPropertiesDialog>
#include <KRun>
#include <klocalizedstring.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QStandardPaths>

KateProjectTreeViewContextMenu::KateProjectTreeViewContextMenu()
{
}

KateProjectTreeViewContextMenu::~KateProjectTreeViewContextMenu()
{
}

static bool isGit(const QString &filename)
{
    QFileInfo fi(filename);
    QDir dir(fi.absoluteDir());
    QProcess git;
    git.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    // git ls-files -z results a bytearray where each entry is \0-terminated.
    // NOTE: Without -z, Umlauts such as "Der Bäcker/Das Brötchen.txt" do not work (#389415, #402213)
    args << QStringLiteral("ls-files") << QStringLiteral("-z") << fi.fileName();
    git.start(QStringLiteral("git"), args);
    bool isGit = false;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        const QList<QByteArray> byteArrayList = git.readAllStandardOutput().split('\0');
        const QString fn = fi.fileName();
        for (const QByteArray &byteArray : byteArrayList) {
            if (fn == QString::fromUtf8(byteArray)) {
                isGit = true;
                break;
            }
        }
    }
    return isGit;
}

void KateProjectTreeViewContextMenu::exec(const QString &filename, const QPoint &pos, QWidget *parent)
{
    /**
     * Create context menu
     */
    QMenu menu;

    /**
     * Copy Path
     */
    QAction *copyAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy File Path"));

    /**
     * Handle "open with",
     * find correct mimetype to query for possible applications
     */
    QMenu *openWithMenu = menu.addMenu(i18n("Open With"));
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filename);
    const KService::List offers = KMimeTypeTrader::self()->query(mimeType.name(), QStringLiteral("Application"));
    // For each one, insert a menu item...
    for (const auto &service : offers) {
        if (service->name() == QLatin1String("Kate")) {
            continue; // omit Kate
        }
        QAction *action = openWithMenu->addAction(QIcon::fromTheme(service->icon()), service->name());
        action->setData(service->entryPath());
    }
    // Perhaps disable menu, if no entries
    openWithMenu->setEnabled(!openWithMenu->isEmpty());

    /**
     * Open Containing folder
     */
    auto openContaingFolderAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("&Open Containing Folder"));

    /**
     * File Properties Dialog
     */
    auto filePropertiesAction = menu.addAction(QIcon::fromTheme(QStringLiteral("dialog-object-properties")), i18n("Properties"));

    /**
     * Git menu
     */
    KMoreToolsMenuFactory menuFactory(QStringLiteral("kate/addons/project/git-tools"));
    QMenu gitMenu; // must live as long as the maybe filled menu items should live
    if (isGit(filename)) {
        menuFactory.fillMenuFromGroupingNames(&gitMenu, {QLatin1String("git-clients-and-actions")}, QUrl::fromLocalFile(filename));
        menu.addSection(i18n("Git:"));
        const auto gitActions = gitMenu.actions();
        for (auto action : gitActions) {
            menu.addAction(action);
        }
    }

    /**
     * run menu and handle the triggered action
     */
    if (QAction *const action = menu.exec(pos)) {
        if (action == copyAction) {
            QApplication::clipboard()->setText(filename);
        } else if (action->parentWidget() == openWithMenu) {
            // handle "open with"
            const QString openWith = action->data().toString();
            if (KService::Ptr app = KService::serviceByDesktopPath(openWith)) {
                QList<QUrl> list;
                list << QUrl::fromLocalFile(filename);
                KRun::runService(*app, list, parent);
            }
        } else if (action == openContaingFolderAction) {
            KIO::highlightInFileManager({QUrl::fromLocalFile(filename)});
        } else if (action == filePropertiesAction) {
            // code copied and adapted from frameworks/kio/src/filewidgets/knewfilemenu.cpp
            KFileItem fileItem(QUrl::fromLocalFile(filename));
            QDialog *dlg = new KPropertiesDialog(fileItem);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->show();
        } else {
            // One of the git actions was triggered
        }
    }
}
