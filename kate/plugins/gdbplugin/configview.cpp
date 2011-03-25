//
// configview.cpp
//
// Description: View for configuring the set of targets to be used with the debugger
//
//
// Copyright (c) 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#include "configview.h"
#include "configview.moc"

#include <QtGui/QCompleter>
#include <QtGui/QDirModel>
#include <QtGui/QLayout>

#include <klocale.h>
#include <kicon.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kfiledialog.h>


ConfigView::ChangingTarget::ChangingTarget( ConfigView* view )
:   m_view( view )
{
    ++m_view->m_changingTarget;
}

ConfigView::ChangingTarget::~ChangingTarget()
{
    --m_view->m_changingTarget;
}


ConfigView::ConfigView( QWidget* parent, Kate::MainWindow* mainWin )
:   QWidget( parent ),
    m_mainWindow( mainWin ),
    m_changingTarget( 0 )
{
    m_targets = new QComboBox();
    m_targets->setEditable( true );
    // don't let Qt insert items when the user edits; we handle it in the
    // editTextChanged signal instead
    m_targets->setInsertPolicy( QComboBox::NoInsert );

    QCompleter* completer1 = new QCompleter( this );
    completer1->setModel( new QDirModel( completer1 ) );
    m_targets->setCompleter( completer1 );

    m_targetLabel = new QLabel( i18n( "&Target:" ) );
    m_targetLabel->setBuddy( m_targets );

    m_addTarget = new QToolButton();
    m_addTarget->setIcon( SmallIcon("document-new") );
    m_addTarget->setToolTip( i18n("Add executable target") );

    m_deleteTarget = new QToolButton();
    m_deleteTarget->setIcon( SmallIcon("edit-delete") );
    m_deleteTarget->setToolTip( i18n("Remove target") );

    m_workingDirectory = new QLineEdit();

    QCompleter* completer2 = new QCompleter( this );
    completer2->setModel( new QDirModel( completer2 ) );
    m_workingDirectory->setCompleter( completer2 );

    m_workDirLabel = new QLabel( i18n( "&Working Directory:" ) );
    m_workDirLabel->setBuddy( m_workingDirectory );

    m_argumentLists = new QComboBox();
    m_argumentLists->setEditable( true );
    // don't let Qt insert items when the user edits; we handle it in the
    // editTextChanged signal instead
    m_argumentLists->setInsertPolicy( QComboBox::NoInsert );

    m_argumentsLabel = new QLabel( i18nc( "Program argument list", "&Arg List:" ) );
    m_argumentsLabel->setBuddy( m_argumentLists );

    m_addArgList = new QToolButton();
    m_addArgList->setIcon( SmallIcon("document-new") );
    m_addArgList->setToolTip( i18n("Add Argument List") );

    m_deleteArgList = new QToolButton();
    m_deleteArgList->setIcon( SmallIcon("edit-delete") );
    m_deleteArgList->setToolTip( i18n("Remove Argument List") );

    m_takeFocus = new QCheckBox( i18nc( "Checkbox to for keeping focus on the command line",
                                        "Keep focus") );
    m_takeFocus->setToolTip( i18n("Keep the focus on the command line") );

    m_redirectTerminal = new QCheckBox( i18n("Redirect IO") );
    m_redirectTerminal->setToolTip( i18n("Redirect the debugged programs IO to a separate tab") );



    QGridLayout* layout = new QGridLayout( this );
    layout->addWidget( m_targetLabel, 0, 0, Qt::AlignRight );
    layout->addWidget( m_targets, 0, 1, 1, 2 );
    layout->addWidget( m_addTarget, 0, 3 );
    layout->addWidget( m_deleteTarget, 0, 4 );
    layout->addWidget( m_workDirLabel, 1, 0, Qt::AlignRight );
    layout->addWidget( m_workingDirectory, 1, 1, 1, 2 );
    layout->addWidget( m_argumentsLabel, 2, 0, Qt::AlignRight );
    layout->addWidget( m_argumentLists, 2, 1, 1, 2 );
    layout->addWidget( m_addArgList, 2, 3 );
    layout->addWidget( m_deleteArgList, 2, 4 );
    layout->addWidget( m_takeFocus, 3, 1 );
    layout->addWidget( m_redirectTerminal, 3, 2 );
    layout->addItem( new QSpacerItem( 1, 1 ), 4, 0 );
    layout->setColumnStretch( 1, 1 );
    layout->setColumnStretch( 2, 1 );
    layout->setRowStretch( 4, 1 );
    m_useBottomLayout = true;

    // calculate the approximate height to exceed before going to "Side Layout"
    m_widgetHeights = ( m_targetLabel->sizeHint().height() + /*layout spacing */6 ) * 9 ;


    connect(    m_targets, SIGNAL( editTextChanged( QString ) ),
                this, SLOT( slotTargetEdited( QString ) ) );
    connect(    m_targets, SIGNAL( currentIndexChanged( int ) ),
                this, SLOT( slotTargetSelected( int ) ) );
    connect(    m_addTarget, SIGNAL( clicked() ),
                this, SLOT( slotAddTarget() ) );
    connect(    m_deleteTarget, SIGNAL( clicked() ),
                this, SLOT( slotDeleteTarget() ) );
    connect(    m_workingDirectory, SIGNAL( textEdited( QString ) ),
                this, SLOT( slotWorkingDirectoryEdited( QString ) ) );
    connect(    m_argumentLists, SIGNAL( editTextChanged( QString ) ),
                this, SLOT( slotArgListEdited( QString ) ) );
    connect(    m_argumentLists, SIGNAL( currentIndexChanged( int ) ),
                this, SLOT( slotArgListSelected( int ) ) );
    connect(    m_addArgList, SIGNAL( clicked() ),
                this, SLOT( slotAddArgList() ) );
    connect(    m_deleteArgList, SIGNAL( clicked() ),
                this, SLOT( slotDeleteArgList() ) );
    connect(    m_redirectTerminal, SIGNAL( toggled( bool ) ),
                this, SIGNAL(showIO( bool ) ) );
}

