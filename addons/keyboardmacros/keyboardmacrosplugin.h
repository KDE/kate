/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_H
#define KEYBOARDMACROSPLUGIN_H

#include <QEvent>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVariantMap>

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

public:
    explicit KeyboardMacrosPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~KeyboardMacrosPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void sendMessage(const QString &text, bool error);
    void displayMessage(const QString &text, KTextEditor::Message::MessageType type);

Q_SIGNALS:
    void message(const QVariantMap &message);

public:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QPointer<QWidget> m_focusWidget;
    QPointer<KTextEditor::Message> m_message;

    KeyboardMacrosPluginCommands *m_commands;

    QAction *m_recordAction = nullptr;
    QAction *m_cancelAction = nullptr;
    QAction *m_playAction = nullptr;
    QAction *m_saveNamedAction = nullptr;
    QAction *m_loadNamedAction = nullptr;
    // QAction *m_playNamedAction = nullptr;
    QAction *m_deleteNamedAction = nullptr;

    void record();
    void stop(bool save);
    void cancel();
    bool play(const QString &name = QString());

    bool save(const QString &name);
    bool load(const QString &name);
    bool remove(const QString &name);

    QString queryName(const QString &query, const QString &action);

    bool m_recording = false;
    Macro m_tape;
    Macro m_macro;
    QMap<QString, Macro> m_namedMacros;
    QString m_storage;

    void loadNamedMacros();
    void saveNamedMacros();

    void focusObjectChanged(QObject *focusObject);
    void applicationStateChanged(Qt::ApplicationState state);

public Q_SLOTS:
    void slotRecord();
    void slotCancel();
    void slotPlay();
    void slotSaveNamed();
    void slotLoadNamed();
    // void slotPlayNamed();
    void slotDeleteNamed();
};

/**
 * Plugin view to add keyboard macros actions to the GUI
 */
class KeyboardMacrosPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit KeyboardMacrosPluginView(KeyboardMacrosPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~KeyboardMacrosPluginView() override;

private:
    KTextEditor::MainWindow *m_mainWindow;
};

/**
 * Plugin commands to manage named keyboard macros
 */
class KeyboardMacrosPluginCommands : public KTextEditor::Command
{
    Q_OBJECT

public:
    explicit KeyboardMacrosPluginCommands(KeyboardMacrosPlugin *plugin);
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *, const QString &cmd, QString &msg) override;

private:
    KeyboardMacrosPlugin *m_plugin;
};

#endif
