//
// Description: Kate Plugin for GDB integration
//
//
// Copyright (c) 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// Copyright (c) 2010 Kåre Särs <kare.sars@iki.fi>
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

#include "plugin_kategdb.h"
#include "plugin_kategdb.moc"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QLayout>
#include <QtGui/QTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QTreeWidget>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kcolorscheme.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kmenu.h>
#include <khistorycombobox.h>

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/markinterface.h>

K_PLUGIN_FACTORY(KatePluginGDBFactory, registerPlugin<KatePluginGDB>();)
K_EXPORT_PLUGIN(KatePluginGDBFactory(
                    KAboutData( "kategdb",
                                "kategdb",
                                ki18n( "GDB Integration" ),
                                "0.1",
                                ki18n( "Kate GDB Integration" ) ) ) )

KatePluginGDB::KatePluginGDB( QObject* parent, const VariantList& )
:   Kate::Plugin( (Kate::Application*)parent, "kate-gdb-plugin" )
{
}

KatePluginGDB::~KatePluginGDB()
{
}

Kate::PluginView* KatePluginGDB::createView( Kate::MainWindow* mainWindow )
{
    return new KatePluginGDBView( mainWindow, application() );
}

KatePluginGDBView::KatePluginGDBView( Kate::MainWindow* mainWin, Kate::Application* application )
:   Kate::PluginView( mainWin ), Kate::XMLGUIClient(KatePluginGDBFactory::componentData())
{
    lastExecUrl = "";
    lastExecLine = -1;
    lastExecFrame = 0;
    kateApplication = application;
    focusOnInput = true;


    toolView = mainWindow()->createToolView(  i18n("Debug View"),
                                                Kate::MainWindow::Bottom,
                                                SmallIcon("debug"),
                                                i18n("Debug View") );
    tabWidget = new QTabWidget( toolView );
    // Output
    outputArea = new QTextEdit();
    outputArea->setAcceptRichText( false  );
    outputArea->setReadOnly( true );
    outputArea->setUndoRedoEnabled( false );
    // fixed wide font, like konsole
    outputArea->setFont( KGlobalSettings::fixedFont() );
    // alternate color scheme, like konsole
    KColorScheme schemeView( QPalette::Active, KColorScheme::View );
    outputArea->setTextBackgroundColor( schemeView.foreground().color() );
    outputArea->setTextColor( schemeView.background().color() );
    QPalette p = outputArea->palette ();
    p.setColor( QPalette::Base, schemeView.foreground().color() );
    outputArea->setPalette( p );

    // input
    inputArea = new KHistoryComboBox( true );
    connect( inputArea,  SIGNAL( returnPressed() ), this, SLOT( slotSendCommand() ) );
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget( inputArea, 10 );
    inputLayout->setContentsMargins( 0,0,0,0 );
    outputArea->setFocusProxy( inputArea ); // take the focus from the outputArea

    QWidget *gdbPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout( gdbPage );
    layout->addWidget( outputArea );
    layout->addLayout( inputLayout );
    layout->setStretch(0, 10);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    // stack page
    stackTree = new QTreeWidget;
    QStringList headers;
    headers << "  " << i18n( "Nr" ) << i18n( "Frame" );
    stackTree->setHeaderLabels(headers);
    stackTree->setRootIsDecorated(false);
    stackTree->resizeColumnToContents(0);
    stackTree->resizeColumnToContents(1);
    connect( stackTree, SIGNAL( itemActivated ( QTreeWidgetItem *, int ) ),
             this, SLOT( stackFrameSelected() ) );

    // config page
    configView = new ConfigView( NULL, mainWin );

    tabWidget->addTab( gdbPage, i18n( "GDB Output" ) );
    tabWidget->addTab( stackTree, i18n( "Call Stack" ) );
    tabWidget->addTab( configView, i18n( "Settings" ) );

    debugView  = new DebugView( this );
    connect( debugView,  SIGNAL( readyForInput( bool ) ), 
             this,       SLOT( enableDebugActions( bool ) ) );

    connect( debugView,  SIGNAL( outputText( const QString ) ), 
             outputArea, SLOT( append( const QString ) ) );

    connect( debugView,  SIGNAL( debugLocationChanged( const char*, int ) ),
             this,       SLOT( slotGoTo( const char*, int ) ) );

    connect( debugView,  SIGNAL( breakPointSet( KUrl const&, int ) ),
             this,       SLOT( slotBreakpointSet( KUrl const&, int ) ) );

    connect( debugView,  SIGNAL( breakPointCleared( KUrl const&, int ) ),
             this,       SLOT( slotBreakpointCleared( KUrl const&, int ) ) );

    connect( debugView,  SIGNAL( programEnded() ),
             this,       SLOT( programEnded() ) );

    connect( debugView,  SIGNAL( gdbEnded() ),
             this,       SLOT( programEnded() ) );

    connect( debugView,  SIGNAL( gdbEnded() ),
             this,       SLOT( gdbEnded() ) );

    connect( debugView,  SIGNAL( stackFrameInfo( QString, QString ) ),
             this,       SLOT( insertStackFrame( QString, QString ) ) );

    connect( debugView,  SIGNAL( stackFrameChanged( int ) ),
             this,       SLOT( stackFrameChanged( int ) ) );

    KAction* a = actionCollection()->addAction( "debug" );
    a->setText( i18n( "Start Debugging" ) );
    a->setIcon( KIcon( "debug" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this,   SLOT( slotDebug() ) );

    a = actionCollection()->addAction( "kill" );
    a->setText( i18n( "Kill / Stop Debugging" ) );
    a->setIcon( KIcon( "media-playback-stop" ) );
    connect(    a,         SIGNAL( triggered( bool ) ),
                debugView, SLOT( slotKill() ) );

    a = actionCollection()->addAction( "rerun" );
    a->setText( i18n( "Restart Debugging" ) );
    a->setIcon( KIcon( "view-refresh" ) );
    connect(    a,         SIGNAL( triggered( bool ) ),
                debugView, SLOT( slotReRun() ) );

    a = actionCollection()->addAction( "toggle_breakpoint" );
    a->setText( i18n( "Toggle Breakpoint / Break" ) );
    a->setIcon( KIcon( "media-playback-pause" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this,   SLOT( slotToggleBreakpoint() ) );

    a = actionCollection()->addAction( "step_in" );
    a->setText( i18n( "Step In" ) );
    a->setIcon( KIcon( "debug-step-into" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                debugView,   SLOT( slotStepInto() ) );

    a = actionCollection()->addAction( "step_over" );
    a->setText( i18n( "Step Over" ) );
    a->setIcon( KIcon( "debug-step-over" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                debugView,   SLOT( slotStepOver() ) );

    a = actionCollection()->addAction( "step_out" );
    a->setText( i18n( "Step Out" ) );
    a->setIcon( KIcon( "debug-step-out" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                debugView, SLOT( slotStepOut() ) );

    a = actionCollection()->addAction( "move_pc" );
    a->setText( i18n( "Move PC" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this,   SLOT( slotMovePC() ) );

    a = actionCollection()->addAction( "run_to_cursor" );
    a->setText( i18n( "Run To Cursor" ) );
    a->setIcon( KIcon( "debug-run-cursor" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this,   SLOT( slotRunToCursor() ) );

    a = actionCollection()->addAction( "continue" );
    a->setText( i18n( "Continue" ) );
    a->setIcon( KIcon( "media-playback-start" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                debugView, SLOT( slotContinue() ) );

    a = actionCollection()->addAction( "print_value" );
    a->setText( i18n( "Print Value" ) );
    a->setIcon( KIcon( "document-preview" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this, SLOT( slotValue() ) );

    // popup context menu
    menu = new KActionMenu( i18n("Debug"), this );
    actionCollection()->addAction( "popup_gdb", menu );
    connect( menu->menu(), SIGNAL( aboutToShow() ), this, SLOT( aboutToShowMenu() ) );
    
    breakpoint = menu->menu()->addAction(i18n("popup_breakpoint"), 
                                         this, SLOT( slotToggleBreakpoint() ) );

    QAction* popupAction = menu->menu()->addAction(i18n( "popup_run_to_cursor" ), 
                                                   this, SLOT( slotRunToCursor() ) );
    popupAction->setText( i18n( "Run To Cursor" ) );
    popupAction = menu->menu()->addAction( "move_pc", 
                                          this, SLOT( slotMovePC() ) );
    popupAction->setText( i18n( "Move PC" ) );
    
    enableDebugActions( false );
    
    mainWindow()->guiFactory()->addClient( this );
}

KatePluginGDBView::~KatePluginGDBView()
{
    mainWindow()->guiFactory()->removeClient( this );
}

void KatePluginGDBView::readSessionConfig(  KConfigBase*    config,
                                            const QString&  groupPrefix )
{
    configView->readConfig( config, groupPrefix );
}

void KatePluginGDBView::writeSessionConfig( KConfigBase*    config,
                                            const QString&  groupPrefix )
{
    configView->writeConfig( config, groupPrefix );
}

void KatePluginGDBView::slotDebug()
{
    configView->snapshotSettings();
    debugView->runDebugger( configView->currentWorkingDirectory(),
                              configView->currentExecutable(),
                              configView->currentArgs() );
    enableDebugActions( true );
    mainWindow()->showToolView( toolView );
    tabWidget->setCurrentWidget( outputArea );
}

void KatePluginGDBView::aboutToShowMenu()
{
    if (!debugView->debuggerRunning() || debugView->debuggerBusy())
    {
        breakpoint->setText(i18n("Insert breakpoint"));
        breakpoint->setDisabled(true);
        return;
    }

    breakpoint->setDisabled(false);

    KTextEditor::View* editView = mainWindow()->activeView();
    KUrl               url = editView->document()->url();
    int                line = editView->cursorPosition().line();

    line++; // GDB uses 1 based line numbers, kate uses 0 based...

    if (debugView->hasBreakpoint(url, line))
    {
        breakpoint->setText(i18n("Remove breakpoint"));
    }
    else
    {
        breakpoint->setText(i18n("Insert breakpoint"));
    }
}

void KatePluginGDBView::slotToggleBreakpoint()
{
    if ( !actionCollection()->action( "continue" )->isEnabled() ) 
    {
        debugView->slotInterrupt();
    }
    else 
    {
        KTextEditor::View* editView = mainWindow()->activeView();
        KUrl               currURL  = editView->document()->url();
        int                line   = editView->cursorPosition().line();

        debugView->toggleBreakpoint( currURL, line + 1 );
    }
}

void KatePluginGDBView::slotBreakpointSet( KUrl const& file, int line)
{
    KTextEditor::MarkInterface* iface =
    qobject_cast<KTextEditor::MarkInterface*>( kateApplication->documentManager()->findUrl( file ) );

    if (iface) 
    {
        iface->setMarkDescription(KTextEditor::MarkInterface::BreakpointActive, i18n("Breakpoint"));
        iface->setMarkPixmap(KTextEditor::MarkInterface::BreakpointActive, 
                             KIcon("media-playback-pause").pixmap(10,10));
        iface->addMark(line, KTextEditor::MarkInterface::BreakpointActive );
    }
}

void KatePluginGDBView::slotBreakpointCleared( KUrl const& file, int line)
{
    KTextEditor::MarkInterface* iface =
    qobject_cast<KTextEditor::MarkInterface*>( kateApplication->documentManager()->findUrl( file ) );

    if (iface) 
    {
        iface->removeMark( line, KTextEditor::MarkInterface::BreakpointActive );
    }
}

void KatePluginGDBView::slotMovePC()
{
    KTextEditor::View*      editView = mainWindow()->activeView();
    KUrl                    currURL = editView->document()->url();
    KTextEditor::Cursor     cursor = editView->cursorPosition();

    debugView->movePC( currURL, cursor.line() );
}

void KatePluginGDBView::slotRunToCursor()
{
    KTextEditor::View*      editView = mainWindow()->activeView();
    KUrl                    currURL = editView->document()->url();
    KTextEditor::Cursor     cursor = editView->cursorPosition();

    debugView->runToCursor( currURL, cursor.line() );
}

void KatePluginGDBView::slotGoTo( const char* fileName, int lineNum )
{

    KUrl url = debugView->resolveFileName( fileName );

    // skip not existing files
    if (!QFile::exists (url.toLocalFile ())) 
    {
        lastExecLine = -1;
        return;
    }

    lastExecUrl = url;
    lastExecLine = lineNum;

    KTextEditor::View*  editView = mainWindow()->openUrl( lastExecUrl );
    editView->setCursorPosition( KTextEditor::Cursor( lastExecLine, 0 ) );
    mainWindow()->window()->raise();
    mainWindow()->window()->setFocus();
}

void KatePluginGDBView::enableDebugActions( bool enable )
{
    actionCollection()->action( "step_in"       )->setEnabled( enable );
    actionCollection()->action( "step_over"     )->setEnabled( enable );
    actionCollection()->action( "step_out"      )->setEnabled( enable );
    actionCollection()->action( "move_pc"       )->setEnabled( enable );
    actionCollection()->action( "run_to_cursor" )->setEnabled( enable );
    actionCollection()->action( "popup_gdb"     )->setEnabled( enable );
    actionCollection()->action( "continue"      )->setEnabled( enable );
    actionCollection()->action( "print_value"   )->setEnabled( enable );

    // "toggle breakpoint" doubles as interrupt while the program is running
    actionCollection()->action( "toggle_breakpoint" )->setEnabled( debugView->debuggerRunning() );
    actionCollection()->action( "kill"              )->setEnabled( debugView->debuggerRunning() );
    actionCollection()->action( "rerun"             )->setEnabled( debugView->debuggerRunning() );

    inputArea->setEnabled( enable );
    if ( enable )
    {
        inputArea->setFocusPolicy( Qt::WheelFocus );
        if ( focusOnInput || configView->takeFocusAlways() ) 
        {
            inputArea->setFocus();
            focusOnInput = false;
        }
        else
        {
            mainWindow()->activeView()->setFocus();
        }
    }
    else
    {
        inputArea->setFocusPolicy( Qt::NoFocus );
        if ( mainWindow()->activeView() ) mainWindow()->activeView()->setFocus();
    }

    if ( (lastExecLine > -1) )
    {
        KTextEditor::MarkInterface* iface =
        qobject_cast<KTextEditor::MarkInterface*>( kateApplication->documentManager()->findUrl( lastExecUrl ) );

        if (iface) 
        {
            if ( enable )
            {
                iface->setMarkDescription(KTextEditor::MarkInterface::Execution, i18n("Execution point"));
                iface->setMarkPixmap(KTextEditor::MarkInterface::Execution, KIcon("arrow-right").pixmap(10,10));
                iface->addMark(lastExecLine, KTextEditor::MarkInterface::Execution);
            }
            else 
            {
                iface->removeMark( lastExecLine, KTextEditor::MarkInterface::Execution );
            }
        }
    }
}

void KatePluginGDBView::programEnded()
{
    // don't set the execution mark on exit
    lastExecLine = -1;
    stackTree->clear();

    // Indicate the state change by showing the debug outputArea
    mainWindow()->showToolView( toolView );
    tabWidget->setCurrentWidget( outputArea );
}

void KatePluginGDBView::gdbEnded()
{
    outputArea->clear();
}

void KatePluginGDBView::slotSendCommand()
{
    QString cmd = inputArea->currentText();

    if ( cmd.isEmpty() ) cmd = lastCommand;

    inputArea->addToHistory( cmd );
    inputArea->setCurrentItem("");
    focusOnInput = true;
    lastCommand = cmd;
    debugView->issueCommand( cmd );

    QScrollBar *sb = outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());

}

void KatePluginGDBView::insertStackFrame( QString const& level, QString const& info )
{
    if ( level == "0") 
    {
        stackTree->clear();
    }
    QStringList columns;
    columns << "  "; // icon place holder
    columns << level;
    columns << info;

    stackTree->insertTopLevelItem(level.toInt(), new QTreeWidgetItem(stackTree, columns));
}

void KatePluginGDBView::stackFrameSelected()
{
    debugView->issueCommand( QString( "(Q)f %1" ).arg( stackTree->currentIndex().row() ) );
}

void KatePluginGDBView::stackFrameChanged( int level )
{
    QTreeWidgetItem *current = stackTree->topLevelItem(lastExecFrame);
    QTreeWidgetItem *next = stackTree->topLevelItem(level);

    if ( current ) current->setIcon ( 0, QIcon() );
    if ( next )    next->setIcon( 0, KIcon("arrow-right") );
    lastExecFrame = level;
}

QString KatePluginGDBView::currentWord( )
{
    KTextEditor::View *kv = mainWindow()->activeView();
    if (!kv) {
        kDebug() << "no KTextEditor::View" << endl;
        return QString();
    }

    if (!kv->cursorPosition().isValid()) {
        kDebug() << "cursor not valid!" << endl;
        return QString();
    }

    int line = kv->cursorPosition().line();
    int col = kv->cursorPosition().column();

    QString linestr = kv->document()->line(line);

    int startPos = qMax(qMin(col, linestr.length()-1), 0);
    int endPos = startPos;
    while (startPos >= 0 && (linestr[startPos].isLetterOrNumber() || linestr[startPos] == '_' || linestr[startPos] == '~')) {
        startPos--;
    }
    while (endPos < (int)linestr.length() && (linestr[endPos].isLetterOrNumber() || linestr[endPos] == '_')) {
        endPos++;
    }
    if  (startPos == endPos) {
        kDebug() << "no word found!" << endl;
        return QString();
    }

    //kDebug() << linestr.mid(startPos+1, endPos-startPos-1);
    return linestr.mid(startPos+1, endPos-startPos-1);
}

void KatePluginGDBView::slotValue()
{
    QString variable;
    KTextEditor::View* editView = mainWindow()->activeView();
    if ( editView && editView->selection() ) variable = editView->selectionText();
    
    if ( variable.isEmpty() ) variable = currentWord();
    
    if ( variable.isEmpty() ) return;
    
    QString cmd = QString( "print %1" ).arg( variable );
    debugView->issueCommand( cmd );
    inputArea->addToHistory( cmd );
    inputArea->setCurrentItem("");

    mainWindow()->showToolView( toolView );
    tabWidget->setCurrentWidget( outputArea );

    QScrollBar *sb = outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());

}