ConfigView::~ConfigView()
{
}

void ConfigView::registerActions( KActionCollection* actionCollection )
{
    m_targetSelectAction = actionCollection->add<KSelectAction>( "targets" );
    m_targetSelectAction->setText( i18n( "Targets" ) );
    connect(    m_targetSelectAction, SIGNAL( triggered( int ) ),
                this, SLOT( slotTargetSelected( int ) ) );

    m_argListSelectAction = actionCollection->add<KSelectAction>( "argLists" );
    m_argListSelectAction->setText( i18n( "Arg Lists" ) );
    connect(    m_argListSelectAction, SIGNAL( triggered( int ) ),
                this, SLOT( slotArgListSelected( int ) ) );
}

void ConfigView::readConfig( KConfigBase* config, QString const& groupPrefix )
{
    if( config->hasGroup( groupPrefix ) )
    {
        ChangingTarget  t( this );

        KConfigGroup    group = config->group( groupPrefix );
        int             version = group.readEntry( "version", 1 );

        int         targetCount = group.readEntry( "targetCount", 0 );
        int         lastTarget = group.readEntry( "lastTarget", 0 );
        QString     targetKey( "target_%1" );
        QString     workingDirectory;
        QStringList targetNames;

        for( int i = 0; i < targetCount; i++ )
        {
            QStringList targetDescription = group.readEntry(targetKey.arg( i ),
                                                            QStringList() );

            if( version == 1 )
            {
                if( targetDescription.length() == 3 )
                {
                    // valid old style config, translate it now; note the
                    // reordering happening here!
                    QStringList temp;
                    temp << targetDescription[2];
                    temp << targetDescription[1];
                    targetDescription = temp;
                }
            }

            if( targetDescription.length() > 0 )
            {
                m_targets->addItem( targetDescription[0], targetDescription );

                if( i == lastTarget && targetDescription.length() > 1 )
                {
                    workingDirectory = targetDescription[1];
                }
            }

            targetNames << targetDescription[0];
        }
        m_targetSelectAction->setItems( targetNames );

        int         argListsCount = group.readEntry( "argsCount", 0 );
        QString     argListsKey( "args_%1" );
        QStringList argLists;

        for( int i = 0; i < argListsCount; i++ )
        {
            QString argList = group.readEntry( argListsKey.arg( i ), QString() );
            m_argumentLists->addItem( argList );
            argLists << argList;
        }
        m_argListSelectAction->setItems( argLists );

        m_targets->setCurrentIndex( lastTarget );

        m_takeFocus->setChecked( group.readEntry( "alwaysFocusOnInput",false ) );

        m_redirectTerminal->setChecked( group.readEntry( "redirectTerminal",false ) );
    }
}

