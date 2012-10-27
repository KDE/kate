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

#include "editsnippet.h"

#include "ui_editsnippet.h"

#include "snippetrepository.h"

#include <QToolButton>

#include <KLocalizedString>
#include <KPushButton>
#include <KAction>
#include <KMimeTypeTrader>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KToolInvocation>
#include <KMessageBox>

#include "snippetstore.h"
#include "snippet.h"

QPair<KTextEditor::View*, QToolButton*> getViewForTab(QWidget* tabWidget)
{
    QVBoxLayout* layout = new QVBoxLayout;
    tabWidget->setLayout(layout);
    KParts::ReadWritePart* part= KMimeTypeTrader::self()->createPartInstanceFromQuery<KParts::ReadWritePart>(
                                        "text/plain", tabWidget, tabWidget);
    KTextEditor::Document* document = qobject_cast<KTextEditor::Document*>(part);
    Q_ASSERT(document);
    Q_ASSERT(document->action("file_save"));
    document->action("file_save")->setEnabled(false);

    KTextEditor::View* view = qobject_cast< KTextEditor::View* >( document->widget() );
    layout->addWidget(view);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addStretch();

    QToolButton* button = new QToolButton;
    button->setText(i18n("Show Documentation"));
    button->setIcon(KIcon("help-about"));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hlayout->addWidget(button);
    layout->addLayout(hlayout);

    return qMakePair(view, button);
}

EditSnippet::EditSnippet(SnippetRepository* repository, Snippet* snippet, QWidget* parent)
    : KDialog(parent), m_ui(new Ui::EditSnippetBase), m_repo(repository)
    , m_snippet(snippet), m_topBoxModified(false)
{
    Q_ASSERT(m_repo);

    setButtons(/*Reset | */Apply | Cancel | Ok);
    m_ui->setupUi(mainWidget());

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
    m_scriptsView->document()->setMode("JavaScript");
    m_scriptsView->document()->setText(m_repo->script());
    m_scriptsView->document()->setModified(false);
    connect(pair.second, SIGNAL(clicked(bool)),
            this, SLOT(slotScriptDocumentation()));

    m_ui->verticalLayout->setMargin(0);
    m_ui->formLayout->setMargin(0);

    m_ui->snippetShortcutWidget->layout()->setMargin(0);

    connect(this, SIGNAL(okClicked()), this, SLOT(save()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(save()));

    connect(m_ui->snippetNameEdit,       SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetNameEdit,       SIGNAL(textEdited(QString)), this, SLOT(validate()));
    connect(m_ui->snippetArgumentsEdit,  SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetPostfixEdit,    SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetPrefixEdit,     SIGNAL(textEdited(QString)), this, SLOT(topBoxModified()));
    connect(m_ui->snippetShortcutWidget, SIGNAL(shortcutChanged(KShortcut)), this, SLOT(topBoxModified()));
    connect(m_snippetView->document(), SIGNAL(textChanged(KTextEditor::Document*)), this, SLOT(validate()));

    // if we edit a snippet, add all existing data
    if ( m_snippet ) {
        setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));

        m_ui->snippetArgumentsEdit->setText(m_snippet->arguments());
        m_snippetView->document()->setText(m_snippet->snippet());
        m_ui->snippetNameEdit->setText(m_snippet->text());
        m_ui->snippetPostfixEdit->setText(m_snippet->postfix());
        m_ui->snippetPrefixEdit->setText(m_snippet->prefix());
        m_ui->snippetShortcutWidget->setShortcut(m_snippet->action()->shortcut());

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
    setInitialSize(initSize);
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
                break;
            }
        }
    }
    button(Ok)->setEnabled(valid);
    button(Apply)->setEnabled(valid);
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
    m_snippet->action()->setShortcut(m_ui->snippetShortcutWidget->shortcut());
    m_repo->setScript(m_scriptsView->document()->text());
    m_scriptsView->document()->setModified(false);
    m_topBoxModified = false;
    m_repo->save();

    setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));
}

void EditSnippet::slotSnippetDocumentation()
{
    KToolInvocation::invokeHelp("katefiletemplates-format", "kate");
}

void EditSnippet::slotScriptDocumentation()
{
    KToolInvocation::invokeHelp("advanced-editing-tools-scripting-api", "kate");
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
