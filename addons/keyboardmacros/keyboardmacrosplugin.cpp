/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "keyboardmacrosplugin.h"
#include "keyboardmacrosplugincommands.h"
#include "keyboardmacrospluginview.h"
#include "keycombination.h"
#include "ktexteditor_utils.h"
#include "macro.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QLoggingCategory>
#include <QObject>
#include <QPointer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>

#include <KLocalizedString>
#include <KPluginFactory>

#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

Q_LOGGING_CATEGORY(KM_DBG, "kate.plugin.keyboardmacros", QtWarningMsg)

K_PLUGIN_FACTORY_WITH_JSON(KeyboardMacrosPluginFactory, "keyboardmacrosplugin.json", registerPlugin<KeyboardMacrosPlugin>();)

// BEGIN Plugin creation and destruction

KeyboardMacrosPlugin::KeyboardMacrosPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
    m_commands = new KeyboardMacrosPluginCommands(this);
    m_storage = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kate/keyboardmacros.json");
}

KeyboardMacrosPlugin::~KeyboardMacrosPlugin()
{
    saveNamedMacros();
    delete m_commands;
}

QObject *KeyboardMacrosPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    // avoid reloading macros from storage when creating a new mainWindow from an existing instance
    if (!m_namedMacrosLoaded) {
        loadNamedMacros();
        m_namedMacrosLoaded = true;
    }
    QPointer<KeyboardMacrosPluginView> pluginView = new KeyboardMacrosPluginView(this, mainWindow);
    m_pluginViews.append(pluginView);
    return pluginView;
}

void KeyboardMacrosPlugin::loadNamedMacros()
{
    QFile storage(m_storage);
    if (!storage.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sendMessage(i18n("Could not open file '%1'.", m_storage), false);
        return;
    }
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(storage.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        sendMessage(i18n("Malformed JSON file '%1': %2", m_storage, parseError.errorString()), true);
    }
    QJsonObject json = jsonDoc.object();
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        // don't load macros we have wiped during this session
        if (!m_wipedMacros.contains(it.key())) {
            auto maybeMacro = Macro::fromJson(it.value());
            if (!maybeMacro.second) {
                sendMessage(i18n("Could not load '%1': malformed macro; wiping it.", it.key()), false);
                m_wipedMacros.insert(it.key());
                continue;
            }
            m_namedMacros.insert(it.key(), maybeMacro.first);
        }
    }
    storage.close();
}

void KeyboardMacrosPlugin::saveNamedMacros()
{
    // first keep a copy of the named macros of our instance
    QMap<QString, Macro> ourNamedMacros;
    ourNamedMacros.swap(m_namedMacros);
    // then reload from storage in case another instance saved macros since we first loaded ours from storage
    loadNamedMacros();
    // then insert all of our macros, prioritizing ours in case of name conflict since we are the most recent save
    m_namedMacros.insert(ourNamedMacros);
    // and now save named macros
    QSaveFile storage(m_storage);
    if (!storage.open(QIODevice::WriteOnly | QIODevice::Text)) {
        sendMessage(i18n("Could not open file '%1'.", m_storage), false);
        return;
    }
    QJsonObject json;
    for (const auto &[name, macro] : m_namedMacros.toStdMap()) {
        json.insert(name, macro.toJson());
    }
    storage.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    storage.commit();
}

// END

// BEGIN GUI feedback helpers

void KeyboardMacrosPlugin::sendMessage(const QString &text, bool error)
{
    QVariantMap genericMessage;
    genericMessage.insert(QStringLiteral("type"), error ? QStringLiteral("Error") : QStringLiteral("Info"));
    genericMessage.insert(QStringLiteral("category"), i18n("Keyboard Macros"));
    genericMessage.insert(QStringLiteral("categoryIcon"), QIcon::fromTheme(QStringLiteral("input-keyboard")));
    genericMessage.insert(QStringLiteral("text"), text);
    Utils::showMessage(genericMessage, KTextEditor::Editor::instance()->application()->activeMainWindow());
}

