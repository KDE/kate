/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2018 Gregor Mi <codestruct@posteo.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 **/

#include "katefileactions.h"

#include <ktexteditor/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/OpenFileManagerWindowJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPropertiesDialog>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QInputDialog>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

void KateFileActions::copyFilePathToClipboard(KTextEditor::Document *doc)
{
    QApplication::clipboard()->setText(doc->url().toDisplayString(QUrl::PreferLocalFile));
}

void KateFileActions::openContainingFolder(KTextEditor::Document *doc)
{
    KIO::highlightInFileManager({doc->url()});
}

void KateFileActions::openFilePropertiesDialog(KTextEditor::Document *doc)
{
    KFileItem fileItem(doc->url());
    QDialog *dlg = new KPropertiesDialog(fileItem);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void KateFileActions::renameDocumentFile(QWidget *parent, KTextEditor::Document *doc)
{
    // TODO: code was copied and adapted from ../addons/filetree/katefiletree.cpp
    // (-> DUPLICATE CODE, the new code here should be also used there!)

    if (!doc) {
        return;
    }

    const QUrl oldFileUrl = doc->url();

    if (oldFileUrl.isEmpty()) { // NEW
        return;
    }

    const QString oldFileName = doc->url().fileName();
    bool ok = false;
    QString newFileName = QInputDialog::getText(parent, // ADAPTED
                                                i18n("Rename file"),
                                                i18n("New file name"),
                                                QLineEdit::Normal,
                                                oldFileName,
                                                &ok);
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

    KIO::CopyJob *job = KIO::move(oldFileUrl, newFileUrl);
    parent->connect(parent, &QObject::destroyed, job, [job]() {
        job->kill();
    });

    // Associate the job with the parent widget, in case of renaming conflicts the ask-user-dialog
    // is window-modal by default
    KJobWidgets::setWindow(job, parent);

    parent->connect(job, &KJob::result, parent, [parent, doc, oldFileUrl](KJob *job) {
        auto *copyJob = static_cast<KIO::CopyJob *>(job);
        if (!copyJob->error()) {
            doc->openUrl(copyJob->destUrl());
            doc->documentSavedOrUploaded(doc, true);
        } else {
            KMessageBox::sorry(parent,
                               i18n("File \"%1\" could not be moved to \"%2\"",
                                    oldFileUrl.toDisplayString(QUrl::PreferLocalFile),
                                    copyJob->destUrl().toDisplayString(QUrl::PreferLocalFile)));
            doc->openUrl(oldFileUrl);
        }
    });
}

void KateFileActions::deleteDocumentFile(QWidget *parent, KTextEditor::Document *doc)
{
    // TODO: code was copied and adapted from ../addons/filetree/katefiletree.cpp
    //       (-> DUPLICATE CODE, the new code here should be also used there!)

    if (!doc) {
        return;
    }

    const auto &&url = doc->url();

    if (url.isEmpty()) { // NEW
        return;
    }

    bool go = (KMessageBox::warningContinueCancel(parent,
                                                  i18n("Do you really want to delete file \"%1\"?", url.toDisplayString()),
                                                  i18n("Delete file"),
                                                  KStandardGuiItem::yes(),
                                                  KStandardGuiItem::no(),
                                                  QStringLiteral("filetreedeletefile"))
               == KMessageBox::Continue);

    if (!go) {
        return;
    }

    if (!KTextEditor::Editor::instance()->application()->closeDocument(doc)) {
        return; // no extra message, the internals of ktexteditor should take care of that.
    }

    if (url.isValid()) {
        KIO::DeleteJob *job = KIO::del(url);
        if (!job->exec()) {
            KMessageBox::sorry(parent, i18n("File \"%1\" could not be deleted.", url.toDisplayString()));
        }
    }
}

QVector<std::pair<QString, QString>> KateFileActions::supportedDiffTools()
{
    // query once if the tools are there in the path and store that
    // we will disable the actions for the tools not found
    static QVector<std::pair<QString, QString>> resultList{{QStringLiteral("kdiff3"), QStandardPaths::findExecutable(QStringLiteral("kdiff3"))},
                                                           {QStringLiteral("kompare"), QStandardPaths::findExecutable(QStringLiteral("kompare"))},
                                                           {QStringLiteral("meld"), QStandardPaths::findExecutable(QStringLiteral("meld"))}};
    return resultList;
}

bool KateFileActions::compareWithExternalProgram(KTextEditor::Document *documentA, KTextEditor::Document *documentB, const QString &diffExecutable)
{
    Q_ASSERT(documentA);
    Q_ASSERT(documentB);

    QProcess process;
    QStringList arguments;
    arguments << documentA->url().toLocalFile() << documentB->url().toLocalFile();
    return process.startDetached(diffExecutable, arguments);
}
