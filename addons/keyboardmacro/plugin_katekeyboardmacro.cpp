/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "plugin_katekeyboardmacro.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/message.h>

#include <KLocalizedString>
#include <QAction>
#include <QString>

#include <KActionCollection>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <QApplication>
#include <QClipboard>

#include <iostream>

K_PLUGIN_FACTORY_WITH_JSON(KeyboardMacroPluginFactory, "keyboardmacroplugin.json", registerPlugin<PluginKateKeyboardMacro>();)

PluginKateKeyboardMacro::PluginKateKeyboardMacro(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
    // register commands
    m_recCommand = new PluginKateKeyboardMacroRecordCommand(this);
    m_runCommand = new PluginKateKeyboardMacroRunCommand(this);
}

PluginKateKeyboardMacro::~PluginKateKeyboardMacro()
{
    delete m_recCommand;
    delete m_runCommand;
}

QObject *PluginKateKeyboardMacro::createView(KTextEditor::MainWindow *)
{
    return nullptr;
}

bool PluginKateKeyboardMacro::record(KTextEditor::View *)
{
    if (m_recording) {
        // stop recording
        std::cerr << "stop recording" << std::endl;
        m_recording = false;
        m_macro = QStringLiteral("foobar");
        return true; // if success
    }

    std::cerr << "start recording" << std::endl;
    m_recording = true;
    return true;
}

bool PluginKateKeyboardMacro::run(KTextEditor::View *view)
{
    if (m_recording) {
        return false;
    }

    if (m_macro.isEmpty()) {
        return false;
    }

    view->insertText(m_macro);
    return true;
}

bool PluginKateKeyboardMacro::isRecording()
{
    return m_recording;
}

void PluginKateKeyboardMacro::slotRecord()
{
    if (!KTextEditor::Editor::instance()->application()->activeMainWindow()) {
        return;
    }

    KTextEditor::View *view(KTextEditor::Editor::instance()->application()->activeMainWindow()->activeView());
    if (!view) {
        return;
    }

    record(view);
}

void PluginKateKeyboardMacro::slotRun()
{
    if (!KTextEditor::Editor::instance()->application()->activeMainWindow()) {
        return;
    }

    KTextEditor::View *view(KTextEditor::Editor::instance()->application()->activeMainWindow()->activeView());
    if (!view) {
        return;
    }

    run(view);
}

// BEGIN commands

PluginKateKeyboardMacroRecordCommand::PluginKateKeyboardMacroRecordCommand(PluginKateKeyboardMacro *plugin)
    : KTextEditor::Command(QStringList() << QStringLiteral("recmac"), plugin)
    , m_plugin(plugin)
{
}

bool PluginKateKeyboardMacroRecordCommand::exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range &)
{
    if (m_plugin->isRecording()) {
        // remove from the recording the call to this command…
    }
    bool success = m_plugin->record(view);
    if (!success) {
        // display fail in toolview
    }
    return true;
}

bool PluginKateKeyboardMacroRecordCommand::help(KTextEditor::View *, const QString &, QString &msg)
{
    msg = i18n(
        "<qt><p>Usage: <code>recmac</code></p>"
        "<p>Start/stop recording a keyboard macro.</p></qt>");
    return true;
}

PluginKateKeyboardMacroRunCommand::PluginKateKeyboardMacroRunCommand(PluginKateKeyboardMacro *plugin)
    : KTextEditor::Command(QStringList() << QStringLiteral("runmac"), plugin)
    , m_plugin(plugin)
{
}

bool PluginKateKeyboardMacroRunCommand::exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range &)
{
    bool success = m_plugin->run(view);
    if (!success) {
        // display fail in toolview
    }
    return true;
}

bool PluginKateKeyboardMacroRunCommand::help(KTextEditor::View *, const QString &, QString &msg)
{
    msg = i18n(
        "<qt><p>Usage: <code>runmac</code></p>"
        "<p>Run recorded keyboard macro.</p></qt>");
    return true;
}

// END

PluginViewKateKeyboardMacro::PluginViewKateKeyboardMacro(PluginKateKeyboardMacro *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(mainwindow)
    , m_mainWindow(mainwindow)
{
    // setup right xml gui data
    KXMLGUIClient::setComponentName(QStringLiteral("keyboardmacro"), i18n("Keyboard Macro"));
    setXMLFile(QStringLiteral("ui.rc"));

    // create record action
    QAction *rec = actionCollection()->addAction(QStringLiteral("record_macro"));
    rec->setText(i18n("&Record Macro..."));
    actionCollection()->setDefaultShortcut(rec, Qt::CTRL | Qt::SHIFT | Qt::Key_K);
    connect(rec, &QAction::triggered, plugin, &PluginKateKeyboardMacro::slotRecord);

    // create run action
    QAction *exec = actionCollection()->addAction(QStringLiteral("run_macro"));
    exec->setText(i18n("&Run Macro"));
    actionCollection()->setDefaultShortcut(exec, Qt::CTRL | Qt::ALT | Qt::Key_K);
    connect(exec, &QAction::triggered, plugin, &PluginKateKeyboardMacro::slotRun);

    // register us at the UI
    mainwindow->guiFactory()->addClient(this);
}

PluginViewKateKeyboardMacro::~PluginViewKateKeyboardMacro()
{
    // remove us from the UI again
    m_mainWindow->guiFactory()->removeClient(this);
}

// required for KeyboardMacroPluginFactory vtable
#include "plugin_katekeyboardmacro.moc"
