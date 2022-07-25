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

class PluginKateKeyboardMacroRecordCommand;
class PluginKateKeyboardMacroRunCommand;

class PluginKateKeyboardMacro : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit PluginKateKeyboardMacro(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~PluginKateKeyboardMacro() override;

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

    PluginKateKeyboardMacroRecordCommand *m_recCommand;
    PluginKateKeyboardMacroRunCommand *m_runCommand;

public Q_SLOTS:
    void slotRecord();
    void slotRun();
};

/**
 * Plugin view to add our actions to the gui
 */
class PluginViewKateKeyboardMacro : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit PluginViewKateKeyboardMacro(PluginKateKeyboardMacro *plugin, KTextEditor::MainWindow *mainwindow);
    ~PluginViewKateKeyboardMacro() override;

private:
    KTextEditor::MainWindow *m_mainWindow;
};

/**
 * recmac command
 */
class PluginKateKeyboardMacroRecordCommand : public KTextEditor::Command
{
    Q_OBJECT

public:
    PluginKateKeyboardMacroRecordCommand(PluginKateKeyboardMacro *plugin);
    bool exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &, QString &msg) override;

private:
    PluginKateKeyboardMacro *m_plugin;
};

/**
 * runmac command
 */
class PluginKateKeyboardMacroRunCommand : public KTextEditor::Command
{
    Q_OBJECT

public:
    PluginKateKeyboardMacroRunCommand(PluginKateKeyboardMacro *plugin);
    bool exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &, QString &msg) override;

private:
    PluginKateKeyboardMacro *m_plugin;
};

#endif
