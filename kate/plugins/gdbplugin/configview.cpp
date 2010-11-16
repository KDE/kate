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
    mainWindow( mainWin )
{
    targets = new QComboBox();
    targets->setEditable( true );
    targets->setInsertPolicy( QComboBox::InsertAtTop );

    QCompleter* completer1 = new QCompleter( this );
    completer1->setModel( new QDirModel( completer1 ) );
    targets->setCompleter( completer1 );

    targetLabel = new QLabel( i18n( "&Target:" ) );
    targetLabel->setBuddy( targets );

    chooseTarget = new QToolButton();
    chooseTarget->setIcon( SmallIcon("application-x-ms-dos-executable") );
    chooseTarget->setToolTip( i18n("Add executable target") );
    
    deleteTarget = new QToolButton();
    deleteTarget->setIcon( SmallIcon("edit-delete") );
    deleteTarget->setToolTip( i18n("Remove target") );
    
    workingDirectories = new QComboBox();
    workingDirectories->setEditable( true );
    workingDirectories->setInsertPolicy( QComboBox::InsertAtTop );

    QCompleter* completer2 = new QCompleter( this );
    completer2->setModel( new QDirModel( completer2 ) );
    workingDirectories->setCompleter( completer2 );

    workDirLabel = new QLabel( i18n( "&Working Directory:" ) );
    workDirLabel->setBuddy( workingDirectories );
    
    chooseWorkingDirectory = new QToolButton();
    chooseWorkingDirectory->setIcon( SmallIcon("inode-directory") );
    chooseWorkingDirectory->setToolTip( i18n("Add Working Directory") );
    
    deleteWorkingDirectory = new QToolButton();
    deleteWorkingDirectory->setIcon( SmallIcon("edit-delete") );
    deleteWorkingDirectory->setToolTip( i18n("Remove Working Directory") );

    argumentLists = new QComboBox();
    argumentLists->setEditable( true );
    argumentLists->setInsertPolicy( QComboBox::InsertAtTop );
    
    argumentsLabel = new QLabel( i18nc( "Program argument list", "&Arg List:" ) );
    argumentsLabel->setBuddy( argumentLists );

    takeFocus = new QCheckBox( i18n("Keep the focus on the command line") );
    redirectTerminal = new QCheckBox( i18n("Redirect output") );

    QGridLayout* layout = new QGridLayout( this );
    layout->addWidget( targetLabel, 0, 0, Qt::AlignRight );
    layout->addWidget( targets, 0, 1, 1, 2 );
    layout->addWidget( chooseTarget, 0, 3 );
    layout->addWidget( deleteTarget, 0, 4 );
    layout->addWidget( workDirLabel, 1, 0, Qt::AlignRight );
    layout->addWidget( workingDirectories, 1, 1, 1, 2 );
    layout->addWidget( chooseWorkingDirectory, 1, 3 );
    layout->addWidget( deleteWorkingDirectory, 1, 4 );
    layout->addWidget( argumentsLabel, 2, 0, Qt::AlignRight );
    layout->addWidget( argumentLists, 2, 1, 1, 2 );
    layout->addWidget( takeFocus, 3, 1 );
    layout->addWidget( redirectTerminal, 3, 2 );
    layout->addItem( new QSpacerItem( 1, 1 ), 4, 0 );
    layout->setColumnStretch( 1, 1 );
    layout->setColumnStretch( 2, 1 );
    layout->setRowStretch( 4, 1 );
    useBottomLayout = true;

    // calculate the approximate height to exceed before going to "Side Layout"
    widgetHeights = (targetLabel->sizeHint().height() + /*layout spacing */6) * 9 ;


    connect(    targets, SIGNAL( currentIndexChanged( int ) ),
                this, SLOT( slotTargetChanged( int ) ) );
    connect(    chooseTarget, SIGNAL( clicked() ),
                this, SLOT( slotChooseTarget() ) );
    connect(    deleteTarget, SIGNAL( clicked() ),
                this, SLOT( slotDeleteTarget() ) );
    connect(    chooseWorkingDirectory, SIGNAL( clicked() ),
                this, SLOT( slotChooseWorkingDirectory() ) );
    connect(    deleteWorkingDirectory, SIGNAL( clicked() ),
                this, SLOT( slotDeleteWorkingDirectory() ) );
    connect(    redirectTerminal, SIGNAL( toggled( bool ) ),
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
                    targets->addItem( targetDescription[2], linkData );

                    if( workingDirectories->findText( targetDescription[1] ) == -1 )
                    {
                        workingDirectories->addItem( targetDescription[1] );
                    }
                }
            }
            else
            {
                if( targetDescription.length() > 0 )
                {
                    // valid new style config
                    QVariant    linkData( targetDescription );
                    targets->addItem( targetDescription[0], linkData );
                }
            }
        }

        if( version > 1 )
        {
            int     wdCount = group.readEntry( "wdCount", 0 );
            QString wdKey( "wd_%1" );

            for( int i = 0; i < wdCount; i++ )
            {
                workingDirectories->addItem(    group.readEntry( wdKey.arg( i ),
                                                                 QString() ) );
            }
        }

        int     argsCount = group.readEntry( "argsCount", 0 );
        QString argsKey( "args_%1" );

        for( int i = 0; i < argsCount; i++ )
        {
            argumentLists->addItem( group.readEntry(    argsKey.arg( i ),
                                                        QString() ) );
        }

        targets->setCurrentIndex( group.readEntry( "lastTarget", 0 ) );
        workingDirectories->setCurrentIndex( group.readEntry( "lastWorkDir", 0 ) );
        argumentLists->setCurrentIndex( group.readEntry( "lastArgs", 0 ) );

        takeFocus->setChecked( group.readEntry( "alwaysFocusOnInput",false ) );

        redirectTerminal->setChecked( group.readEntry( "redirectTerminal",false ) );
    }
}

