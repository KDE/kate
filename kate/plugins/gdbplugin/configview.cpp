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

ConfigView::ConfigView( QWidget* parent, Kate::MainWindow* mainWin )
:   QWidget( parent ),
    m_mainWindow( mainWin )
{
    m_targets = new QComboBox();
    m_targets->setEditable( true );
    m_targets->setInsertPolicy( QComboBox::InsertAtTop );

    QCompleter* completer1 = new QCompleter( this );
    completer1->setModel( new QDirModel( completer1 ) );
    m_targets->setCompleter( completer1 );

    m_targetLabel = new QLabel( i18n( "&Target:" ) );
    m_targetLabel->setBuddy( m_targets );

    m_chooseTarget = new QToolButton();
    m_chooseTarget->setIcon( SmallIcon("application-x-ms-dos-executable") );
    m_chooseTarget->setToolTip( i18n("Add executable target") );
    
    m_deleteTarget = new QToolButton();
    m_deleteTarget->setIcon( SmallIcon("edit-delete") );
    m_deleteTarget->setToolTip( i18n("Remove target") );
    
    m_workingDirectories = new QComboBox();
    m_workingDirectories->setEditable( true );
    m_workingDirectories->setInsertPolicy( QComboBox::InsertAtTop );

    QCompleter* completer2 = new QCompleter( this );
    completer2->setModel( new QDirModel( completer2 ) );
    m_workingDirectories->setCompleter( completer2 );

    m_workDirLabel = new QLabel( i18n( "&Working Directory:" ) );
    m_workDirLabel->setBuddy( m_workingDirectories );
    
    m_chooseWorkingDirectory = new QToolButton();
    m_chooseWorkingDirectory->setIcon( SmallIcon("inode-directory") );
    m_chooseWorkingDirectory->setToolTip( i18n("Add Working Directory") );
    
    m_deleteWorkingDirectory = new QToolButton();
    m_deleteWorkingDirectory->setIcon( SmallIcon("edit-delete") );
    m_deleteWorkingDirectory->setToolTip( i18n("Remove Working Directory") );

    m_argumentLists = new QComboBox();
    m_argumentLists->setEditable( true );
    m_argumentLists->setInsertPolicy( QComboBox::InsertAtTop );
    
    m_argumentsLabel = new QLabel( i18nc( "Program argument list", "&Arg List:" ) );
    m_argumentsLabel->setBuddy( m_argumentLists );

    m_takeFocus = new QCheckBox( i18nc( "Checkbox to for keeping focus on the command line",
                                        "Keep focus") );
    m_takeFocus->setToolTip( i18n("Keep the focus on the command line") );

    m_redirectTerminal = new QCheckBox( i18n("Redirect IO") );
    m_redirectTerminal->setToolTip( i18n("Redirect the debugged programs IO to a separate tab") );
    
    

    QGridLayout* layout = new QGridLayout( this );
    layout->addWidget( m_targetLabel, 0, 0, Qt::AlignRight );
    layout->addWidget( m_targets, 0, 1, 1, 2 );
    layout->addWidget( m_chooseTarget, 0, 3 );
    layout->addWidget( m_deleteTarget, 0, 4 );
    layout->addWidget( m_workDirLabel, 1, 0, Qt::AlignRight );
    layout->addWidget( m_workingDirectories, 1, 1, 1, 2 );
    layout->addWidget( m_chooseWorkingDirectory, 1, 3 );
    layout->addWidget( m_deleteWorkingDirectory, 1, 4 );
    layout->addWidget( m_argumentsLabel, 2, 0, Qt::AlignRight );
    layout->addWidget( m_argumentLists, 2, 1, 1, 2 );
    layout->addWidget( m_takeFocus, 3, 1 );
    layout->addWidget( m_redirectTerminal, 3, 2 );
    layout->addItem( new QSpacerItem( 1, 1 ), 4, 0 );
    layout->setColumnStretch( 1, 1 );
    layout->setColumnStretch( 2, 1 );
    layout->setRowStretch( 4, 1 );
    m_useBottomLayout = true;

    // calculate the approximate height to exceed before going to "Side Layout"
    m_widgetHeights = ( m_targetLabel->sizeHint().height() + /*layout spacing */6 ) * 9 ;


    connect(    m_targets, SIGNAL( currentIndexChanged( int ) ),
                this, SLOT( slotTargetChanged( int ) ) );
    connect(    m_chooseTarget, SIGNAL( clicked() ),
                this, SLOT( slotChooseTarget() ) );
    connect(    m_deleteTarget, SIGNAL( clicked() ),
                this, SLOT( slotDeleteTarget() ) );
    connect(    m_chooseWorkingDirectory, SIGNAL( clicked() ),
                this, SLOT( slotChooseWorkingDirectory() ) );
    connect(    m_deleteWorkingDirectory, SIGNAL( clicked() ),
                this, SLOT( slotDeleteWorkingDirectory() ) );
    connect(    m_redirectTerminal, SIGNAL( toggled( bool ) ),
                this, SIGNAL(showIO( bool ) ) );
}

