//
// debugview.cpp
//
// Description: Manages the interaction with GDB
//
//
// Copyright (c) 2008-2010 Ian Wakeling <ian.wakeling@ntlworld.com>
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

#include "debugview.h"
#include "debugview.moc"

#include <QtCore/QRegExp>
#include <QtCore/QFile>
#include <QtCore/QTimer>

#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>

#include <signal.h>
#include <stdlib.h>

static Q_PID getDebuggeePid( Q_PID gdbPid );

DebugView::DebugView( QObject* parent )
:   QObject( parent ),
    debugProcess(0),
    state( none ),
    subState( normal )
{
}


DebugView::~DebugView()
{
    if ( debugProcess && ( debugProcess->state() != QProcess::NotRunning ) )
    {
        Q_PID pid = getDebuggeePid( debugProcess->pid() );
        if( pid != 0 )
        {
            ::kill( pid, SIGKILL );
        }
        debugProcess->kill();
        debugProcess->blockSignals( true );
        debugProcess->waitForFinished();
    }
}

void DebugView::runDebugger(    QString const&  newWorkingDirectory,
                                QString const&  newTarget,
                                QString const&  newArguments )
{
    workingDirectory = newWorkingDirectory;
    target = newTarget;
    arguments = newArguments;

    if( state == none )
    {
        outBuffer.clear();
        errBuffer.clear();
        errorList.clear();

        //create a process to control GDB
        debugProcess = new QProcess( this );
        debugProcess->setWorkingDirectory( workingDirectory );

        //use the shell to find gdb for us, rather than launching gdb directly
        const char* shell = getenv( "SHELL" );
        if( shell == NULL )
        {
            shell = "/bin/sh";
        }

        //prepare the arguments to pass to shell
        QStringList args;
        args.append( "-c" );
        args.append( "gdb" );

        connect( debugProcess, SIGNAL( error(QProcess::ProcessError) ),
                            this, SLOT( slotError() ) );

        connect( debugProcess, SIGNAL( readyReadStandardError() ),
                            this, SLOT( slotReadDebugStdErr() ) );

        connect( debugProcess, SIGNAL( readyReadStandardOutput() ),
                            this, SLOT( slotReadDebugStdOut() ) );

        connect( debugProcess, SIGNAL( finished(int,QProcess::ExitStatus) ),
                            this, SLOT( slotDebugFinished(int,QProcess::ExitStatus) ) );

        debugProcess->start( shell, args );

        nextCommands << "set pagination off";
        state = ready;
    }
    else
    {
        // On startup the gdb prompt will trigger the "nextCommands",
        // here we have to trigger it manually.
        QTimer::singleShot(0, this, SLOT(issueNextCommand()));
    }
    nextCommands << QString("file %1").arg(target);
    nextCommands << QString("set args %1").arg(arguments);
}

bool DebugView::debuggerRunning()
{
    return( state != none );
}

bool DebugView::debuggerBusy()
{
    return( state == executingCmd );
}

bool DebugView::hasBreakpoint( const KUrl& url, int line )
{
    for (int i = 0; i<breakPointList.size(); i++) {
        if ( (url == breakPointList[i].file) && (line == breakPointList[i].line) ) {
            return true;
        }
    }
    return false;
}

void DebugView::toggleBreakpoint( KUrl const& url, int line )
{
    if( state == ready )
    {
        QString cmd;
        if (hasBreakpoint( url, line ))
        {
            cmd = QString("clear %1:%2").arg(url.path()).arg(line);
        }
        else {
            cmd = QString("break %1:%2").arg(url.path()).arg(line);
        }
        issueCommand( cmd );
    }
}

void DebugView::slotError()
{
    KMessageBox::sorry( NULL, "Could not start debugger process" );
}

void DebugView::slotReadDebugStdOut()
{
    outBuffer += QString::fromLocal8Bit( debugProcess->readAllStandardOutput().data() );
    int end=0;
    // handle one line at a time
    do {
        end = outBuffer.indexOf('\n');
        if (end < 0) break;
        processLine( outBuffer.mid(0, end) );
        outBuffer.remove(0,end+1);
    } while (1);

    if (outBuffer == "(gdb) ") 
    {
        outBuffer.clear();
        processLine( "(gdb) " );
    }
}

