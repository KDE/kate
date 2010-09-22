//
// debugview.h
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

#ifndef DEBUGVIEW_H
#define DEBUGVIEW_H

#include <QtCore/QProcess>
#include <QtCore/QObject>

#include <kurl.h>

class DebugView : public QObject
{
Q_OBJECT
public:
    DebugView( QObject* parent );
    ~DebugView();

    void runDebugger(   QString const&  workingDirectory,
                        QString const&  target,
                        QString const&  arguments );
    bool debuggerRunning();
    bool debuggerBusy();
    bool hasBreakpoint( KUrl const& url, int line );

    KUrl resolveFileName( char const* fileName );

    void toggleBreakpoint( KUrl const& url, int line );
    void movePC( KUrl const& url, int line );
    void runToCursor( KUrl const& url, int line );

    void issueCommand( QString const& cmd );


public Q_SLOTS:
    void slotInterrupt();
    void slotStepInto();
    void slotStepOver();
    void slotStepOut();
    void slotContinue();
    void slotKill();
    void slotReRun();

private Q_SLOTS:
    void slotError();
    void slotReadDebugStdOut();
    void slotReadDebugStdErr();
    void slotDebugFinished( int exitCode, QProcess::ExitStatus status );
    void issueNextCommand();

Q_SIGNALS:
    void debugLocationChanged( const char* filename, int lineNum );
    void breakPointSet( KUrl const& filename, int lineNum );
    void breakPointCleared( KUrl const& filename, int lineNum );
    void stackFrameInfo( QString const& level, QString const& info );
    void stackFrameChanged( int level );
    
    void outputText( const QString &text );
    void outputError( const QString &text );
    void readyForInput( bool ready );
    void programEnded();
    void gdbEnded();

private:
    enum State
    {
        none,
        ready,
        executingCmd,
        listingBreakpoints
    };

    enum SubState
    {
        normal,
        stackFrameSeen,
        stackTraceSeen
    };

    struct BreakPoint
    {
        int  number;
        KUrl file;
        int  line;
    };

private:
    void processLine( QString output );
    void processErrors();
    void outputTextMaybe( const QString &text );

private:
    QProcess*           debugProcess;
    QString             workingDirectory;
    QString             target;
    QString             arguments;
    State               state;
    SubState            subState;
    QString             currentFile;
    QString             newFrameFile;
    int                 newFrameLevel;
    QStringList         nextCommands;
    QString             lastCommand;
    QList<BreakPoint>   breakPointList;
    QString             outBuffer;
    QString             errBuffer;
    QStringList         errorList;
};

#endif
