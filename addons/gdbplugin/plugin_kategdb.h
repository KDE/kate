//
// Description: Kate Plugin for GDB integration
//
//
// Copyright (c) 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// Copyright (c) 2010-2014 Kåre Särs <kare.sars@iki.fi>
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

#include <KTextEditor/Application>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>
#include <KActionMenu>
#include <KTextEditor/SessionConfigInterface>

#include "debugview.h"
#include "configview.h"
#include "ioview.h"
#include "localsview.h"

class KHistoryComboBox;
class QTextEdit;
class QTreeWidget;

typedef QList<QVariant> VariantList;

class KatePluginGDB : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginGDB(QObject* parent = nullptr, const VariantList& = VariantList());
    ~KatePluginGDB() override;

    QObject* createView(KTextEditor::MainWindow* mainWindow) override;
};

class KatePluginGDBView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)

public:
  KatePluginGDBView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWin);
    ~KatePluginGDBView() override;

    // reimplemented: read and write session config
    void readSessionConfig (const KConfigGroup& config) override;
    void writeSessionConfig (KConfigGroup& config) override;

private Q_SLOTS:
    void slotDebug();
    void slotRestart();
    void slotToggleBreakpoint();
    void slotMovePC();
    void slotRunToCursor();
    void slotGoTo(const QUrl &fileName, int lineNum);
    void slotValue();

    void aboutToShowMenu();
    void slotBreakpointSet(const QUrl &file, int line);
    void slotBreakpointCleared(const QUrl &file, int line);
    void slotSendCommand();
    void enableDebugActions(bool enable);
    void programEnded();
    void gdbEnded();

    void insertStackFrame(QString const& level, QString const& info);
    void stackFrameChanged(int level);
    void stackFrameSelected();

    void insertThread(int number, bool active);
    void threadSelected(int thread);

    void showIO(bool show);
    void addOutputText(QString const& text);
    void addErrorText(QString const& text);
    void clearMarks();
    void handleEsc(QEvent *e);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QString currentWord();


    KTextEditor::Application *m_kateApplication;
    KTextEditor::MainWindow  *m_mainWin;
    QWidget*              m_toolView;
    QWidget*              m_localsStackToolView;
    QTabWidget*           m_tabWidget;
    QTextEdit*            m_outputArea;
    KHistoryComboBox*     m_inputArea;
    QWidget*              m_gdbPage;
    QComboBox*            m_threadCombo;
    int                   m_activeThread;
    QTreeWidget*          m_stackTree;
    QString               m_lastCommand;
    DebugView*            m_debugView;
    ConfigView*           m_configView;
    IOView*               m_ioView;
    LocalsView*           m_localsView;
    QPointer<KActionMenu> m_menu;
    QAction*              m_breakpoint;
    QUrl                  m_lastExecUrl;
    int                   m_lastExecLine;
    int                   m_lastExecFrame;
    bool                  m_focusOnInput;
};

#endif
