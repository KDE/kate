/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2010 Milian Wolff <mail@milianw.de>
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "editsnippet.h"

#include "ui_editsnippet.h"

#include "snippetrepository.h"
#include "snippetstore.h"
#include "snippet.h"

#include <KLocalizedString>
#include <KMimeTypeTrader>
#include <khelpclient.h>
#include <KMessageBox>
#include <KMessageWidget>

#include <QToolButton>
#include <QPushButton>

#include <KTextEditor/Editor>
#include <KTextEditor/Document>
#include <KTextEditor/View>

QPair<KTextEditor::View*, QToolButton*> getViewForTab(QWidget* tabWidget)
{
    QVBoxLayout* layout = new QVBoxLayout;
    tabWidget->setLayout(layout);

    KTextEditor::Document *document = KTextEditor::Editor::instance()->createDocument (tabWidget);
    KTextEditor::View *view = document->createView (tabWidget);

    Q_ASSERT(view);
    Q_ASSERT(view->action("file_save"));
    view->action("file_save")->setEnabled(false);

    layout->addWidget(view);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addStretch();

    QToolButton* button = new QToolButton;
    button->setText(i18n("Help"));
    button->setIcon(QIcon::fromTheme(QLatin1String("help-contents")));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hlayout->addWidget(button);
    layout->addLayout(hlayout);

    return qMakePair(view, button);
}

EditSnippet::EditSnippet(SnippetRepository* repository, Snippet* snippet, QWidget* parent)
    : QDialog(parent), m_ui(new Ui::EditSnippetBase), m_repo(repository)
    , m_snippet(snippet), m_topBoxModified(false)
{
    Q_ASSERT(m_repo);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QWidget *w = new QWidget(this);
    mainLayout->addWidget(w);
    m_ui->setupUi(w);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    mainLayout->addWidget(buttons);

    m_okButton = new QPushButton;
    KGuiItem::assign(m_okButton, KStandardGuiItem::ok());
    buttons->addButton(m_okButton, QDialogButtonBox::AcceptRole);
    connect(m_okButton, SIGNAL(clicked()), this, SLOT(saveAndAccept()));

    m_applyButton = new QPushButton;
    KGuiItem::assign(m_applyButton, KStandardGuiItem::apply());
    buttons->addButton(m_applyButton, QDialogButtonBox::ApplyRole);
    connect(m_applyButton, SIGNAL(clicked()), this, SLOT(save()));

    QPushButton *cancelButton = new QPushButton;
    KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
    buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    ///TODO: highlighting and documentation of template handler variables
    QPair<KTextEditor::View*, QToolButton*> pair = getViewForTab(m_ui->snippetTab);
    m_snippetView = pair.first;
    if (!m_repo->fileTypes().isEmpty()) {
        m_snippetView->document()->setMode(m_repo->fileTypes().first());
    }
    connect(pair.second, SIGNAL(clicked(bool)),
            this, SLOT(slotSnippetDocumentation()));
    ///TODO: highlighting and documentation of KTextEditor API
    pair = getViewForTab(m_ui->scriptTab);
    m_scriptsView = pair.first;
    m_scriptsView->document()->setMode(QLatin1String("JavaScript"));
    m_scriptsView->document()->setText(m_repo->script());
    m_scriptsView->document()->setModified(false);
    connect(pair.second, SIGNAL(clicked(bool)),
            this, SLOT(slotScriptDocumentation()));

    m_ui->verticalLayout->setMargin(0);
    m_ui->formLayout->setMargin(0);

    m_ui->snippetShortcutWidget->layout()->setMargin(0);

    connect(m_ui->snippetNameEdit,       SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetNameEdit,       SIGNAL(textEdited(QString)), this, SLOT(validate()));
    connect(m_ui->snippetArgumentsEdit,  SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetPostfixEdit,    SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetPrefixEdit,     SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetShortcutWidget, SIGNAL(shortcutChanged(QList<QKeySequence>)), this, SLOT(topBoxModified()));
    connect(m_snippetView->document(), SIGNAL(textChanged(KTextEditor::Document*)), this, SLOT(validate()));

    // if we edit a snippet, add all existing data
    if ( m_snippet ) {
        setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));

        m_ui->snippetArgumentsEdit->setText(m_snippet->arguments());
        m_snippetView->document()->setText(m_snippet->snippet());
        m_ui->snippetNameEdit->setText(m_snippet->text());
        m_ui->snippetPostfixEdit->setText(m_snippet->postfix());
        m_ui->snippetPrefixEdit->setText(m_snippet->prefix());
        m_ui->snippetShortcutWidget->setShortcut(m_snippet->action()->shortcuts());

        // unset modified flags
        m_snippetView->document()->setModified(false);
        m_topBoxModified = false;
    } else {
        setWindowTitle(i18n("Create New Snippet in Repository %1", m_repo->text()));
    }

    validate();

    m_ui->snippetNameEdit->setFocus();

    QSize initSize = sizeHint();
    initSize.setHeight( initSize.height() + 200 );
}