void ConfigView::writeConfig( KConfigBase* config, QString const& groupPrefix )
{
    KConfigGroup    group = config->group( groupPrefix );

    group.writeEntry( "version", 2 );
    
    int     targetCount = targets->count();
    QString targetKey( "target_%1" );

    group.writeEntry( "targetCount", targetCount );
    for( int i = 0; i < targetCount; i++ )
    {
        QStringList targetDescription = targets->itemData( i ).toStringList();
        group.writeEntry( targetKey.arg( i ), targetDescription );
    }

    group.writeEntry( "lastTarget", targets->currentIndex() );

    int     wdCount = workingDirectories->count();
    QString wdKey( "wd_%1" );

    group.writeEntry( "wdCount", wdCount );
    for( int i = 0; i < wdCount; i++ )
    {
        group.writeEntry(   wdKey.arg( i ),
                            workingDirectories->itemText( i ) );
    }

    group.writeEntry( "lastWorkDir", workingDirectories->currentIndex() );

    int     argsCount = argumentLists->count();
    QString argsKey( "args_%1" );

    group.writeEntry( "argsCount", argsCount );
    for( int i = 0; i < argsCount; i++ )
    {
        group.writeEntry(   argsKey.arg( i ),
                            argumentLists->itemText( i ) );
    }

    group.writeEntry( "lastArgs", argumentLists->currentIndex() );

    group.writeEntry( "alwaysFocusOnInput", takeFocus->isChecked() );

    group.writeEntry( "redirectTerminal", redirectTerminal->isChecked() );

}

void ConfigView::snapshotSettings()
{
    int         targetIndex = targets->findText( currentExecutable() );
    QStringList targetDescription;

    targetDescription << currentExecutable();
    targetDescription << currentWorkingDirectory();
    targetDescription << currentArgs();

    if( targetIndex == -1 )
    {
        // The user has edited the target, so store a new one
        targets->insertItem( 0, targetDescription[0], targetDescription );
    }
    else
    {
        // Existing target, just update links
        targets->setItemData( targetIndex, targetDescription );
    }

    if( workingDirectories->findText( currentWorkingDirectory() ) == -1 )
    {
        // The user has edited the working directory, so store a new one
        workingDirectories->insertItem( 0, currentWorkingDirectory() );
    }

    if( argumentLists->findText( currentArgs() ) == -1 )
    {
        // The user has edited the argument list, so store a new one
        argumentLists->insertItem( 0, currentArgs() );
    }
}

QString ConfigView::currentExecutable() const
{
    return targets->currentText();
}

QString ConfigView::currentWorkingDirectory() const
{
    return workingDirectories->currentText();
}

QString ConfigView::currentArgs() const
{
    return argumentLists->currentText();
}

