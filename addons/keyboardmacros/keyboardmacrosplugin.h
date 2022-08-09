/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_H
#define KEYBOARDMACROSPLUGIN_H

#include <QEvent>
#include <QKeySequence>
#include <QList>
#include <QLockFile>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <KActionMenu>

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include "macro.h"

class KeyboardMacrosPluginView;
class KeyboardMacrosPluginCommands;

class KeyboardMacrosPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    friend KeyboardMacrosPluginView;
    friend KeyboardMacrosPluginCommands;

    // GUI elements
    QList<QPointer<KeyboardMacrosPluginView>> m_pluginViews;
    KeyboardMacrosPluginCommands *m_commands;

    // State and logic
    bool m_recording = false;
    QPointer<QWidget> m_focusWidget;
    QKeySequence m_recordActionShortcut;
    QKeySequence m_playActionShortcut;
    Macro m_tape;
    Macro m_macro;
    QString m_storage;
    QLockFile *m_storageLock;
    QMap<QString, Macro> m_namedMacros;
    QSet<QString> m_wipedMacros;

public:
    // Plugin creation and destruction
    explicit KeyboardMacrosPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KeyboardMacrosPlugin() override;
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    void clearPluginViews();

private:
    void loadNamedMacros(bool locked = false);
    void saveNamedMacros();

    // GUI feedback helpers
    void sendMessage(const QString &text, bool error);
    void displayMessage(const QString &text, KTextEditor::Message::MessageType type);

Q_SIGNALS:
    void message(const QVariantMap &message);

    // Events filter and focus management helpers
public:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void focusObjectChanged(QObject *focusObject);
    void applicationStateChanged(Qt::ApplicationState state);

    // Core functions
    void record();
    void cancel();
    void stop(bool save);
    bool play(const QString &name = QString());
    bool save(const QString &name);
    bool load(const QString &name);
    bool wipe(const QString &name);
};

/**
 * Plugin view to add keyboard macros actions to the GUI
 */
class KeyboardMacrosPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    KeyboardMacrosPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QPointer<QAction> m_recordAction;
    QPointer<QAction> m_cancelAction;
    QPointer<QAction> m_playAction;
    QPointer<QAction> m_saveAction;
    QPointer<KActionMenu> m_loadMenu;
    QMap<QString, QPointer<QAction>> m_namedMacrosLoadActions;
    QPointer<KActionMenu> m_playMenu;
    QMap<QString, QPointer<QAction>> m_namedMacrosPlayActions;
    QPointer<KActionMenu> m_wipeMenu;
    QMap<QString, QPointer<QAction>> m_namedMacrosWipeActions;

public:
    explicit KeyboardMacrosPluginView(KeyboardMacrosPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~KeyboardMacrosPluginView() override;

    // shortcut getter
    QKeySequence recordActionShortcut() const;
    QKeySequence playActionShortcut() const;

    // GUI update helpers
    void recordingOn();
    void recordingOff();
    void macroLoaded(bool enable);
    void addNamedMacro(const QString &name, const QString &description);
    void removeNamedMacro(const QString &name);

    // Action slots
public Q_SLOTS:
    void slotRecord();
    void slotCancel();
    void slotPlay();
    void slotSave();
    void slotLoadNamed(const QString &name = QString());
    void slotPlayNamed(const QString &name = QString());
    void slotWipeNamed(const QString &name = QString());
};

/**
 * Plugin commands to manage named keyboard macros
 */
class KeyboardMacrosPluginCommands : public KTextEditor::Command
{
    Q_OBJECT

public:
    explicit KeyboardMacrosPluginCommands(KeyboardMacrosPlugin *plugin);
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &) override;
    bool help(KTextEditor::View *, const QString &cmd, QString &msg) override;

private:
    KeyboardMacrosPlugin *m_plugin;
};

#endif
