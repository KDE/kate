/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2013 Dominik Haumann <dhaumann.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojecttreeviewcontextmenu.h"
#include "filehistorywidget.h"
#include "git/gitutils.h"
#include "katefileactions.h"
#include "kateproject.h"
#include "kateprojectinfoviewterminal.h"
#include "kateprojectitem.h"
#include "kateprojectviewtree.h"

#include <KAuthorized>
#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KPropertiesDialog>
#include <KTerminalLauncherJob>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>

#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>

static QString getName(QWidget *parent, const QString &title)
{
    QInputDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setLabelText(i18n("Enter name:"));
    dlg.setInputMode(QInputDialog::TextInput);
    dlg.resize(400, dlg.height());

    int res = dlg.exec();
    bool suc = res == QDialog::Accepted;
    if (!suc || dlg.textValue().isEmpty()) {
        return {};
    }
    return dlg.textValue();
}

static void onDeleteFile(const QModelIndex &index, const QString &path, KateProjectViewTree *parent)
{
    if (!index.isValid())
        return;
    const QPersistentModelIndex idx = index;
    const QString title = i18n("Delete File");
    const QString text = i18n("Do you want to delete the file '%1'?", path);
    if (QMessageBox::Yes == QMessageBox::question(parent, title, text, QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes)) {
        if (!idx.isValid())
            return;
        const QList<KTextEditor::Document *> openDocuments = KTextEditor::Editor::instance()->application()->documents();

        // if is open, close
        for (auto doc : openDocuments) {
            if (doc->url().adjusted(QUrl::RemoveScheme) == QUrl(path).adjusted(QUrl::RemoveScheme)) {
                KTextEditor::Editor::instance()->application()->closeDocument(doc);
                break;
            }
        }
        parent->removeFile(idx, path);
    }
}

void KateProjectTreeViewContextMenu::exec(const QString &filename, const QModelIndex &index, const QPoint &pos, KateProjectViewTree *parent)
{
    /**
     * Create context menu
     */
    QMenu menu(parent);

    /**
     * Copy Path, always available, put that to the top
     */
    QAction *copyAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy-path")), i18n("Copy Location"));

    const bool isRootDirectory = !index.isValid();

    QAction *addFile = nullptr;
    QAction *addFolder = nullptr;
    if (isRootDirectory || index.data(KateProjectItem::TypeRole).toInt() == KateProjectItem::Directory) {
        addFile = menu.addAction(QIcon::fromTheme(QStringLiteral("document-new")), i18n("New File…"));
        addFolder = menu.addAction(QIcon::fromTheme(QStringLiteral("folder-new")), i18n("New Folder…"));
    }

    // we can ATM only handle file renames
    QAction *rename = nullptr;
    QAction *fileDelete = nullptr;
    if (index.data(KateProjectItem::TypeRole).toInt() == KateProjectItem::File) {
        rename = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("&Rename"));
        fileDelete = menu.addAction(QIcon::fromTheme(QStringLiteral("delete")), i18n("Delete"));
    }

    /**
     * File Properties Dialog
     */
    auto filePropertiesAction = menu.addAction(QIcon::fromTheme(QStringLiteral("dialog-object-properties")), i18n("Properties"));

    QUrl url = QUrl::fromLocalFile(filename);
    menu.addSeparator();
    QMenu *openWithMenu = menu.addMenu(QIcon::fromTheme(QStringLiteral("system-run")), i18n("Open With"));
    KateFileActions::prepareOpenWithMenu(url, openWithMenu);
    openWithMenu->setEnabled(!openWithMenu->isEmpty());

    /**
     * Open external terminal here
     */
    if (KateProjectInfoViewTerminal::isLoadable() && KAuthorized::authorize(QStringLiteral("shell_access"))) {
        menu.addAction(QIcon::fromTheme(QStringLiteral("terminal")), i18n("Open Internal Terminal Here"), [parent, &filename]() {
            QFileInfo checkFile(filename);
            if (checkFile.isFile()) {
                parent->openTerminal(checkFile.absolutePath());
            } else {
                parent->openTerminal(filename);
            }
        });
    }

    QAction *terminal = nullptr;
    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        terminal = menu.addAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18n("Open External Terminal Here"));
    }

    /**
     * Open Containing folder
     */
    auto openContaingFolderAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("&Open Containing Folder"));

    /**
     * Git history
     */
    QAction *fileHistory = nullptr;
    QMenu gitMenu; // must live as long as the maybe filled menu items should live
    if (GitUtils::isGitRepo(QFileInfo(filename).absolutePath())) {
        menu.addSeparator();
        fileHistory = menu.addAction(i18n("Show Git History"));
        fileHistory->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));
    }

    auto externaltoolsplugin = parent->mainWindow()->pluginView(QStringLiteral("externaltoolsplugin"));
    auto view = parent->mainWindow()->activeView();
    auto doc = view ? view->document() : nullptr;
    if (doc && externaltoolsplugin) {
        QAction *a = nullptr;
        QMetaObject::invokeMethod(externaltoolsplugin, "externalToolsForDocumentAction", Q_RETURN_ARG(QAction *, a), doc);
        if (a) {
            a->setParent(&menu);
            menu.addAction(a);
        }
    }

    /**
     * run menu and handle the triggered action
     */
    if (QAction *const action = menu.exec(pos)) {
        if (action == copyAction) {
            QApplication::clipboard()->setText(filename);
        } else if (terminal && action == terminal) {
            // handle "open terminal here"
            QFileInfo checkFile(filename);
            auto *job = new KTerminalLauncherJob(QString());
            if (checkFile.isFile()) {
                job->setWorkingDirectory(checkFile.absolutePath());
            } else {
                job->setWorkingDirectory(filename);
            }
            job->start();
        } else if (action->parent() == openWithMenu) {
            KateFileActions::showOpenWithMenu(parent, url, action);
        } else if (action == openContaingFolderAction) {
            KIO::highlightInFileManager({url});
        } else if (fileDelete && action == fileDelete) {
            onDeleteFile(index, filename, parent);
        } else if (action == filePropertiesAction) {
            // code copied and adapted from frameworks/kio/src/filewidgets/knewfilemenu.cpp
            KFileItem fileItem(url);
            QDialog *dlg = new KPropertiesDialog(fileItem, parent);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->show();
        } else if (rename && action == rename) {
            if (!index.isValid()) {
                return;
            }
            /**
             * hack:
             * We store a reference to project in the item so that
             * after rename we can update file2Item map properly.
             */
            KateProjectItem *item = parent->project()->itemForFile(index.data(Qt::UserRole).toString());
            if (!item) {
                return;
            }
            item->setData(QVariant::fromValue(parent->project()), KateProjectItem::ProjectRole);

            /** start the edit */
            parent->edit(index);
        } else if (action == fileHistory) {
            FileHistory::showFileHistory(index.data(Qt::UserRole).toString());
        } else if (addFile && action == addFile) {
            QString name = getName(parent, i18n("New File"));
            if (!name.isEmpty()) {
                parent->addFile(index, name);
            }
        } else if (addFolder && action == addFolder) {
            QString name = getName(parent, i18n("New Folder"));
            if (!name.isEmpty()) {
                parent->addDirectory(index, name);
            }
        } else {
            // One of the git actions was triggered
        }
    }
}