void KeyboardMacrosPlugin::displayMessage(const QString &text, KTextEditor::Message::MessageType type)
{
    KTextEditor::View *view = KTextEditor::Editor::instance()->application()->activeMainWindow()->activeView();
    if (!view) {
        return;
    }
    QPointer<KTextEditor::Message> msg = new KTextEditor::Message(i18n("<b>Keyboard Macros:</b> %1", text), type);
    msg->setIcon(QIcon::fromTheme(QStringLiteral("input-keyboard")));
    msg->setWordWrap(true);
    msg->setPosition(KTextEditor::Message::BottomInView);
    msg->setAutoHide(1500);
    msg->setAutoHideMode(KTextEditor::Message::Immediate);
    msg->setView(view);
    view->document()->postMessage(msg);
}

// END

// BEGIN Events filter and focus management helpers

bool KeyboardMacrosPlugin::eventFilter(QObject *obj, QEvent *event)
{
    // we only spy on keyboard events so we only need to check ShortcutOverride and return false
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
        // we don't want to record the shortcut for recording or playing (avoid infinite loop and stack overflow)
        if (m_recordActionShortcut.matches(QKeySequence(keyEvent->key() | keyEvent->modifiers())) == QKeySequence::ExactMatch
            || m_playActionShortcut.matches(QKeySequence(keyEvent->key() | keyEvent->modifiers())) == QKeySequence::ExactMatch) {
            return false;
        }
        // otherwise we add the keyboard event to the macro
        KeyCombination kc(keyEvent);
        qDebug(KM_DBG) << "key combination:" << kc;
        m_tape.append(kc);
        return false;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

void KeyboardMacrosPlugin::focusObjectChanged(QObject *focusObject)
{
    qDebug(KM_DBG) << "focusObjectChanged:" << focusObject;
    QPointer<QWidget> focusWidget = qobject_cast<QWidget *>(focusObject);
    if (focusWidget.isNull()) {
        return;
    }
    // update which widget we filter events from when the focus has changed
    if (!m_focusWidget.isNull()) {
        m_focusWidget->removeEventFilter(this);
    }
    m_focusWidget = focusWidget;
    m_focusWidget->installEventFilter(this);
}

void KeyboardMacrosPlugin::applicationStateChanged(Qt::ApplicationState state)
{
    qDebug(KM_DBG) << "applicationStateChanged:" << state;
    // somehow keeping our event filter on while the app is out of focus made Kate crash, we fix that here
    switch (state) {
    case Qt::ApplicationSuspended:
    case Qt::ApplicationHidden:
    case Qt::ApplicationInactive:
        if (!m_focusWidget.isNull()) {
            m_focusWidget->removeEventFilter(this);
        }
        break;
    case Qt::ApplicationActive:
        break;
    }
}

// END

// BEGIN Core functions

void KeyboardMacrosPlugin::record()
{
    // start recording
    qDebug(KM_DBG) << "start recording";
    // retrieve current record and play shortcuts
    m_recordActionShortcut = m_pluginViews.first()->recordActionShortcut();
    m_playActionShortcut = m_pluginViews.first()->playActionShortcut();
    // install our spy on currently focused widget
    m_focusWidget = qApp->focusWidget();
    m_focusWidget->installEventFilter(this);
    // update recording status
    m_recording = true;
    // update GUI
    for (auto &pluginView : m_pluginViews) {
        pluginView->recordingOn();
    }
    // connect focus change events
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &KeyboardMacrosPlugin::applicationStateChanged);
    connect(qApp, &QGuiApplication::focusObjectChanged, this, &KeyboardMacrosPlugin::focusObjectChanged);
    // display feedback
    displayMessage(i18n("Recording…"), KTextEditor::Message::Information);
}

void KeyboardMacrosPlugin::cancel()
{
    stop(false);
}