void DebugView::slotReadDebugStdErr()
{
    errBuffer += QString::fromLocal8Bit( debugProcess->readAllStandardError().data() );
    int end=0;
    // add whole lines at a time to the error list
    do {
        end = errBuffer.indexOf('\n');
        if (end < 0) break;
        errorList << errBuffer.mid(0, end);
        errBuffer.remove(0,end+1);
    } while (1);

    processErrors();
}

void DebugView::slotDebugFinished( int /*exitCode*/, QProcess::ExitStatus status )
{
    if( status != QProcess::NormalExit )
    {
        emit outputText( "***gdb abended***" );
    }

    delete debugProcess;
    debugProcess = NULL;
    state = none;
    emit readyForInput( false );

    // remove all old breakpoints
    BreakPoint bPoint;
    while ( breakPointList.size() > 0 )
    {
        bPoint = breakPointList.takeFirst();
        emit breakPointCleared( bPoint.file, bPoint.line -1 );
    }

    emit gdbEnded();
}

void DebugView::movePC( KUrl const& url, int line )
{
    if( state == ready )
    {
        QString cmd = QString("tbreak %1%2").arg(url.path()).arg(line);
        nextCommands <<  QString("jump %1:%2").arg(url.path()).arg(line);
        issueCommand( cmd );
    }
}

void DebugView::runToCursor( KUrl const& url, int line )
{
    if( state == ready )
    {
        QString cmd = QString("tbreak %1:%2").arg(url.path()).arg(line);
        nextCommands << "continue";
        issueCommand( cmd );
    }
}

void DebugView::slotInterrupt()
{
    if( state == executingCmd )
    {
        Q_PID pid = getDebuggeePid( debugProcess->pid() );
        if( pid != 0 )
        {
            ::kill( pid, SIGINT );
        }
    }
}

void DebugView::slotKill()
{
    if( state == ready )
    {
        issueCommand( "kill" );
    }
    else if( state == executingCmd )
    {
        Q_PID pid = getDebuggeePid( debugProcess->pid() );
        if( pid != 0 )
        {
            ::kill( pid, SIGKILL );
        }
    }
}

void DebugView::slotReRun()
{
    if( state == ready )
    {
        issueCommand( "kill" );
    }
    else if( state == executingCmd )
    {
        Q_PID pid = getDebuggeePid( debugProcess->pid() );
        if( pid != 0 )
        {
            ::kill( pid, SIGKILL );
        }
    }
    nextCommands << QString("file %1").arg(target);
    nextCommands << QString("set args %1").arg(arguments);
    nextCommands << "run";
}

void DebugView::slotStepInto()
{
    nextCommands << "(Q)info stack";
    nextCommands << "(Q)frame";
    issueCommand( "step" );
}

void DebugView::slotStepOver()
{
    nextCommands << "(Q)info stack";
    nextCommands << "(Q)frame";
    issueCommand( "next" );
}

void DebugView::slotStepOut()
{
    nextCommands << "(Q)info stack";
    nextCommands << "(Q)frame";
    issueCommand( "finish" );
}

void DebugView::slotContinue()
{
    nextCommands << "(Q)info stack";
    nextCommands << "(Q)frame";
    issueCommand( "continue" );
}



