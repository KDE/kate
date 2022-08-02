/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_H
#define KEYBOARDMACROSPLUGIN_H

#include <QEvent>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/MainWindow>
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

Q_SIGNALS:
    void message(const QVariantMap &message);

public:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QWidget *m_focusWidget = nullptr;

    KeyboardMacrosPluginCommands *m_commands;

    QAction *m_recordAction = nullptr;
    QAction *m_cancelAction = nullptr;
    QAction *m_playAction = nullptr;

    void record();
    void stop(bool save);
    void cancel();
    bool play(QString name = QString());

    bool save(QString name);
    bool load(QString name);
    bool remove(QString name);

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
    KeyboardMacrosPluginCommands(KeyboardMacrosPlugin *plugin);
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *, const QString &cmd, QString &msg) override;

private:
    KeyboardMacrosPlugin *m_plugin;
};

#endif