void KeyboardMacrosPlugin::stop(bool save)
{
    // stop recording
    qDebug(KM_DBG) << (save ? "end" : "cancel") << "recording";
    // uninstall our spy
    m_focusWidget->removeEventFilter(this);
    // update recording status
    m_recording = false;
    if (save) { // end recording
        // clear current macro
        m_macro.clear();
        // replace it with the tape
        m_macro.swap(m_tape);
        // clear tape
        m_tape.clear();
    } else { // cancel recording
        // clear tape
        m_tape.clear();
    }
    // update GUI
    for (auto &pluginView : m_pluginViews) {
        pluginView->recordingOff();
        pluginView->macroLoaded(!m_macro.isEmpty());
    }
    // disconnect focus change events
    disconnect(qApp, &QGuiApplication::applicationStateChanged, this, &KeyboardMacrosPlugin::applicationStateChanged);
    disconnect(qApp, &QGuiApplication::focusObjectChanged, this, &KeyboardMacrosPlugin::focusObjectChanged);
    // display feedback
    displayMessage(i18n("Recording %1", (save ? i18n("ended") : i18n("canceled"))), KTextEditor::Message::Positive);
}

bool KeyboardMacrosPlugin::play(const QString &name)
{
    Macro macro;
    if (!name.isEmpty() && m_namedMacros.contains(name)) {
        macro = m_namedMacros.value(name);
        qDebug(KM_DBG) << "playing macro:" << name;
    } else if (name.isEmpty() && !m_macro.isEmpty()) {
        macro = m_macro;
        qDebug(KM_DBG) << "playing macro!";
    } else {
        return false;
    }
    for (const auto &keyCombination : macro) {
        // send key press
        QKeyEvent keyPress = keyCombination.keyPress();
        qApp->sendEvent(qApp->focusWidget(), &keyPress);
        // send key release
        QKeyEvent keyRelease = keyCombination.keyRelease();
        qApp->sendEvent(qApp->focusWidget(), &keyRelease);
        // process events immediately if a shortcut may have been triggered
        if (!keyCombination.isVisibleInput()) {
            qApp->processEvents(QEventLoop::AllEvents);
        }
    }
    return true;
}

bool KeyboardMacrosPlugin::save(const QString &name)
{
    // we don't need to save macros that do nothing
    if (m_macro.isEmpty()) {
        return false;
    }
    qDebug(KM_DBG) << "saving macro:" << name;
    m_namedMacros.insert(name, m_macro);
    // update GUI:
    for (auto &pluginView : m_pluginViews) {
        pluginView->addNamedMacro(name, m_macro.toString());
    }
    // display feedback
    displayMessage(i18n("Saved '%1'", name), KTextEditor::Message::Positive);
    return true;
}

bool KeyboardMacrosPlugin::load(const QString &name)
{
    if (!m_namedMacros.contains(name)) {
        return false;
    }
    qDebug(KM_DBG) << "loading macro:" << name;
    // clear current macro
    m_macro.clear();
    // load named macro
    m_macro = m_namedMacros.value(name);
    // update GUI
    for (auto &pluginView : m_pluginViews) {
        pluginView->macroLoaded(true);
    }
    // display feedback
    displayMessage(i18n("Loaded '%1'", name), KTextEditor::Message::Positive);
    return true;
}

bool KeyboardMacrosPlugin::wipe(const QString &name)
{
    if (!m_namedMacros.contains(name)) {
        return false;
    }
    qDebug(KM_DBG) << "wiping macro:" << name;
    m_namedMacros.remove(name);
    m_wipedMacros.insert(name);
    // update GUI
    for (auto &pluginView : m_pluginViews) {
        pluginView->removeNamedMacro(name);
    }
    // display feedback
    displayMessage(i18n("Wiped '%1'", name), KTextEditor::Message::Positive);
    return true;
}

// END

// required for KeyboardMacrosPluginFactory vtable
#include "keyboardmacrosplugin.moc"