ConfigView::~ConfigView()
{
}

void ConfigView::readConfig( KConfigBase* config, QString const& groupPrefix )
{
    if( config->hasGroup( groupPrefix ) )
    {
        KConfigGroup    group = config->group( groupPrefix );
        int             version = group.readEntry( "version", 1 );

        int     targetCount = group.readEntry( "targetCount", 0 );
        QString targetKey( "target_%1" );

        for( int i = 0; i < targetCount; i++ )
        {
            QStringList targetDescription = group.readEntry(targetKey.arg( i ),
                                                            QStringList() );

            if( version == 1 )
            {
                if( targetDescription.length() == 3 )
                {
                    // valid old style config, translate it now
                    QStringList linkData;
                    linkData << targetDescription[2];
                    linkData << targetDescription[1];
                    m_targets->addItem( targetDescription[2], linkData );

                    if( m_workingDirectories->findText( targetDescription[1] ) == -1 )
                    {
                        m_workingDirectories->addItem( targetDescription[1] );
                    }
                }
            }
            else
            {
                if( targetDescription.length() > 0 )
                {
                    // valid new style config
                    QVariant    linkData( targetDescription );
                    m_targets->addItem( targetDescription[0], linkData );
                }
            }
        }

        if( version > 1 )
        {
            int     wdCount = group.readEntry( "wdCount", 0 );
            QString wdKey( "wd_%1" );

            for( int i = 0; i < wdCount; i++ )
            {
                m_workingDirectories->addItem( group.readEntry( wdKey.arg( i ),
                                                                QString() ) );
            }
        }

        int     argsCount = group.readEntry( "argsCount", 0 );
        QString argsKey( "args_%1" );

        for( int i = 0; i < argsCount; i++ )
        {
            m_argumentLists->addItem( group.readEntry( argsKey.arg( i ),
                                                       QString() ) );
        }

        m_targets->setCurrentIndex( group.readEntry( "lastTarget", 0 ) );
        m_workingDirectories->setCurrentIndex( group.readEntry( "lastWorkDir", 0 ) );
        m_argumentLists->setCurrentIndex( group.readEntry( "lastArgs", 0 ) );

        m_takeFocus->setChecked( group.readEntry( "alwaysFocusOnInput",false ) );

        m_redirectTerminal->setChecked( group.readEntry( "redirectTerminal",false ) );
    }
}

void ConfigView::writeConfig( KConfigBase* config, QString const& groupPrefix )
{
    KConfigGroup    group = config->group( groupPrefix );

    group.writeEntry( "version", 2 );
    
    int     targetCount = m_targets->count();
    QString targetKey( "target_%1" );

    group.writeEntry( "targetCount", targetCount );
    for( int i = 0; i < targetCount; i++ )
    {
        QStringList targetDescription = m_targets->itemData( i ).toStringList();
        group.writeEntry( targetKey.arg( i ), targetDescription );
    }

    group.writeEntry( "lastTarget", m_targets->currentIndex() );

    int     wdCount = m_workingDirectories->count();
    QString wdKey( "wd_%1" );

    group.writeEntry( "wdCount", wdCount );
    for( int i = 0; i < wdCount; i++ )
    {
        group.writeEntry(   wdKey.arg( i ),
                            m_workingDirectories->itemText( i ) );
    }

    group.writeEntry( "lastWorkDir", m_workingDirectories->currentIndex() );

    int     argsCount = m_argumentLists->count();
    QString argsKey( "args_%1" );

    group.writeEntry( "argsCount", argsCount );
    for( int i = 0; i < argsCount; i++ )
    {
        group.writeEntry(   argsKey.arg( i ),
                            m_argumentLists->itemText( i ) );
    }

    group.writeEntry( "lastArgs", m_argumentLists->currentIndex() );

    group.writeEntry( "alwaysFocusOnInput", m_takeFocus->isChecked() );

    group.writeEntry( "redirectTerminal", m_redirectTerminal->isChecked() );

}

