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

#ifndef PLUGIN_KATEGDB_H
#define PLUGIN_KATEGDB_H

#include <QPointer>

#include <kate/mainwindow.h>
#include <kate/plugin.h>

#include <kdeversion.h>
#include <kxmlguiclient.h>
#include <kactionmenu.h>

#include "debugview.h"
#include "configview.h"
#include "ioview.h"
#include "localsview.h"

class KHistoryComboBox;
class QTextEdit;
class QTreeWidget;

typedef QList<QVariant> VariantList;

class KatePluginGDB : public Kate::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginGDB( QObject* parent = NULL, const VariantList& = VariantList() );
    virtual ~KatePluginGDB();

    Kate::PluginView* createView( Kate::MainWindow* mainWindow );
};

class KatePluginGDBView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    KatePluginGDBView( Kate::MainWindow* mainWindow, Kate::Application* application );
    ~KatePluginGDBView();

    virtual void readSessionConfig( KConfigBase* config, const QString& groupPrefix );
    virtual void writeSessionConfig( KConfigBase* config, const QString& groupPrefix );

private Q_SLOTS:
    void slotDebug();
    void slotRestart();
    void slotToggleBreakpoint();
    void slotMovePC();
    void slotRunToCursor();
    void slotGoTo( const KUrl &fileName, int lineNum );
    void slotValue();

    void aboutToShowMenu();
    void slotBreakpointSet( const KUrl &file, int line );
    void slotBreakpointCleared( const KUrl &file, int line );
    void slotSendCommand();
    void enableDebugActions( bool enable );
    void programEnded();
    void gdbEnded();
    void insertStackFrame( QString const& level, QString const& info );
    void stackFrameChanged( int level );
    void stackFrameSelected();
    void showIO( bool show );
    void addErrorText( QString const& text );
    void clearMarks();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private:
    QString currentWord();


    Kate::Application*    m_kateApplication;
    QWidget*              m_toolView;
    QWidget*              m_localsToolView;
    QWidget*              m_stackToolView;
    QTabWidget*           m_tabWidget;
    QTextEdit*            m_outputArea;
    KHistoryComboBox*     m_inputArea;
    QWidget*              m_gdbPage;
    QTreeWidget*          m_stackTree;
    QString               m_lastCommand;
    DebugView*            m_debugView;
    ConfigView*           m_configView;
    IOView*               m_ioView;
    LocalsView*           m_localsView;
    QPointer<KActionMenu> m_menu;
    QAction*              m_breakpoint;
    KUrl                  m_lastExecUrl;
    int                   m_lastExecLine;
    int                   m_lastExecFrame;
    bool                  m_focusOnInput;
};

#endif
