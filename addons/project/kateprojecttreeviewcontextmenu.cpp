/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2013 Dominik Haumann <dhaumann.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojecttreeviewcontextmenu.h"
#include "git/gitutils.h"
#include "kateprojectviewtree.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KNS3/KMoreTools>
#include <KNS3/KMoreToolsMenuFactory>
#include <KPropertiesDialog>

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

class AskNameDialog : public QDialog
{
public:
    struct Name {
        Name(const QString &n, bool s)
            : name{n}
            , success{s}
        {
        }
        QString name;
        bool success = false;
    };

    AskNameDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout;
        setLayout(layout);
        layout->addWidget(&m_lineEdit);

        QHBoxLayout *hl = new QHBoxLayout;
        hl->addWidget(&m_addBtn);
        hl->addWidget(&m_cancelBtn);

        m_addBtn.setText(i18n("Add"));
        m_cancelBtn.setText(i18n("Cancel"));

        connect(&m_addBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(&m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        layout->addLayout(hl);
    }

    Name askName()
    {
        int res = exec();
        bool suc = res == QDialog::Accepted;
        if (!suc || m_lineEdit.text().isEmpty()) {
            return {{}, false};
        }
        return {m_lineEdit.text(), true};
    }

private:
    QLineEdit m_lineEdit;
    QPushButton m_addBtn;
    QPushButton m_cancelBtn;
};

void KateProjectTreeViewContextMenu::exec(const QString &filename, const QModelIndex &index, const QPoint &pos, KateProjectViewTree *parent)
{
    /**
     * Create context menu
     */
    QMenu menu;

    QAction *addFile = nullptr;
    QAction *addFolder = nullptr;
    if (index.data(KateProjectItem::TypeRole).toInt() == KateProjectItem::Directory) {
        addFile = menu.addAction(QIcon::fromTheme(QStringLiteral("document-new")), i18n("&Add File"));
        addFolder = menu.addAction(QIcon::fromTheme(QStringLiteral("folder-new")), i18n("&Add Folder"));
    }

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
    const KService::List offers = KApplicationTrader::queryByMimeType(mimeType.name());
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

    QAction *fileHistory = nullptr;
    /**
     * Git menu
     */
    KMoreToolsMenuFactory menuFactory(QStringLiteral("kate/addons/project/git-tools"));
    QMenu gitMenu; // must live as long as the maybe filled menu items should live
    if (GitUtils::isGitRepo(QFileInfo(filename).absolutePath())) {
        fileHistory = menu.addAction(i18n("Show File History"));

        menuFactory.fillMenuFromGroupingNames(&gitMenu, {QLatin1String("git-clients-and-actions")}, QUrl::fromLocalFile(filename));
        menu.addSection(i18n("Git:"));
        const auto gitActions = gitMenu.actions();
        for (auto action : gitActions) {
            menu.addAction(action);
        }
    }

    auto handleOpenWith = [parent](QAction *action, const QString &filename) {
        KService::Ptr app = KService::serviceByDesktopPath(action->data().toString());
        // If app is null, ApplicationLauncherJob will invoke the open-with dialog
        auto *job = new KIO::ApplicationLauncherJob(app);
        job->setUrls({QUrl::fromLocalFile(filename)});
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parent));
        job->start();
    };

    // we can ATM only handle file renames
    QAction *rename = nullptr;
    if (index.data(KateProjectItem::TypeRole).toInt() == KateProjectItem::File) {
        rename = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("&Rename"));
    }

    /**
     * run menu and handle the triggered action
     */
    if (QAction *const action = menu.exec(pos)) {
        if (action == copyAction) {
            QApplication::clipboard()->setText(filename);
        } else if (action->parentWidget() == openWithMenu) {
            // handle "open with"
            handleOpenWith(action, filename);
        } else if (action == openContaingFolderAction) {
            KIO::highlightInFileManager({QUrl::fromLocalFile(filename)});
        } else if (action == filePropertiesAction) {
            // code copied and adapted from frameworks/kio/src/filewidgets/knewfilemenu.cpp
            KFileItem fileItem(QUrl::fromLocalFile(filename));
            QDialog *dlg = new KPropertiesDialog(fileItem);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->show();
        } else if (rename && action == rename) {
            parent->edit(index);
        } else if (action == fileHistory) {
            showFileHistory(index.data(Qt::UserRole).toString());
        } else if (action == addFile) {
            AskNameDialog d;
            auto name = d.askName();
            if (name.success) {
                parent->addFile(index, name.name);
            }
        } else if (action == addFolder) {
            AskNameDialog d;
            auto name = d.askName();
            if (name.success) {
                parent->addDirectory(index, name.name);
            }
        } else {
            // One of the git actions was triggered
        }
    }
}
