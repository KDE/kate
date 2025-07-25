/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2018 Gregor Mi <codestruct@posteo.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 **/

#include "katefileactions.h"

#include "hostprocess.h"

#include <KTextEditor/Document>
#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPropertiesDialog>
#include <KService>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QInputDialog>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

void KateFileActions::copyFilePathToClipboard(KTextEditor::Document *doc)
{
    const QUrl url = doc->url();
    // ensure we prefer native separators, bug 381052
    QApplication::clipboard()->setText(url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.url());
}

void KateFileActions::copyFileNameToClipboard(KTextEditor::Document *doc)
{
    QApplication::clipboard()->setText(doc->url().fileName());
}

void KateFileActions::openContainingFolder(KTextEditor::Document *doc)
{
    KIO::highlightInFileManager({doc->url()});
}

void KateFileActions::openFilePropertiesDialog(QWidget *parent, KTextEditor::Document *doc)
{
    KFileItem fileItem(doc->url());
    QDialog *dlg = new KPropertiesDialog(fileItem, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void KateFileActions::renameDocumentFile(QWidget *parent, KTextEditor::Document *doc)
{
    if (!doc) {
        return;
    }

    const QUrl oldFileUrl = doc->url();
    if (oldFileUrl.isEmpty()) {
        return;
    }

    const QString oldFileName = doc->url().fileName();
    bool ok = false;
    QString newFileName = QInputDialog::getText(parent, i18n("Rename file"), i18n("New file name"), QLineEdit::Normal, oldFileName, &ok);
    if (!ok || (newFileName == oldFileName)) {
        return;
    }

    QUrl newFileUrl = oldFileUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
    newFileUrl.setPath(newFileUrl.path() + QLatin1Char('/') + newFileName);

    if (!newFileUrl.isValid()) {
        return;
    }

    if (!doc->closeUrl()) {
        return;
    }

    doc->waitSaveComplete();

    KIO::CopyJob *job = KIO::moveAs(oldFileUrl, newFileUrl);
    QWidget::connect(parent, &QObject::destroyed, job, [job]() {
        job->kill();
    });

    // Associate the job with the parent widget, in case of renaming conflicts the ask-user-dialog
    // is window-modal by default
    KJobWidgets::setWindow(job, parent);

    QWidget::connect(job, &KJob::result, parent, [parent, doc, oldFileUrl](KJob *job) {
        auto *copyJob = static_cast<KIO::CopyJob *>(job);
        if (!copyJob->error()) {
            doc->openUrl(copyJob->destUrl());
            doc->documentSavedOrUploaded(doc, true);
        } else {
            KMessageBox::error(parent,
                               i18n("File \"%1\" could not be moved to \"%2\"",
                                    oldFileUrl.toDisplayString(QUrl::PreferLocalFile),
                                    copyJob->destUrl().toDisplayString(QUrl::PreferLocalFile)));
            doc->openUrl(oldFileUrl);
        }
    });
}

void KateFileActions::deleteDocumentFile(QWidget *parent, KTextEditor::Document *doc)
{
    if (!doc) {
        return;
    }

    const auto url = doc->url();

    if (url.isEmpty()) {
        return;
    }

    bool go = (KMessageBox::warningContinueCancel(parent,
                                                  i18n("Do you really want to delete file \"%1\"?", url.toDisplayString()),
                                                  i18n("Delete file"),
                                                  KStandardGuiItem::del(),
                                                  KStandardGuiItem::cancel(),
                                                  QStringLiteral("filetreedeletefile"))
               == KMessageBox::Continue);

    if (!go) {
        return;
    }

    // If the document is modified, user will be asked if he wants to save it.
    // This confirmation is useless when deleting, so we mark the document as unmodified.
    doc->setModified(false);

    if (!KTextEditor::Editor::instance()->application()->closeDocument(doc)) {
        return; // no extra message, the internals of ktexteditor should take care of that.
    }

    if (url.isValid()) {
        KIO::DeleteJob *job = KIO::del(url);
        if (!job->exec()) {
            KMessageBox::error(parent, i18n("File \"%1\" could not be deleted.", url.toDisplayString()));
        }
    }
}

QList<KateFileActions::DiffTool> KateFileActions::supportedDiffTools()
{
    // query once if the tools are there in the path and store that
    // we will disable the actions for the tools not found
    static QList<DiffTool> resultList{{QStringLiteral("kdiff3"), safeExecutableName(QStringLiteral("kdiff3"))},
                                      {QStringLiteral("kompare"), safeExecutableName(QStringLiteral("kompare"))},
                                      {QStringLiteral("meld"), safeExecutableName(QStringLiteral("meld"))}};
    return resultList;
}

bool KateFileActions::compareWithExternalProgram(KTextEditor::Document *documentA, KTextEditor::Document *documentB, const QString &diffExecutable)
{
    Q_ASSERT(documentA);
    Q_ASSERT(documentB);

    QProcess process;
    QStringList arguments;
    arguments << documentA->url().toLocalFile() << documentB->url().toLocalFile();
    return QProcess::startDetached(diffExecutable, arguments);
}

void KateFileActions::prepareOpenWithMenu(const QUrl &url, QMenu *menu)
{
    // dh: in bug #307699, this slot is called when launching the Kate application
    // unfortunately, no one ever could reproduce except users.

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForUrl(url);

    menu->clear();

    // get a list of appropriate services.
    const KService::List offers = KApplicationTrader::queryByMimeType(mime.name());
    QAction *a = nullptr;

    // add all default open-with-actions except "Kate"
    for (const auto &service : offers) {
        if (service->name() == QLatin1String("Kate")) {
            continue;
        }
        a = menu->addAction(QIcon::fromTheme(service->icon()), service->name());
        a->setData(service->entryPath());
    }

    // append "Other..." to call the KDE "open with" dialog.
    menu->addSeparator();
    QAction *other = menu->addAction(QIcon::fromTheme(QStringLiteral("system-run")), i18n("&Other Application..."));
    other->setData(QString());
}

void KateFileActions::showOpenWithMenu(QWidget *parent, const QUrl &url, QAction *action)
{
    KService::Ptr app = KService::serviceByDesktopPath(action->data().toString());
    // If app is null, ApplicationLauncherJob will invoke the open-with dialog
    auto *job = new KIO::ApplicationLauncherJob(app);
    job->setUrls({url});
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parent));
    job->start();
}
