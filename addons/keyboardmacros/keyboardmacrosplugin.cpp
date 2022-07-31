/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "keyboardmacrosplugin.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtAlgorithms>

#include <KTextEditor/Editor>
#include <KTextEditor/Message>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(KeyboardMacrosPluginFactory, "keyboardmacrosplugin.json", registerPlugin<KeyboardMacrosPlugin>();)

KeyboardMacrosPlugin::KeyboardMacrosPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

KeyboardMacrosPlugin::~KeyboardMacrosPlugin()
{
    qDeleteAll(m_tape.begin(), m_tape.end());
    m_tape.clear();
    qDeleteAll(m_macro.begin(), m_macro.end());
    m_macro.clear();
}

QObject *KeyboardMacrosPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
    return new KeyboardMacrosPluginView(this, mainWindow);
}

void KeyboardMacrosPlugin::sendMessage(const QString &text, bool error)
{
    QVariantMap genericMessage;
    genericMessage.insert(QStringLiteral("type"), error ? QStringLiteral("Error") : QStringLiteral("Info"));
    genericMessage.insert(QStringLiteral("category"), i18n("Keyboard Macros"));
    genericMessage.insert(QStringLiteral("categoryIcon"), QIcon::fromTheme(QStringLiteral("input-keyboard")));
    genericMessage.insert(QStringLiteral("text"), text);
    Q_EMIT message(genericMessage);
}

bool KeyboardMacrosPlugin::eventFilter(QObject *obj, QEvent *event)
{
    // Update which widget we filter events from if the focus has changed
    m_focusWidget->removeEventFilter(this);
    m_focusWidget = qApp->focusWidget();
    m_focusWidget->installEventFilter(this);

    // We only spy on keyboard events so we only need to check ShortcutOverride and return false
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        // if only modifiers are pressed, we don't care
        switch (keyEvent->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_Meta:
        case Qt::Key_AltGr:
            return false;
        }
        // we don't want to record the shortcut for recording
        if (m_recordAction->shortcut().matches(QKeySequence(keyEvent->key() | keyEvent->modifiers())) == QKeySequence::ExactMatch) {
            return false;
        }
        // otherwise we add the keyboard event to the macro
        m_tape.append(new QKeyEvent(QEvent::KeyPress, keyEvent->key(), keyEvent->modifiers(), keyEvent->text()));
        m_tape.append(new QKeyEvent(QEvent::KeyRelease, keyEvent->key(), keyEvent->modifiers(), keyEvent->text()));
        return false;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

void KeyboardMacrosPlugin::record()
{
    // start recording
    qDebug("[KeyboardMacrosPlugin] start recording");
    m_focusWidget = qApp->focusWidget();
    m_focusWidget->installEventFilter(this);
    m_recording = true;
    m_recordAction->setText(i18n("End Macro &Recording"));
    m_cancelAction->setEnabled(true);
    // connect applicationStateChanged
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &KeyboardMacrosPlugin::applicationStateChanged);
}

void KeyboardMacrosPlugin::stop(bool save)
{
    // disconnect applicationStateChanged
    disconnect(qApp, &QGuiApplication::applicationStateChanged, this, &KeyboardMacrosPlugin::applicationStateChanged);
    // stop recording
    qDebug("[KeyboardMacrosPlugin] %s recording", save ? "end" : "cancel");
    m_focusWidget->removeEventFilter(this);
    m_recording = false;
    if (save) {
        // delete current macro
        qDeleteAll(m_macro.begin(), m_macro.end());
        m_macro.clear();
        // replace it with the tape
        m_macro.swap(m_tape);
        // clear tape
        m_tape.clear();
        m_playAction->setEnabled(!m_macro.isEmpty());
    } else { // cancel
        // delete tape
        qDeleteAll(m_tape.begin(), m_tape.end());
        m_tape.clear();
    }
    m_recordAction->setText(i18n("&Record Macro..."));
    m_cancelAction->setEnabled(false);
}

void KeyboardMacrosPlugin::cancel()
{
    stop(false);
}

bool KeyboardMacrosPlugin::play()
{
    if (m_macro.isEmpty()) {
        return false;
    }
    Macro::Iterator it;
    for (it = m_macro.begin(); it != m_macro.end(); it++) {
        QKeyEvent *keyEvent = *it;
        keyEvent->setAccepted(false);
        qApp->sendEvent(qApp->focusWidget(), keyEvent);
    }
    return true;
}

void KeyboardMacrosPlugin::applicationStateChanged(Qt::ApplicationState state)
{
    switch (state) {
    case Qt::ApplicationSuspended:
        sendMessage(i18n("Application suspended, aborting record."), true);
        cancel();
        break;
    case Qt::ApplicationHidden:
    case Qt::ApplicationInactive:
        // TODO: would be nice to be able to pause recording
        sendMessage(i18n("Application lost focus, aborting record."), true);
        cancel();
        break;
    case Qt::ApplicationActive:
        // TODO: and resume it here
        break;
    }
}

void KeyboardMacrosPlugin::slotRecord()
{
    if (m_recording) {
        stop(true);
    } else {
        record();
    }
}

void KeyboardMacrosPlugin::slotPlay()
{
    if (m_recording) {
        stop(true);
    }
    if (!play()) {
        sendMessage(i18n("Macro is empty."), false);
    }
}

void KeyboardMacrosPlugin::slotCancel()
{
    if (!m_recording) {
        return;
    }
    cancel();
}

// BEGIN Plugin view to add our actions to the gui

KeyboardMacrosPluginView::KeyboardMacrosPluginView(KeyboardMacrosPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(mainwindow)
    , m_mainWindow(mainwindow)
{
    // setup xml gui
    KXMLGUIClient::setComponentName(QStringLiteral("keyboardmacros"), i18n("Keyboard Macros"));
    setXMLFile(QStringLiteral("ui.rc"));

    // create record action
    QAction *rec = actionCollection()->addAction(QStringLiteral("keyboardmacros_record"));
    rec->setText(i18n("&Record Macro..."));
    actionCollection()->setDefaultShortcut(rec, Qt::CTRL | Qt::SHIFT | Qt::Key_K);
    connect(rec, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotRecord);
    plugin->m_recordAction = rec;

    // create cancel action
    QAction *cancel = actionCollection()->addAction(QStringLiteral("keyboardmacros_cancel"));
    cancel->setText(i18n("&Cancel Macro Recording"));
    cancel->setEnabled(false);
    connect(cancel, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotCancel);
    plugin->m_cancelAction = cancel;

    // create play action
    QAction *play = actionCollection()->addAction(QStringLiteral("keyboardmacros_play"));
    play->setText(i18n("&Play Macro"));
    actionCollection()->setDefaultShortcut(play, Qt::CTRL | Qt::ALT | Qt::Key_K);
    play->setEnabled(false);
    connect(play, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotPlay);
    plugin->m_playAction = play;

    // register our gui elements
    mainwindow->guiFactory()->addClient(this);
}

KeyboardMacrosPluginView::~KeyboardMacrosPluginView()
{
    // remove us from the gui
    m_mainWindow->guiFactory()->removeClient(this);
}

// END

// required for KeyboardMacrosPluginFactory vtable
#include "keyboardmacrosplugin.moc"