void DebugView::processLine( QString line )
{
    if (line.isEmpty()) return;

    static QRegExp prompt( "\\(gdb\\).*" );
    static QRegExp breakpointList( "Num\\s+Type\\s+Disp\\s+Enb\\s+Address\\s+What.*" );
    static QRegExp stackFrameAny( "#(\\d+)\\s(.*)" );
    static QRegExp stackFrameFile( "#(\\d+)\\s+(?:0x[\\da-f]+\\s*in\\s)*(\\S+)(\\s\\([^)]*\\))\\sat\\s([^:]+):(\\d+).*" );
    static QRegExp changeFile( "(?:(?:Temporary\\sbreakpoint|Breakpoint)\\s*\\d+,\\s*|0x[\\da-f]+\\s*in\\s*)?[^\\s]+\\s*\\([^)]*\\)\\s*at\\s*([^:]+):(\\d+).*" );
    static QRegExp changeLine( "(\\d+)\\s+.*" );
    static QRegExp breakPointReg( "Breakpoint\\s+(\\d+)\\s+at\\s+0x[\\da-f]+:\\s+file\\s+([^\\,]+)\\,\\s+line\\s+(\\d+).*" );
    static QRegExp breakPointDel( "Deleted\\s+breakpoint.*" );
    static QRegExp exitProgram( "Program\\s+exited.*" );

    switch( state )
    {
        case none:
        case ready:
            if( prompt.exactMatch( line ) ) 
            {
                // we get here after initialization
                QTimer::singleShot(0, this, SLOT(issueNextCommand()));
            }
            break;

        case executingCmd:
            if( breakpointList.exactMatch( line ) )
            {
                state = listingBreakpoints;
            }
            else if ( stackFrameAny.exactMatch( line ) )
            {
                if ( lastCommand.contains( "info stack" ) )
                {
                    emit stackFrameInfo( stackFrameAny.cap(1), stackFrameAny.cap(2));
                }
                else
                {
                    subState = ( subState == normal ) ? stackFrameSeen : stackTraceSeen;

                    newFrameLevel = stackFrameAny.cap( 1 ).toInt();

                    if ( stackFrameFile.exactMatch( line ) )
                    {
                        newFrameFile = stackFrameFile.cap( 4 );
                    }
                }
            }
            else if( changeFile.exactMatch( line ) )
            {
                currentFile = changeFile.cap( 1 ).trimmed();
                int lineNum = changeFile.cap( 2 ).toInt();

                // GDB uses 1 based line numbers, kate uses 0 based...
                emit debugLocationChanged( currentFile.toLocal8Bit(), lineNum - 1 );
            }
            else if( changeLine.exactMatch( line ) )
            {
                int lineNum = changeLine.cap( 1 ).toInt();

                if( subState == stackFrameSeen )
                {
                    currentFile = newFrameFile;
                }
                // GDB uses 1 based line numbers, kate uses 0 based...
                emit debugLocationChanged( currentFile.toLocal8Bit(), lineNum - 1 );
            }
            else if (breakPointReg.exactMatch(line)) 
            {
                BreakPoint breakPoint;
                breakPoint.number = breakPointReg.cap( 1 ).toInt();
                breakPoint.file = breakPointReg.cap( 2 );
                breakPoint.line = breakPointReg.cap( 3 ).toInt();
                breakPointList << breakPoint;
                emit breakPointSet( breakPoint.file, breakPoint.line -1 );
            }
            else if (breakPointDel.exactMatch(line)) 
            {
                line.remove("Deleted breakpoint");
                line.remove("s"); // in case of multiple breakpoints
                QStringList numbers = line.split(' ', QString::SkipEmptyParts);
                for (int i=0; i<numbers.size(); i++) 
                {
                    for (int j = 0; j<breakPointList.size(); j++) 
                    {
                        if ( numbers[i].toInt() == breakPointList[j].number ) 
                        {
                            emit breakPointCleared( breakPointList[j].file, breakPointList[j].line -1 );
                            breakPointList.removeAt(j);
                            break;
                        }
                    }
                }
            }
            else if ( exitProgram.exactMatch( line ) || 
                line.contains( "The program no longer exists" ) ||
                line.contains( "Kill the program being debugged" ) )
            {
                if ( ( nextCommands.size() > 0 ) && !nextCommands[0].contains("file") ) 
                {
                    nextCommands.clear();
                }
                emit programEnded();
            }
            else if ( line.contains( "Program received signal" ) )
            {
                nextCommands << "(Q)info stack";
                nextCommands << "(Q)frame";
            }
            else if( prompt.exactMatch( line ) )
            {
                if( subState == stackFrameSeen )
                {
                    emit stackFrameChanged( newFrameLevel );
                }
                state = ready;

                // Give the error a possibility get noticed since stderr and stdout are not in sync
                QTimer::singleShot(0, this, SLOT(issueNextCommand()));
            }
            break;

        case listingBreakpoints:
            if( prompt.exactMatch( line ) )
            {
                state = ready;
                emit readyForInput( true );
            }
            break;
    }
    outputTextMaybe( line );
}

