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
                                "kategdbplugin",
                                ki18n( "GDB Integration" ),
                                "0.1",
                                ki18n( "Kate GDB Integration" ) ) ) )

KatePluginGDB::KatePluginGDB( QObject* parent, const VariantList& )
:   Kate::Plugin( (Kate::Application*)parent, "kate-gdb-plugin" )
{
    KGlobal::locale()->insertCatalog("kategdbplugin");
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
    m_lastExecUrl = "";
    m_lastExecLine = -1;
    m_lastExecFrame = 0;
    m_kateApplication = application;
    m_focusOnInput = true;


    m_toolView = mainWindow()->createToolView(i18n("Debug View"),
                                              Kate::MainWindow::Bottom,
                                              SmallIcon("debug"),
                                              i18n("Debug View"));

    m_localsToolView = mainWindow()->createToolView(i18n("Locals"),
                                                Kate::MainWindow::Right,
                                                SmallIcon("debug"),
                                                i18n("Locals"));

    m_tabWidget = new QTabWidget( m_toolView );
    // Output
    m_outputArea = new QTextEdit();
    m_outputArea->setAcceptRichText( false  );
    m_outputArea->setReadOnly( true );
    m_outputArea->setUndoRedoEnabled( false );
    // fixed wide font, like konsole
    m_outputArea->setFont( KGlobalSettings::fixedFont() );
    // alternate color scheme, like konsole
    KColorScheme schemeView( QPalette::Active, KColorScheme::View );
    m_outputArea->setTextBackgroundColor( schemeView.foreground().color() );
    m_outputArea->setTextColor( schemeView.background().color() );
    QPalette p = m_outputArea->palette ();
    p.setColor( QPalette::Base, schemeView.foreground().color() );
    m_outputArea->setPalette( p );

    // input
    m_inputArea = new KHistoryComboBox( true );
    connect( m_inputArea,  SIGNAL( returnPressed() ), this, SLOT( slotSendCommand() ) );
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget( m_inputArea, 10 );
    inputLayout->setContentsMargins( 0,0,0,0 );
    m_outputArea->setFocusProxy( m_inputArea ); // take the focus from the outputArea

    m_gdbPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout( m_gdbPage );
    layout->addWidget( m_outputArea );
    layout->addLayout( inputLayout );
    layout->setStretch(0, 10);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    // stack page
    m_stackTree = new QTreeWidget(m_tabWidget);
    QStringList headers;
    headers << "  " << i18nc( "Column label (frame number)", "Nr" ) << i18nc( "Column label", "Frame" );
    m_stackTree->setHeaderLabels(headers);
    m_stackTree->setRootIsDecorated(false);
    m_stackTree->resizeColumnToContents(0);
    m_stackTree->resizeColumnToContents(1);
    m_stackTree->setAutoScroll(false);
    connect( m_stackTree, SIGNAL( itemActivated ( QTreeWidgetItem *, int ) ),
             this, SLOT( stackFrameSelected() ) );

    // config page
    m_configView = new ConfigView( NULL, mainWin );

    m_ioView = new IOView();
    connect( m_configView, SIGNAL( showIO( bool ) ),
             this,       SLOT( showIO( bool ) ) );

    m_tabWidget->addTab( m_gdbPage, i18nc( "Tab label", "GDB Output" ) );
    m_tabWidget->addTab( m_stackTree, i18nc( "Tab label", "Call Stack" ) );
    m_tabWidget->addTab( m_configView, i18nc( "Tab label", "Settings" ) );
    //tabWidget->addTab( m_ioView, i18n( "IO" ) );

    m_localsView = new LocalsView(m_localsToolView);

    m_debugView  = new DebugView( this );
    connect( m_debugView,  SIGNAL( readyForInput( bool ) ), 
             this,       SLOT( enableDebugActions( bool ) ) );

    connect( m_debugView,  SIGNAL( outputText( const QString ) ), 
             m_outputArea, SLOT( append( const QString ) ) );

    connect( m_debugView,  SIGNAL( outputError( const QString ) ), 
             this,       SLOT( addErrorText( const QString ) ) );

    connect( m_debugView,  SIGNAL( debugLocationChanged( const char*, int ) ),
             this,       SLOT( slotGoTo( const char*, int ) ) );

    connect( m_debugView,  SIGNAL( breakPointSet( KUrl const&, int ) ),
             this,       SLOT( slotBreakpointSet( KUrl const&, int ) ) );

    connect( m_debugView,  SIGNAL( breakPointCleared( KUrl const&, int ) ),
             this,       SLOT( slotBreakpointCleared( KUrl const&, int ) ) );

    connect( m_debugView,  SIGNAL( programEnded() ),
             this,       SLOT( programEnded() ) );

    connect( m_debugView,  SIGNAL( gdbEnded() ),
             this,       SLOT( programEnded() ) );

    connect( m_debugView,  SIGNAL( gdbEnded() ),
             this,       SLOT( gdbEnded() ) );

    connect( m_debugView,  SIGNAL( stackFrameInfo( QString, QString ) ),
             this,       SLOT( insertStackFrame( QString, QString ) ) );

    connect( m_debugView,  SIGNAL( stackFrameChanged( int ) ),
             this,       SLOT( stackFrameChanged( int ) ) );

    connect( m_debugView,  SIGNAL( infoLocal( QString ) ),
             m_localsView, SLOT( addLocal( QString ) ) );

    // Actions
    KAction* a = actionCollection()->addAction( "debug" );
    a->setText( i18n( "Start Debugging" ) );
    a->setIcon( KIcon( "debug" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this,   SLOT( slotDebug() ) );

    a = actionCollection()->addAction( "kill" );
    a->setText( i18n( "Kill / Stop Debugging" ) );
    a->setIcon( KIcon( "media-playback-stop" ) );
    connect(    a,         SIGNAL( triggered( bool ) ),
                m_debugView, SLOT( slotKill() ) );

    a = actionCollection()->addAction( "rerun" );
    a->setText( i18n( "Restart Debugging" ) );
    a->setIcon( KIcon( "view-refresh" ) );
    connect(    a,         SIGNAL( triggered( bool ) ),
                this, SLOT( slotRestart() ) );

    a = actionCollection()->addAction( "toggle_breakpoint" );
    a->setText( i18n( "Toggle Breakpoint / Break" ) );
    a->setIcon( KIcon( "media-playback-pause" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this,   SLOT( slotToggleBreakpoint() ) );

    a = actionCollection()->addAction( "step_in" );
    a->setText( i18n( "Step In" ) );
    a->setIcon( KIcon( "debug-step-into" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                m_debugView,   SLOT( slotStepInto() ) );

    a = actionCollection()->addAction( "step_over" );
    a->setText( i18n( "Step Over" ) );
    a->setIcon( KIcon( "debug-step-over" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                m_debugView,   SLOT( slotStepOver() ) );

    a = actionCollection()->addAction( "step_out" );
    a->setText( i18n( "Step Out" ) );
    a->setIcon( KIcon( "debug-step-out" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                m_debugView, SLOT( slotStepOut() ) );

    a = actionCollection()->addAction( "move_pc" );
    a->setText( i18nc( "Move Program Counter (next execution)", "Move PC" ) );
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
                m_debugView, SLOT( slotContinue() ) );

    a = actionCollection()->addAction( "print_value" );
    a->setText( i18n( "Print Value" ) );
    a->setIcon( KIcon( "document-preview" ) );
    connect(    a,      SIGNAL( triggered( bool ) ),
                this, SLOT( slotValue() ) );

    // popup context m_menu
    m_menu = new KActionMenu( i18n("Debug"), this );
    actionCollection()->addAction( "popup_gdb", m_menu );
    connect( m_menu->menu(), SIGNAL( aboutToShow() ), this, SLOT( aboutToShowMenu() ) );
    
    m_breakpoint = m_menu->menu()->addAction(i18n("popup_breakpoint"), 
                                         this, SLOT( slotToggleBreakpoint() ) );

    QAction* popupAction = m_menu->menu()->addAction(i18n( "popup_run_to_cursor" ), 
                                                   this, SLOT( slotRunToCursor() ) );
    popupAction->setText( i18n( "Run To Cursor" ) );
    popupAction = m_menu->menu()->addAction( "move_pc", 
                                          this, SLOT( slotMovePC() ) );
    popupAction->setText( i18nc( "Move Program Counter (next execution)", "Move PC" ) );
    
    enableDebugActions( false );
    
    mainWindow()->guiFactory()->addClient( this );
}

KatePluginGDBView::~KatePluginGDBView()
{
    mainWindow()->guiFactory()->removeClient( this );
    delete m_toolView;
    delete m_localsToolView;
}

void KatePluginGDBView::readSessionConfig(  KConfigBase*    config,
                                            const QString&  groupPrefix )
{
    m_configView->readConfig( config, groupPrefix );
}

void KatePluginGDBView::writeSessionConfig( KConfigBase*    config,
                                            const QString&  groupPrefix )
{
    m_configView->writeConfig( config, groupPrefix );
}

void KatePluginGDBView::slotDebug()
{
    m_configView->snapshotSettings();
    QString args = m_configView->currentArgs();
    if ( m_configView->showIOTab() )
    {
        args += QString( " < %1 1> %2 2> %3" )
        .arg( m_ioView->stdinFifo() )
        .arg( m_ioView->stdoutFifo() )
        .arg( m_ioView->stderrFifo() );
    }
    enableDebugActions( true );
    m_debugView->runDebugger( m_configView->currentWorkingDirectory(),
                            m_configView->currentExecutable(),
                            args );
    mainWindow()->showToolView( m_toolView );
    m_tabWidget->setCurrentWidget( m_gdbPage );
    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
    m_localsView->clear();
}

void KatePluginGDBView::slotRestart()
{
    mainWindow()->showToolView( m_toolView );
    m_tabWidget->setCurrentWidget( m_gdbPage );
    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
    m_debugView->slotReRun();
    m_localsView->clear();
}

void KatePluginGDBView::aboutToShowMenu()
{
    if (!m_debugView->debuggerRunning() || m_debugView->debuggerBusy())
    {
        m_breakpoint->setText(i18n("Insert breakpoint"));
        m_breakpoint->setDisabled(true);
        return;
    }

    m_breakpoint->setDisabled(false);

    KTextEditor::View* editView = mainWindow()->activeView();
    KUrl               url = editView->document()->url();
    int                line = editView->cursorPosition().line();

    line++; // GDB uses 1 based line numbers, kate uses 0 based...

    if (m_debugView->hasBreakpoint(url, line))
    {
        m_breakpoint->setText(i18n("Remove breakpoint"));
    }
    else
    {
        m_breakpoint->setText(i18n("Insert breakpoint"));
    }
}

void KatePluginGDBView::slotToggleBreakpoint()
{
    if ( !actionCollection()->action( "continue" )->isEnabled() ) 
    {
        m_debugView->slotInterrupt();
    }
    else 
    {
        KTextEditor::View* editView = mainWindow()->activeView();
        KUrl               currURL  = editView->document()->url();
        int                line     = editView->cursorPosition().line();

        m_debugView->toggleBreakpoint( currURL, line + 1 );
    }
}

void KatePluginGDBView::slotBreakpointSet( KUrl const& file, int line)
{
    KTextEditor::MarkInterface* iface =
    qobject_cast<KTextEditor::MarkInterface*>( m_kateApplication->documentManager()->findUrl( file ) );

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
    qobject_cast<KTextEditor::MarkInterface*>( m_kateApplication->documentManager()->findUrl( file ) );

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

    m_debugView->movePC( currURL, cursor.line() + 1 );
}

void KatePluginGDBView::slotRunToCursor()
{
    KTextEditor::View*      editView = mainWindow()->activeView();
    KUrl                    currURL = editView->document()->url();
    KTextEditor::Cursor     cursor = editView->cursorPosition();

    // GDB starts lines from 1, kate returns lines starting from 0 (displaying 1)
    m_debugView->runToCursor( currURL, cursor.line() + 1 );
}

void KatePluginGDBView::slotGoTo( const char* fileName, int lineNum )
{

    KUrl url = m_debugView->resolveFileName( fileName );

    // skip not existing files
    if (!QFile::exists (url.toLocalFile ())) 
    {
        m_lastExecLine = -1;
        return;
    }

    m_lastExecUrl = url;
    m_lastExecLine = lineNum;

    KTextEditor::View*  editView = mainWindow()->openUrl( m_lastExecUrl );
    editView->setCursorPosition( KTextEditor::Cursor( m_lastExecLine, 0 ) );
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
    actionCollection()->action( "toggle_breakpoint" )->setEnabled( m_debugView->debuggerRunning() );
    actionCollection()->action( "kill"              )->setEnabled( m_debugView->debuggerRunning() );
    actionCollection()->action( "rerun"             )->setEnabled( m_debugView->debuggerRunning() );

    m_inputArea->setEnabled( enable );
    m_stackTree->setEnabled( enable );
    m_localsView->setEnabled( enable );
    if ( enable )
    {
        m_inputArea->setFocusPolicy( Qt::WheelFocus );
        if ( m_focusOnInput || m_configView->takeFocusAlways() ) 
        {
            m_inputArea->setFocus();
            m_focusOnInput = false;
        }
        else
        {
            mainWindow()->activeView()->setFocus();
        }
    }
    else
    {
        m_inputArea->setFocusPolicy( Qt::NoFocus );
        if ( mainWindow()->activeView() ) mainWindow()->activeView()->setFocus();
    }

    m_ioView->enableInput( !enable && m_debugView->debuggerRunning() );

    if ( ( m_lastExecLine > -1 ) )
    {
        KTextEditor::MarkInterface* iface =
        qobject_cast<KTextEditor::MarkInterface*>( m_kateApplication->documentManager()->findUrl( m_lastExecUrl ) );

        if (iface) 
        {
            if ( enable )
            {
                iface->setMarkDescription(KTextEditor::MarkInterface::Execution, i18n("Execution point"));
                iface->setMarkPixmap(KTextEditor::MarkInterface::Execution, KIcon("arrow-right").pixmap(10,10));
                iface->addMark(m_lastExecLine, KTextEditor::MarkInterface::Execution);
            }
            else 
            {
                iface->removeMark( m_lastExecLine, KTextEditor::MarkInterface::Execution );
            }
        }
    }
}

void KatePluginGDBView::programEnded()
{
    // don't set the execution mark on exit
    m_lastExecLine = -1;
    m_stackTree->clear();
    m_localsView->clear();

    // Indicate the state change by showing the debug outputArea
    mainWindow()->showToolView( m_toolView );
    m_tabWidget->setCurrentWidget( m_gdbPage );
}

void KatePluginGDBView::gdbEnded()
{
    m_outputArea->clear();
    m_localsView->clear();
    m_ioView->clearOutput();
}

void KatePluginGDBView::slotSendCommand()
{
    QString cmd = m_inputArea->currentText();

    if ( cmd.isEmpty() ) cmd = m_lastCommand;

    m_inputArea->addToHistory( cmd );
    m_inputArea->setCurrentItem("");
    m_focusOnInput = true;
    m_lastCommand = cmd;
    m_debugView->issueCommand( cmd );

    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void KatePluginGDBView::insertStackFrame( QString const& level, QString const& info )
{
    if ( level.isEmpty() && info.isEmpty() ) {
        m_stackTree->resizeColumnToContents(2);
        return;
    }
    
    if ( level == "0") 
    {
        m_stackTree->clear();
    }
    QStringList columns;
    columns << "  "; // icon place holder
    columns << level;
    int lastSpace = info.lastIndexOf(" ");
    QString shortInfo = info.mid(lastSpace);
    columns << shortInfo;

    QTreeWidgetItem *item = new QTreeWidgetItem(columns);
    item->setToolTip(2, QString("<qt>%1<qt>").arg(info));
    m_stackTree->insertTopLevelItem(level.toInt(), item);
}

void KatePluginGDBView::stackFrameSelected()
{
    m_debugView->issueCommand( QString( "(Q)f %1" ).arg( m_stackTree->currentIndex().row() ) );
}

void KatePluginGDBView::stackFrameChanged( int level )
{
    QTreeWidgetItem *current = m_stackTree->topLevelItem(m_lastExecFrame);
    QTreeWidgetItem *next = m_stackTree->topLevelItem(level);

    if ( current ) current->setIcon ( 0, QIcon() );
    if ( next )    next->setIcon( 0, KIcon("arrow-right") );
    m_lastExecFrame = level;
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
    m_debugView->issueCommand( cmd );
    m_inputArea->addToHistory( cmd );
    m_inputArea->setCurrentItem("");

    mainWindow()->showToolView( m_toolView );
    m_tabWidget->setCurrentWidget( m_gdbPage );

    QScrollBar *sb = m_outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void KatePluginGDBView::showIO( bool show )
{
    if ( show )
    {
        m_tabWidget->addTab( m_ioView, i18n( "IO" ) );
    }
    else 
    {
        m_tabWidget->removeTab( m_tabWidget->indexOf(m_ioView) );
    }
}

void KatePluginGDBView::addErrorText( QString const& text )
{
    m_outputArea->setFontItalic( true );
    m_outputArea->append( text );
    m_outputArea->setFontItalic( false );
}