void ConfigView::slotTargetChanged( int index )
{
    // Select the last working directory and argument list used with this target
    if( index >= 0 )
    {
        QStringList targetDescription = targets->itemData( index ).toStringList();
        int         wdIndex = -1;
        int         argIndex = -1;

        if( targetDescription.length() > 1 )
        {
            wdIndex = workingDirectories->findText( targetDescription[1] );
            if( wdIndex >= 0 )
            {
                workingDirectories->setCurrentIndex( wdIndex );
            }
        }

        if( targetDescription.length() > 2 )
        {
            argIndex = argumentLists->findText( targetDescription[2] );
            if( argIndex >= 0 )
            {
                argumentLists->setCurrentIndex( argIndex );
            }
        }
    }
}

void ConfigView::slotChooseTarget()
{
    KUrl    defDir( targets->currentText() );

    if( targets->currentText().isEmpty() )
    {
        // try current document dir
        KTextEditor::View*  view = mainWindow->activeView();

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
        targets->insertItem( 0, target, targetDescription );
        targets->setCurrentIndex( 0 );
    }
}

void ConfigView::slotChooseWorkingDirectory()
{
    KUrl    defDir( workingDirectories->currentText() );

    if( workingDirectories->currentText().isEmpty() )
    {
        // try current document dir
        KTextEditor::View*  view = mainWindow->activeView();

        if( view != NULL )
        {
            defDir = view->document()->url();
        }
    }

    QString wd = KFileDialog::getExistingDirectory( defDir, NULL, QString() );
    if( !wd.isEmpty() )
    {
        workingDirectories->insertItem( 0, wd );
        workingDirectories->setCurrentIndex( 0 );
    }
}

void ConfigView::slotDeleteTarget()
{
    int currentIndex = targets->currentIndex();
    if( currentIndex >= 0 )
    {
        targets->removeItem( currentIndex );
    }
}

void ConfigView::slotDeleteWorkingDirectory()
{
    int currentIndex = workingDirectories->currentIndex();
    if( currentIndex >= 0 )
    {
        workingDirectories->removeItem( currentIndex );
    }
}

bool ConfigView::takeFocusAlways() const
{
    return takeFocus->isChecked();
}


void ConfigView::resizeEvent( QResizeEvent * )
{
    // check if the widgets fit in a VBox layout
    if ( useBottomLayout && ( size().height() > widgetHeights ) )
    {
        delete layout();
        QGridLayout* layout = new QGridLayout( this );
        layout->addWidget( targetLabel, 0, 0, Qt::AlignLeft );
        layout->addWidget( targets, 1, 0 );
        layout->addWidget( chooseTarget, 1, 1 );
        layout->addWidget( deleteTarget, 1, 2 );
        layout->addWidget( workDirLabel, 2, 0, Qt::AlignLeft );
        layout->addWidget( workingDirectories, 3, 0 );
        layout->addWidget( chooseWorkingDirectory, 3, 1 );
        layout->addWidget( deleteWorkingDirectory, 3, 2 );
        layout->addWidget( argumentsLabel, 4, 0, Qt::AlignLeft );
        layout->addWidget( argumentLists, 5, 0 );
        layout->addWidget( takeFocus, 6, 0 );
        layout->addWidget( redirectTerminal, 7, 0 );
        layout->addItem( new QSpacerItem( 1, 1 ), 8, 0 );
        layout->setColumnStretch( 0, 1 );
        layout->setRowStretch( 8, 1 );
        useBottomLayout = false;
    }
    else if ( !useBottomLayout && ( size().height() < widgetHeights ) )
    {
        delete layout();
        QGridLayout* layout = new QGridLayout( this );
        layout->addWidget( targetLabel, 0, 0, Qt::AlignRight );
        layout->addWidget( targets, 0, 1, 1, 2 );
        layout->addWidget( chooseTarget, 0, 3 );
        layout->addWidget( deleteTarget, 0, 4 );
        layout->addWidget( workDirLabel, 1, 0, Qt::AlignRight );
        layout->addWidget( workingDirectories, 1, 1, 1, 2 );
        layout->addWidget( chooseWorkingDirectory, 1, 3 );
        layout->addWidget( deleteWorkingDirectory, 1, 4 );
        layout->addWidget( argumentsLabel, 2, 0, Qt::AlignRight );
        layout->addWidget( argumentLists, 2, 1, 1, 2 );
        layout->addWidget( takeFocus, 3, 1 );
        layout->addWidget( redirectTerminal, 3, 2 );
        layout->addItem( new QSpacerItem( 1, 1 ), 4, 0 );
        layout->setColumnStretch( 1, 1 );
        layout->setColumnStretch( 2, 1 );
        layout->setRowStretch( 4, 1 );
        useBottomLayout = true;
    }
}

bool ConfigView::showIOTab() const
{
    return redirectTerminal->isChecked();
}


