/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "plugin_katekeyboardmacro.h"

#include <QAction>
#include <QCoreApplication>
#include <QString>

#include <KTextEditor/Editor>
#include <KTextEditor/Message>

#include <KLocalizedString>
#include <KActionCollection>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <iostream>
#include <qevent.h>

K_PLUGIN_FACTORY_WITH_JSON(KeyboardMacroPluginFactory, "keyboardmacroplugin.json", registerPlugin<PluginKateKeyboardMacro>();)

PluginKateKeyboardMacro::PluginKateKeyboardMacro(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
    // register "recmac" and "runmac" commands
    m_recCommand = new PluginKateKeyboardMacroRecordCommand(this);
    m_runCommand = new PluginKateKeyboardMacroRunCommand(this);
}

PluginKateKeyboardMacro::~PluginKateKeyboardMacro()
{
    delete m_recCommand;
    delete m_runCommand;
}

QObject *PluginKateKeyboardMacro::createView(KTextEditor::MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
    return new PluginViewKateKeyboardMacro(this, mainWindow);
}

// https://doc.qt.io/qt-6/eventsandfilters.html
// https://doc.qt.io/qt-6/qobject.html#installEventFilter
// https://stackoverflow.com/questions/41631011/my-qt-eventfilter-doesnt-stop-events-as-it-should

// file:///usr/share/qt5/doc/qtcore/qobject.html#installEventFilter
// file:///usr/share/qt5/doc/qtcore/qcoreapplication.html#sendEvent
// also see postEvent, sendPostedEvents, etc

bool PluginKateKeyboardMacro::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = new QKeyEvent(*static_cast<QKeyEvent *>(event));
        m_keyEvents.enqueue(keyEvent);
        qDebug("Capture key press: %s", keyEvent->text().toUtf8().data());
        return true;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

bool PluginKateKeyboardMacro::record(KTextEditor::View *)
{
    if (m_recording) { // end recording
        // KTextEditor::Editor::instance()->application()->activeMainWindow()->window()->removeEventFilter(this);
        QCoreApplication::instance()->removeEventFilter(this);
        std::cerr << "stop recording" << std::endl;
        m_recording = false;
        return true; // if success
    }

    // start recording
    std::cerr << "start recording" << std::endl;
    QCoreApplication::instance()->installEventFilter(this);
    m_recording = true;
    return true;
}

bool PluginKateKeyboardMacro::run(KTextEditor::View *view)
{
    if (m_recording) {
        // end recording before running macro
        record(view);
    }

    while (!m_keyEvents.isEmpty()) {
        QKeyEvent *keyEvent = m_keyEvents.dequeue();
        qDebug("Emit key press: %s", keyEvent->text().toUtf8().data());
        QCoreApplication::sendEvent(QCoreApplication::instance(), keyEvent);
        delete keyEvent;
    }

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

// BEGIN Plugin view to add our actions to the gui

PluginViewKateKeyboardMacro::PluginViewKateKeyboardMacro(PluginKateKeyboardMacro *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(mainwindow)
    , m_mainWindow(mainwindow)
{
    // setup xml gui
    KXMLGUIClient::setComponentName(QStringLiteral("keyboardmacro"), i18n("Keyboard Macro"));
    setXMLFile(QStringLiteral("ui.rc"));

    // create record action
    QAction *rec = actionCollection()->addAction(QStringLiteral("record_macro"));
    rec->setText(i18n("&Record Macro..."));
    actionCollection()->setDefaultShortcut(rec, Qt::CTRL | Qt::SHIFT | Qt::Key_K);
    connect(rec, &QAction::triggered, plugin, &PluginKateKeyboardMacro::slotRecord);

    // create run action
    QAction *run = actionCollection()->addAction(QStringLiteral("run_macro"));
    run->setText(i18n("&Run Macro"));
    actionCollection()->setDefaultShortcut(run, Qt::CTRL | Qt::ALT | Qt::Key_K);
    connect(run, &QAction::triggered, plugin, &PluginKateKeyboardMacro::slotRun);

    // register our gui elements
    mainwindow->guiFactory()->addClient(this);
}

PluginViewKateKeyboardMacro::~PluginViewKateKeyboardMacro()
{
    // remove us from the gui
    m_mainWindow->guiFactory()->removeClient(this);
}

// END

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
    if (!m_plugin->record(view)) {
        // display fail in toolview
    }
    return true;
}

bool PluginKateKeyboardMacroRecordCommand::help(KTextEditor::View *, const QString &, QString &msg)
{
    msg = i18n("<qt><p>Usage: <code>recmac</code></p><p>Start/stop recording a keyboard macro.</p></qt>");
    return true;
}

PluginKateKeyboardMacroRunCommand::PluginKateKeyboardMacroRunCommand(PluginKateKeyboardMacro *plugin)
    : KTextEditor::Command(QStringList() << QStringLiteral("runmac"), plugin)
    , m_plugin(plugin)
{
}

bool PluginKateKeyboardMacroRunCommand::exec(KTextEditor::View *view, const QString &, QString &, const KTextEditor::Range &)
{
    if (!m_plugin->run(view)) {
        // display fail in toolview
    }
    return true;
}

bool PluginKateKeyboardMacroRunCommand::help(KTextEditor::View *, const QString &, QString &msg)
{
    msg = i18n("<qt><p>Usage: <code>runmac</code></p><p>Run recorded keyboard macro.</p></qt>");
    return true;
}

// END

// required for KeyboardMacroPluginFactory vtable
#include "plugin_katekeyboardmacro.moc"
