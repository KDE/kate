/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLUGIN_KATEKEYBOARDMACRO_H
#define PLUGIN_KATEKEYBOARDMACRO_H

#include <QKeyEvent>
#include <QList>

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

class KeyboardMacrosPluginRecordCommand;
class KeyboardMacrosPluginRunCommand;

class KeyboardMacrosPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KeyboardMacrosPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~KeyboardMacrosPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    bool eventFilter(QObject *obj, QEvent *event) override;

    void reset();
    bool record(KTextEditor::View *view);
    bool run(KTextEditor::View *view);
    bool isRecording();

private:
    KTextEditor::MainWindow *m_mainWindow;

    bool m_recording = false;
    QList<QKeyEvent *> m_keyEvents;

    KeyboardMacrosPluginRecordCommand *m_recCommand;
    KeyboardMacrosPluginRunCommand *m_runCommand;

public Q_SLOTS:
    void slotRecord();
    void slotRun();
};

/**
 * Plugin view to add our actions to the gui
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
 * recmac command
 */
class KeyboardMacrosPluginRecordCommand : public KTextEditor::Command
{
    Q_OBJECT

public:
    KeyboardMacrosPluginRecordCommand(KeyboardMacrosPlugin *plugin);
    bool exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &, QString &msg) override;

private:
    KeyboardMacrosPlugin *m_plugin;
};

/**
 * runmac command
 */
class KeyboardMacrosPluginRunCommand : public KTextEditor::Command
{
    Q_OBJECT

public:
    KeyboardMacrosPluginRunCommand(KeyboardMacrosPlugin *plugin);
    bool exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &, QString &msg) override;

private:
    KeyboardMacrosPlugin *m_plugin;
};

#endif