void ConfigView::snapshotSettings()
{
    int         targetIndex = m_targets->findText( currentExecutable() );
    QStringList targetDescription;

    targetDescription << currentExecutable();
    targetDescription << currentWorkingDirectory();
    targetDescription << currentArgs();

    if( targetIndex == -1 )
    {
        // The user has edited the target, so store a new one
        m_targets->insertItem( 0, targetDescription[0], targetDescription );
    }
    else
    {
        // Existing target, just update links
        m_targets->setItemData( targetIndex, targetDescription );
    }

    if( m_workingDirectories->findText( currentWorkingDirectory() ) == -1 )
    {
        // The user has edited the working directory, so store a new one
        m_workingDirectories->insertItem( 0, currentWorkingDirectory() );
    }

    if( m_argumentLists->findText( currentArgs() ) == -1 )
    {
        // The user has edited the argument list, so store a new one
        m_argumentLists->insertItem( 0, currentArgs() );
    }
}

QString ConfigView::currentExecutable() const
{
    return m_targets->currentText();
}

QString ConfigView::currentWorkingDirectory() const
{
    return m_workingDirectories->currentText();
}

QString ConfigView::currentArgs() const
{
    return m_argumentLists->currentText();
}

void ConfigView::slotTargetChanged( int index )
{
    // Select the last working directory and argument list used with this target
    if( index >= 0 )
    {
        QStringList targetDescription = m_targets->itemData( index ).toStringList();
        int         wdIndex = -1;
        int         argIndex = -1;

        if( targetDescription.length() > 1 )
        {
            wdIndex = m_workingDirectories->findText( targetDescription[1] );
            if( wdIndex >= 0 )
            {
                m_workingDirectories->setCurrentIndex( wdIndex );
            }
        }

        if( targetDescription.length() > 2 )
        {
            argIndex = m_argumentLists->findText( targetDescription[2] );
            if( argIndex >= 0 )
            {
                m_argumentLists->setCurrentIndex( argIndex );
            }
        }
    }
}

void ConfigView::slotChooseTarget()
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
        QStringList targetDescription;
        
        targetDescription << target;
        m_targets->insertItem( 0, target, targetDescription );
        m_targets->setCurrentIndex( 0 );
    }
}

void ConfigView::slotChooseWorkingDirectory()
{
    KUrl    defDir( m_workingDirectories->currentText() );

    if( m_workingDirectories->currentText().isEmpty() )
    {
        // try current document dir
        KTextEditor::View*  view = m_mainWindow->activeView();

        if( view != NULL )
        {
            defDir = view->document()->url();
        }
    }

    QString wd = KFileDialog::getExistingDirectory( defDir, NULL, QString() );
    if( !wd.isEmpty() )
    {
        m_workingDirectories->insertItem( 0, wd );
        m_workingDirectories->setCurrentIndex( 0 );
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

void ConfigView::slotDeleteWorkingDirectory()
{
    int currentIndex = m_workingDirectories->currentIndex();
    if( currentIndex >= 0 )
    {
        m_workingDirectories->removeItem( currentIndex );
    }
}

bool ConfigView::takeFocusAlways() const
{
    return m_takeFocus->isChecked();
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
        layout->addWidget( m_chooseTarget, 1, 1 );
        layout->addWidget( m_deleteTarget, 1, 2 );
        layout->addWidget( m_workDirLabel, 2, 0, Qt::AlignLeft );
        layout->addWidget( m_workingDirectories, 3, 0 );
        layout->addWidget( m_chooseWorkingDirectory, 3, 1 );
        layout->addWidget( m_deleteWorkingDirectory, 3, 2 );
        layout->addWidget( m_argumentsLabel, 4, 0, Qt::AlignLeft );
        layout->addWidget( m_argumentLists, 5, 0 );
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
        layout->addWidget( m_chooseTarget, 0, 3 );
        layout->addWidget( m_deleteTarget, 0, 4 );
        layout->addWidget( m_workDirLabel, 1, 0, Qt::AlignRight );
        layout->addWidget( m_workingDirectories, 1, 1, 1, 2 );
        layout->addWidget( m_chooseWorkingDirectory, 1, 3 );
        layout->addWidget( m_deleteWorkingDirectory, 1, 4 );
        layout->addWidget( m_argumentsLabel, 2, 0, Qt::AlignRight );
        layout->addWidget( m_argumentLists, 2, 1, 1, 2 );
        layout->addWidget( m_takeFocus, 3, 1 );
        layout->addWidget( m_redirectTerminal, 3, 2 );
        layout->addItem( new QSpacerItem( 1, 1 ), 4, 0 );
        layout->setColumnStretch( 1, 1 );
        layout->setColumnStretch( 2, 1 );
        layout->setRowStretch( 4, 1 );
        m_useBottomLayout = true;
    }
}

bool ConfigView::showIOTab() const
{
    return m_redirectTerminal->isChecked();
}


