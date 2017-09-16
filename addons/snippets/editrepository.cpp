/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2010 Milian Wolff <mail@milianw.de>
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2014 Sven Brauch <svenbrauch@gmail.com>
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

#include "editrepository.h"
#include "snippetrepository.h"
#include "snippetstore.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/document.h>

#include <KLocalizedString>
#include <KStandardGuiItem>
#include <KUser>

#include <QPushButton>
#include <QDialogButtonBox>

EditRepository::EditRepository(SnippetRepository* repository, QWidget* parent)
    : QDialog(parent), Ui::EditRepositoryBase(), m_repo(repository)
{
    setupUi(this);

    connect(repoNameEdit, &KLineEdit::textEdited, this, &EditRepository::validate);
    connect(this, &QDialog::accepted, this, &EditRepository::save);

    auto ok = buttonBox->button(QDialogButtonBox::Ok);
    KGuiItem::assign(ok, KStandardGuiItem::ok());
    connect(ok, &QPushButton::clicked, this, &EditRepository::accept);

    auto cancel = buttonBox->button(QDialogButtonBox::Cancel);
    KGuiItem::assign(cancel, KStandardGuiItem::cancel());
    connect(cancel, &QPushButton::clicked, this, &EditRepository::reject);

    // fill list of available modes
    QSharedPointer<KTextEditor::Document> document(KTextEditor::Editor::instance()->createDocument(nullptr));
    repoFileTypesList->addItems(document->highlightingModes());
    repoFileTypesList->sortItems();
    repoFileTypesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(repoFileTypesList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EditRepository::updateFileTypes);

    // add default licenses
    repoLicenseEdit->addItems(QStringList() << QLatin1String("BSD") << QLatin1String("Artistic") << QLatin1String("LGPL v2+") << QLatin1String("LGPL v3+"));
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

void EditRepository::validate()
{
    bool valid = !repoNameEdit->text().isEmpty() && !repoNameEdit->text().contains(QLatin1Char('/'));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
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
        repoFileTypesListLabel->setText(types.join(QLatin1String(", ")));
    }
}
