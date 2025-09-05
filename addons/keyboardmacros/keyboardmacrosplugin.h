/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QEvent>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>

#include <KTextEditor/Application>
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
    QKeySequence m_saveActionShortcut;
    Macro m_tape;
    Macro m_macro;
    QString m_storage;
    bool m_namedMacrosLoaded = false;
    std::map<QString, Macro> m_namedMacros;
    QSet<QString> m_wipedMacros;

    // Plugin creation and destruction
public:
    explicit KeyboardMacrosPlugin(QObject *parent);
    ~KeyboardMacrosPlugin() override;
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    void loadNamedMacros();
    void saveNamedMacros();

    // GUI feedback helpers
    void sendMessage(const QString &text, bool error);
    void displayMessage(const QString &text, KTextEditor::Message::MessageType type, bool noAutoHide = false);

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