void ConfigView::writeConfig( KConfigBase* config, QString const& groupPrefix )
{
    KConfigGroup    group = config->group( groupPrefix );

    group.writeEntry( "version", 3 );

    int     targetCount = m_targets->count();
    QString targetKey( "target_%1" );

    group.writeEntry( "targetCount", targetCount );
    group.writeEntry( "lastTarget", m_targets->currentIndex() );
    for( int i = 0; i < targetCount; i++ )
    {
        QStringList targetDescription = m_targets->itemData( i ).toStringList();
        group.writeEntry( targetKey.arg( i ), targetDescription );
    }


    int     argsCount = m_argumentLists->count();
    QString argsKey( "args_%1" );

    group.writeEntry( "argsCount", argsCount );
    group.writeEntry( "lastArgs", m_argumentLists->currentIndex() );
    for( int i = 0; i < argsCount; i++ )
    {
        group.writeEntry(   argsKey.arg( i ),
                            m_argumentLists->itemText( i ) );
    }


    group.writeEntry( "alwaysFocusOnInput", m_takeFocus->isChecked() );

    group.writeEntry( "redirectTerminal", m_redirectTerminal->isChecked() );

}

QString ConfigView::currentExecutable() const
{
    return m_targets->currentText();
}

QString ConfigView::currentWorkingDirectory() const
{
    return m_workingDirectory->text();
}

QString ConfigView::currentArgs() const
{
    return m_argumentLists->currentText();
}

bool ConfigView::takeFocusAlways() const
{
    return m_takeFocus->isChecked();
}

bool ConfigView::showIOTab() const
{
    return m_redirectTerminal->isChecked();
}

void ConfigView::slotTargetEdited( QString updatedTarget )
{
    if( m_changingTarget == 0 )
    {
        QStringList items;

        // rebuild the target description for the item being edited and the
        // target menu
        for( int i = 0; i < m_targets->count(); ++i )
        {
            if( i == m_targets->currentIndex() )
            {
                updateCurrentTargetDescription( 0, updatedTarget );
                m_targets->setItemText( i, updatedTarget );
                items.append( updatedTarget );
            }
            else
            {
                items.append( m_targets->itemText( i ) );
            }
        }
        m_targetSelectAction->setItems( items );
        m_targetSelectAction->setCurrentItem( m_targets->currentIndex() );
    }
}

void ConfigView::slotTargetSelected( int index )
{
    if( index >= 0 )
    {
        ChangingTarget  t( this );

        // Select the last working directory and argument list used with this target
        QStringList targetDescription = m_targets->itemData( index ).toStringList();
        int         argIndex = -1;

        if( targetDescription.length() > 1 )
        {
            m_workingDirectory->setText( targetDescription[1] );
        }

        if( targetDescription.length() > 2 )
        {
            argIndex = m_argumentLists->findText( targetDescription[2] );
            if( argIndex >= 0 )
            {
                m_argumentLists->setCurrentIndex( argIndex );
                m_argListSelectAction->setCurrentItem( argIndex );
            }
        }

        // Keep combo box and menu in sync
        m_targets->setCurrentIndex( index );
        m_targetSelectAction->setCurrentItem( index );
    }
}

void ConfigView::slotAddTarget()
{
    KUrl    defDir( m_targets->currentText() );

    if( m_targets->currentText().isEmpty() )
    {
        // try current document dir
        KTextEditor::View*  view = m_mainWindow->activeView();

        if( view != NULL )
        {
            defDir = view->document()->url();
        }
    }

    QString target = KFileDialog::getOpenFileName( defDir );
    if( !target.isEmpty() )
    {
        // create the new item. Inherit the current working directory and
        // argument list by default
        QStringList targetDescription;

        targetDescription << target;
        targetDescription << m_workingDirectory->text();
        targetDescription << m_argumentLists->currentText();

        m_targets->insertItem( 0, target, targetDescription );
        m_targets->setCurrentIndex( 0 );
    }
}

void ConfigView::slotDeleteTarget()
{
    int currentIndex = m_targets->currentIndex();
    if( currentIndex >= 0 )
    {
        m_targets->removeItem( currentIndex );
    }
}

void ConfigView::slotWorkingDirectoryEdited( QString updatedWorkingDirectory )
{
    if( m_changingTarget == 0 )
    {
        updateCurrentTargetDescription( 1, updatedWorkingDirectory );
    }
}

