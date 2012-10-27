/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "editrepository.h"

#include "snippetrepository.h"

#include <KLocalizedString>

#include <KTextEditor/EditorChooser>
#include <KPushButton>

#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>

#include <KUser>

#include "snippetstore.h"

EditRepository::EditRepository(SnippetRepository* repository, QWidget* parent)
    : KDialog(parent), Ui::EditRepositoryBase(), m_repo(repository)
{
    setButtons(/*Reset | */Apply | Cancel | Ok);
    setupUi(mainWidget());
    mainWidget()->layout()->setMargin(0);

    connect(this, SIGNAL(okClicked()), this, SLOT(save()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(save()));

    connect(repoNameEdit, SIGNAL(textEdited(QString)), this, SLOT(validate()));

    // fill list of available modes
    KTextEditor::Document *document = KTextEditor::EditorChooser::editor()->createDocument(0);
    repoFileTypesList->addItems(document->highlightingModes());
    repoFileTypesList->sortItems();
    repoFileTypesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(repoFileTypesList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateFileTypes()));

    delete document;

    // add default licenses
    repoLicenseEdit->addItems(QStringList() << "Artistic" << "BSD" << "LGPL v2+" << "LGPL v3+");
    repoLicenseEdit->setCurrentIndex(1); // preselect BSD
    repoLicenseEdit->setEditable(true);

    // if we edit a repo, add all existing data
    if ( m_repo ) {
        repoNameEdit->setText(m_repo->text());
        repoAuthorsEdit->setText(m_repo->authors());
        repoNamespaceEdit->setText(m_repo->completionNamespace());
        if ( !m_repo->license().isEmpty() ) {
            int index = repoLicenseEdit->findText(m_repo->license());
            if ( index == -1 ) {
                repoLicenseEdit->addItem(m_repo->license());
                repoLicenseEdit->model()->sort(0);
                index = repoLicenseEdit->findText(m_repo->license());
            }
            repoLicenseEdit->setCurrentIndex(index);
        }
        foreach ( const QString& type, m_repo->fileTypes() ) {
            foreach( QListWidgetItem* item, repoFileTypesList->findItems(type, Qt::MatchExactly) ) {
                item->setSelected(true);
            }
        }

        setWindowTitle(i18n("Edit Snippet Repository %1", m_repo->text()));
    } else {
        setWindowTitle(i18n("Create New Snippet Repository"));
        KUser user;
        repoAuthorsEdit->setText(user.property(KUser::FullName).toString());
    }

    validate();
    updateFileTypes();
    repoNameEdit->setFocus();
}

EditRepository::~EditRepository()
{
}

void EditRepository::validate()
{
    bool valid = !repoNameEdit->text().isEmpty() && !repoNameEdit->text().contains('/');
    button(Ok)->setEnabled(valid);
    button(Apply)->setEnabled(valid);
}

void EditRepository::save()
{
    Q_ASSERT(!repoNameEdit->text().isEmpty());
    if ( !m_repo ) {
        // save as new repo
        m_repo = SnippetRepository::createRepoFromName(repoNameEdit->text());
    }
    m_repo->setText(repoNameEdit->text());
    m_repo->setAuthors(repoAuthorsEdit->text());
    m_repo->setLicense(repoLicenseEdit->currentText());
    m_repo->setCompletionNamespace(repoNamespaceEdit->text());

    QStringList types;
    foreach( QListWidgetItem* item, repoFileTypesList->selectedItems() ) {
        types << item->text();
    }
    m_repo->setFileTypes(types);
    m_repo->save();

    setWindowTitle(i18n("Edit Snippet Repository %1", m_repo->text()));
}

void EditRepository::updateFileTypes()
{
    QStringList types;
    foreach( QListWidgetItem* item, repoFileTypesList->selectedItems() ) {
        types << item->text();
    }
    if ( types.isEmpty() ) {
        repoFileTypesListLabel->setText(i18n("<i>leave empty for general purpose snippets</i>"));
    } else {
        repoFileTypesListLabel->setText(types.join(", "));
    }
}

#include "editrepository.moc"