void DebugView::processErrors()
{
    QString error;
    while (errorList.size() > 0) {
        error = errorList.takeFirst();
        //kDebug() << error;
        if( error == "The program is not being run." )
        {
            if ( lastCommand == "continue" ) 
            {
                nextCommands.clear();
                nextCommands << "run";
                QTimer::singleShot(0, this, SLOT(issueNextCommand()));
            }
            else if ( ( lastCommand == "step" ) ||
                ( lastCommand == "next" ) ||
                ( lastCommand == "finish" ) )
            {
                nextCommands.clear();
                nextCommands << "tbreak main";
                nextCommands << "run";
                QTimer::singleShot(0, this, SLOT(issueNextCommand()));
            }
            else if ( (lastCommand == "kill"))
            {
                if ( nextCommands.size() > 0 )
                {
                    if ( !nextCommands[0].contains("file") )
                    {
                        nextCommands.clear();
                        nextCommands << "quit";
                    }
                    // else continue with "ReRun"
                }
                else 
                {
                    nextCommands << "quit";
                }
                QTimer::singleShot(0, this, SLOT(issueNextCommand()));
            }
            // else do nothing
        }
        else if ( error.contains( "No line " ) ||
            error.contains( "No source file named" ) )
        {
            // setting a breakpoint failed. Do not continue.
            nextCommands.clear();
            emit readyForInput( true );
        }
        else if ( error.contains( "No stack" ) )
        {
            nextCommands.clear();
            emit programEnded();
        }
        emit outputError( error );
    }
 }

void DebugView::issueCommand( QString const& cmd )
{
    if( state == ready )
    {
        emit readyForInput( false );
        state = executingCmd;
        subState = normal;
        lastCommand = cmd;
        if ( cmd.startsWith("(Q)") ) 
        {
            debugProcess->write( cmd.mid(3).toLocal8Bit() + "\n" );
        }
        else {
            emit outputText( "(gdb) " + cmd );
            debugProcess->write( cmd.toLocal8Bit() + "\n" );
        }
    }
}

void DebugView::issueNextCommand()
{
    if( state == ready )
    {
        if( nextCommands.size() > 0)
        {
            QString cmd = nextCommands.takeFirst();
            //kDebug() << "Next command" << cmd;
            issueCommand( cmd );
        }
        else 
        {
            emit readyForInput( true );
        }
    }
}

KUrl DebugView::resolveFileName( char const* fileName )
{
    KUrl url;

    //did we end up with an absolute path or a relative one?
    if(  fileName[0] == '/' )
    {
        url.setPath( fileName );
    }
    else
    {
        url.setPath( workingDirectory );
        url.addPath( fileName );
        url.cleanPath();
    }

    return url;
}

void DebugView::outputTextMaybe( const QString &text )
{
    if ( !lastCommand.startsWith( "(Q)" )  && !text.contains( "(gdb)" ) )
    {
        emit outputText( text );
    }
}

static Q_PID getDebuggeePid( Q_PID gdbPid )
{
    Q_PID                   pid = 0;
    Q_PID                   parentPid = 0;
    QDir                    dir( "/proc");
    QStringList             entries = dir.entryList( QDir::Dirs );
    QStringList::iterator   iter = entries.begin();

    while(  parentPid != gdbPid && iter != entries.end() )
    {
        pid = (*iter).toInt();
        if( pid != 0 && pid != gdbPid )
        {
            QFile   f( QString( "/proc/" + *iter + "/status" ) );
            if( f.open( QIODevice::ReadOnly | QIODevice::Text ) )
            {
                QString line( f.readLine() );
                parentPid = -1;
                while( parentPid == -1 && line.length() > 0 )
                {
                    QStringList nameValuePair = line.split( '\t' );
                    if( nameValuePair[0] == "PPid:" )
                    {
                        parentPid = nameValuePair[1].toInt();
                    }
                    else
                    {
                        line = f.readLine();
                    }
                }
            }
        }
        iter++;
    }

    return( parentPid == gdbPid ? pid : 0 );
}

