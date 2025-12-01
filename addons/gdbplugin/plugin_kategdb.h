//
// Description: Kate Plugin for GDB integration
//
//
// SPDX-FileCopyrightText: 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2010-2014 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#pragma once

#include <QPointer>

#include <KActionMenu>
#include <KTerminalLauncherJob>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>
#include <KXMLGUIClient>
#include <QTimer>
#include <optional>

#include "backend.h"
#include "configview.h"
#include "dap/entities.h"
#include "ioview.h"
#include "localsview.h"
#include "sessionconfig.h"

class KHistoryComboBox;
class QTextEdit;
class QTreeWidget;
class QTreeView;
class QSplitter;
class QTabWidget;
class StackView;
class BreakpointView;

typedef QVariantList VariantList;

class KatePluginGDB : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginGDB(QObject *parent = nullptr, const VariantList & = VariantList());
    ~KatePluginGDB() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;
    void readConfig();
    void writeConfig() const;

    const QString m_settingsPath;
    const QUrl m_defaultConfigPath;
    QUrl m_configPath;
    QUrl configPath() const
    {
        return m_configPath.isEmpty() ? m_defaultConfigPath : m_configPath;
    }
Q_SIGNALS:
    // Update for config
    void update() const;
};

class KatePluginGDBView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)

public:
    KatePluginGDBView(KatePluginGDB *plugin, KTextEditor::MainWindow *mainWin);
    ~KatePluginGDBView() override;

    // reimplemented: read and write session config
    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;

private Q_SLOTS:
    void slotDebug();
    void slotRestart();
    void slotMovePC();
    void slotRunToCursor();
    void slotGoTo(const QUrl &fileName, int lineNum);
    void slotValue();

    void aboutToShowMenu();
    void slotSendCommand();
    void enableDebugActions(bool enable);
    void programEnded();

    void showIO(bool show);
    void addOutput(const dap::Output &output);
    void addOutputText(QString const &text);
    void addErrorText(QString const &text);
    void handleEsc(QEvent *e);
    void requestRunInTerminal(const dap::RunInTerminalRequestArguments &args, const dap::Client::ProcessInTerminal &notifyCreation);

    void onToolViewMoved(QWidget *toolview, KTextEditor::MainWindow::ToolViewPosition);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QString currentWord();
    void initDebugToolview();

    void displayMessage(const QString &message, KTextEditor::Message::MessageType level);
    void enableHotReloadOnSave(KTextEditor::View *view);
    QToolButton *createDebugButton(QAction *action);
    KTextEditor::MainWindow::ToolViewPosition toolviewPosition(QWidget *toolview) const;

    KatePluginGDB *const m_plugin;
    KTextEditor::Application *m_kateApplication;
    KTextEditor::MainWindow *m_mainWin;
    std::unique_ptr<QWidget> m_toolView;
    std::unique_ptr<QWidget> m_localsStackToolView;
    std::unique_ptr<QWidget> m_breakpointToolView;
    QTabWidget *m_tabWidget;
    QTextEdit *m_outputArea;
    KHistoryComboBox *m_inputArea;
    QWidget *m_gdbPage;
    QString m_lastCommand;
    Backend *m_backend;
    ConfigView *m_configView = nullptr;
    std::unique_ptr<IOView> m_ioView;
    LocalsView *m_localsView;
    StackView *m_stackView;
    BreakpointView *m_breakpointView;
    QPointer<KActionMenu> m_menu;
    QAction *m_breakpoint;
    QUrl m_lastExecUrl;
    int m_lastExecLine;
    bool m_focusOnInput;
    QPointer<KTextEditor::Message> m_infoMessage;
    KSelectAction *m_targetSelectAction = nullptr;
    QSplitter *m_localsStackSplitter;

    QAction *m_hotReloadOnSaveAction;
    QTimer m_hotReloadTimer;
    QMetaObject::Connection m_hotReloadOnSaveConnection;

    // Debug buttons
    QWidget *m_buttonWidget;
    QToolButton *m_continueButton;
    DebugPluginSessionConfig::ConfigData m_sessionConfig;
};