EditSnippet::~EditSnippet()
{
    delete m_ui;
}

void EditSnippet::setSnippetText( const QString& text )
{
    m_snippetView->document()->setText(text);
    validate();
}

void EditSnippet::validate()
{
    const QString& name = m_ui->snippetNameEdit->text();
    bool valid = !name.isEmpty() && !m_snippetView->document()->isEmpty();
    if (valid) {
        // make sure the snippetname includes no spaces
        for ( int i = 0; i < name.length(); ++i ) {
            if ( name.at(i).isSpace() ) {
                valid = false;
                m_ui->messageWidget->setText(i18n("Snippet name cannot contain spaces"));
                m_ui->messageWidget->animatedShow();
                break;
            }
        }
        if (valid) {
            // hide message widget if snippet does not include spaces
            m_ui->messageWidget->animatedHide();
        }
    }
    m_okButton->setEnabled(valid);
    m_applyButton->setEnabled(valid);
}

void EditSnippet::saveAndAccept()
{
  save();
  accept();
}

void EditSnippet::save()
{
    Q_ASSERT(!m_ui->snippetNameEdit->text().isEmpty());

    if ( !m_snippet ) {
        // save as new snippet
        m_snippet = new Snippet();
        m_repo->appendRow(m_snippet);
    }
    m_snippet->setArguments(m_ui->snippetArgumentsEdit->text());
    m_snippet->setSnippet(m_snippetView->document()->text());
    m_snippetView->document()->setModified(false);
    m_snippet->setText(m_ui->snippetNameEdit->text());
    m_snippet->setPostfix(m_ui->snippetPostfixEdit->text());
    m_snippet->setPrefix(m_ui->snippetPrefixEdit->text());
    m_snippet->action()->setShortcuts(m_ui->snippetShortcutWidget->shortcut());
    m_repo->setScript(m_scriptsView->document()->text());
    m_scriptsView->document()->setModified(false);
    m_topBoxModified = false;
    m_repo->save();

    setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));
}

void EditSnippet::slotSnippetDocumentation()
{
    KHelpClient::invokeHelp(QLatin1String("kate-application-plugin-snippets"), QLatin1String("kate"));
}

void EditSnippet::slotScriptDocumentation()
{
    KHelpClient::invokeHelp(QLatin1String("dev-scripting-api"), QLatin1String("kate"));
}

void EditSnippet::reject()
{
    if (m_topBoxModified || m_snippetView->document()->isModified() || m_scriptsView->document()->isModified()) {
        int ret = KMessageBox::warningContinueCancel(qApp->activeWindow(),
            i18n("The snippet contains unsaved changes. Do you want to continue and lose all changes?"),
            i18n("Warning - Unsaved Changes")
        );
        if (ret == KMessageBox::Cancel) {
            return;
        }
    }
    QDialog::reject();
}

void EditSnippet::topBoxModified()
{
    m_topBoxModified = true;
}

#include "editsnippet.moc"