void ConfigView::slotArgListEdited( QString updatedArgList )
{
    if( m_changingTarget == 0 )
    {
        QStringList items;

        // rebuild the target description for the current target and update the
        // arg lists menu
        for( int i = 0; i < m_argumentLists->count(); ++i )
        {
            if( i == m_argumentLists->currentIndex() )
            {
                updateCurrentTargetDescription( 2, updatedArgList );
                m_argumentLists->setItemText( i, updatedArgList );
                items.append( updatedArgList );
            }
            else
            {
                items.append( m_argumentLists->itemText( i ) );
            }
        }
        m_argListSelectAction->setItems( items );
        m_argListSelectAction->setCurrentItem( m_argumentLists->currentIndex() );
    }
}

void ConfigView::slotArgListSelected( int index )
{
    if( index >= 0 && m_changingTarget == 0 )
    {
        // Keep combo box and menu in sync
        m_argumentLists->setCurrentIndex( index );
        m_argListSelectAction->setCurrentItem( index );

        // update the current target with the new selection
        updateCurrentTargetDescription( 2, m_argumentLists->itemText( index ) );
    }
}

void ConfigView::slotAddArgList()
{
    m_argumentLists->insertItem( 0, "new-arg-list" );
    m_argumentLists->setCurrentIndex( 0 );
}

void ConfigView::slotDeleteArgList()
{
    int currentIndex = m_argumentLists->currentIndex();
    if( currentIndex >= 0 )
    {
        m_argumentLists->removeItem( currentIndex );
    }
}

void ConfigView::resizeEvent( QResizeEvent * )
{
    // check if the widgets fit in a VBox layout
    if ( m_useBottomLayout && ( size().height() > m_widgetHeights ) )
    {
        delete layout();
        QGridLayout* layout = new QGridLayout( this );
        layout->addWidget( m_targetLabel, 0, 0, Qt::AlignLeft );
        layout->addWidget( m_targets, 1, 0 );
        layout->addWidget( m_addTarget, 1, 1 );
        layout->addWidget( m_deleteTarget, 1, 2 );
        layout->addWidget( m_workDirLabel, 2, 0, Qt::AlignLeft );
        layout->addWidget( m_workingDirectory, 3, 0 );
        layout->addWidget( m_argumentsLabel, 4, 0, Qt::AlignLeft );
        layout->addWidget( m_argumentLists, 5, 0 );
        layout->addWidget( m_addArgList, 5, 1 );
        layout->addWidget( m_deleteArgList, 5, 2 );
        layout->addWidget( m_takeFocus, 6, 0 );
        layout->addWidget( m_redirectTerminal, 7, 0 );
        layout->addItem( new QSpacerItem( 1, 1 ), 8, 0 );
        layout->setColumnStretch( 0, 1 );
        layout->setRowStretch( 8, 1 );
        m_useBottomLayout = false;
    }
    else if ( !m_useBottomLayout && ( size().height() < m_widgetHeights ) )
    {
        delete layout();
        QGridLayout* layout = new QGridLayout( this );
        layout->addWidget( m_targetLabel, 0, 0, Qt::AlignRight );
        layout->addWidget( m_targets, 0, 1, 1, 2 );
        layout->addWidget( m_addTarget, 0, 3 );
        layout->addWidget( m_deleteTarget, 0, 4 );
        layout->addWidget( m_workDirLabel, 1, 0, Qt::AlignRight );
        layout->addWidget( m_workingDirectory, 1, 1, 1, 2 );
        layout->addWidget( m_argumentsLabel, 2, 0, Qt::AlignRight );
        layout->addWidget( m_argumentLists, 2, 1, 1, 2 );
        layout->addWidget( m_addArgList, 2, 3 );
        layout->addWidget( m_deleteArgList, 2, 4 );
        layout->addWidget( m_takeFocus, 3, 1 );
        layout->addWidget( m_redirectTerminal, 3, 2 );
        layout->addItem( new QSpacerItem( 1, 1 ), 4, 0 );
        layout->setColumnStretch( 1, 1 );
        layout->setColumnStretch( 2, 1 );
        layout->setRowStretch( 4, 1 );
        m_useBottomLayout = true;
    }
}

void ConfigView::updateCurrentTargetDescription( int field, QString const& value )
{
    int         targetIndex = m_targets->currentIndex();
    QStringList targetDescription = m_targets->itemData( targetIndex ).toStringList();
    targetDescription[field] = value;
    m_targets->setItemData( targetIndex, targetDescription );
}
