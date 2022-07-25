/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLUGIN_KATEKEYBOARDMACRO_H
#define PLUGIN_KATEKEYBOARDMACRO_H

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
    /**
     * Plugin constructor.
     */
    explicit PluginKateKeyboardMacro(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~PluginKateKeyboardMacro() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    bool record(KTextEditor::View *view);
    bool run(KTextEditor::View *view);
    bool isRecording();

private:
    KTextEditor::MainWindow *m_mainWindow;

    bool m_recording = false;
    QString m_macro;

    PluginKateKeyboardMacroRecordCommand *m_recCommand;
    PluginKateKeyboardMacroRunCommand *m_runCommand;

public Q_SLOTS:
    void slotRecord();
    void slotRun();
};

/**
 * recmac command
 */
class PluginKateKeyboardMacroRecordCommand : public KTextEditor::Command
{
    Q_OBJECT

public:
    PluginKateKeyboardMacroRecordCommand(PluginKateKeyboardMacro *plugin);
    // Kate::Command
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
    // Kate::Command
    bool exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range & = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &, QString &msg) override;

private:
    PluginKateKeyboardMacro *m_plugin;
};

/**
 * Plugin view to merge the actions into the UI
 */
class PluginViewKateKeyboardMacro : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Construct plugin view
     * @param plugin our plugin
     * @param mainwindows the mainwindow for this view
     */
    explicit PluginViewKateKeyboardMacro(PluginKateKeyboardMacro *plugin, KTextEditor::MainWindow *mainwindow);

    /**
     * Our Destructor
     */
    ~PluginViewKateKeyboardMacro() override;

private:
    /**
     * the main window we belong to
     */
    KTextEditor::MainWindow *m_mainWindow;
};

#endif
