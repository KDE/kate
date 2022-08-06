/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "keyboardmacrosplugin.h"

#include <QAction>
#include <QApplication>
#include <QCompleter>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QIODevice>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include "keycombination.h"
#include "macro.h"

Q_LOGGING_CATEGORY(KM_DBG, "kate.plugin.keyboardmacros", QtWarningMsg)

K_PLUGIN_FACTORY_WITH_JSON(KeyboardMacrosPluginFactory, "keyboardmacrosplugin.json", registerPlugin<KeyboardMacrosPlugin>();)

// BEGIN Plugin creation and destruction

KeyboardMacrosPlugin::KeyboardMacrosPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
    m_commands = new KeyboardMacrosPluginCommands(this);
    m_storage = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kate/keyboardmacros.json");
    loadNamedMacros();
}

KeyboardMacrosPlugin::~KeyboardMacrosPlugin()
{
    saveNamedMacros();
    delete m_commands;
}

QObject *KeyboardMacrosPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
    m_pluginView = new KeyboardMacrosPluginView(this, mainWindow);
    return m_pluginView;
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
            m_namedMacros.insert(it.key(), Macro(it.value()));
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
    QFile storage(m_storage);
    if (!storage.open(QIODevice::WriteOnly | QIODevice::Text)) {
        sendMessage(i18n("Could not open file '%1'.", m_storage), false);
        return;
    }
    QJsonObject json;
    for (const auto &[name, macro] : m_namedMacros.toStdMap()) {
        json.insert(name, macro.toJson());
    }
    storage.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    storage.close();
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
    Q_EMIT message(genericMessage);
}

void KeyboardMacrosPlugin::displayMessage(const QString &text, KTextEditor::Message::MessageType type)
{
    KTextEditor::View *view = m_mainWindow->activeView();
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
        if (m_recordAction->shortcut().matches(QKeySequence(keyEvent->key() | keyEvent->modifiers())) == QKeySequence::ExactMatch
            || m_playAction->shortcut().matches(QKeySequence(keyEvent->key() | keyEvent->modifiers())) == QKeySequence::ExactMatch) {
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
    if (focusWidget == nullptr) {
        return;
    }
    // update which widget we filter events from when the focus has changed
    if (m_focusWidget != nullptr) {
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
        if (m_focusWidget != nullptr) {
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
    // install our spy on currently focused widget
    m_focusWidget = qApp->focusWidget();
    m_focusWidget->installEventFilter(this);
    // update recording status
    m_recording = true;
    // update GUI
    m_recordAction->setText(i18n("End Macro &Recording"));
    m_recordAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
    m_cancelAction->setEnabled(true);
    m_playAction->setEnabled(true);
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
        // update GUI
        m_playAction->setEnabled(!m_macro.isEmpty());
        m_saveAction->setEnabled(!m_macro.isEmpty());
    } else { // cancel recording
        // clear tape
        m_tape.clear();
    }
    // update GUI
    m_recordAction->setText(i18n("&Record Macro..."));
    m_recordAction->setIcon(QIcon::fromTheme(QStringLiteral("media-record")));
    m_cancelAction->setEnabled(false);
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
        // process events
        qApp->processEvents(QEventLoop::AllEvents);
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
    m_pluginView->addNamedMacro(name, m_macro);
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
    m_saveAction->setEnabled(true);
    m_playAction->setEnabled(true);
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
    m_pluginView->removeNamedMacro(name);
    // display feedback
    displayMessage(i18n("Wiped '%1'", name), KTextEditor::Message::Positive);
    return true;
}

// END

// BEGIN Action slots

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
    play();
}

void KeyboardMacrosPlugin::slotCancel()
{
    if (!m_recording) {
        return;
    }
    cancel();
}

void KeyboardMacrosPlugin::slotSave()
{
    if (m_recording) {
        return;
    }
    bool ok;
    QString name = QInputDialog::getText(m_mainWindow->window(),
                                         i18n("Save Macro"),
                                         i18n("Under which name should the current macro be saved?"),
                                         QLineEdit::Normal,
                                         QStringLiteral(""),
                                         &ok);
    if (!ok || name.isEmpty()) {
        return;
    }
    save(name);
}

void KeyboardMacrosPlugin::slotLoadNamed(const QString &name)
{
    if (m_recording) {
        return;
    }
    if (name.isEmpty()) {
        return;
    }
    load(name);
}

void KeyboardMacrosPlugin::slotPlayNamed(const QString &name)
{
    if (m_recording) {
        return;
    }
    if (name.isEmpty()) {
        return;
    }
    play(name);
}

void KeyboardMacrosPlugin::slotWipeNamed(const QString &name)
{
    if (m_recording) {
        return;
    }
    if (QMessageBox::question(m_mainWindow->window(), i18n("Keyboard Macros"), i18n("Wipe the '%1' macro?", name)) != QMessageBox::Yes) {
        return;
    }
    wipe(name);
}

// END

// BEGIN Plugin view to add our actions to the GUI

KeyboardMacrosPluginView::KeyboardMacrosPluginView(KeyboardMacrosPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(mainwindow)
    , m_plugin(plugin)
    , m_mainWindow(mainwindow)
{
    // setup XML GUI
    KXMLGUIClient::setComponentName(QStringLiteral("keyboardmacros"), i18n("Keyboard Macros"));
    setXMLFile(QStringLiteral("ui.rc"));

    // TODO: set an icon for the "Keyboard Macros" menu at the toplevel of this plugin; but how?
    // ??->setIcon(QIcon::fromTheme(QStringLiteral("input-keyboard")));

    // create record action
    QAction *record = actionCollection()->addAction(QStringLiteral("keyboardmacros_record"));
    record->setText(i18n("&Record Macro..."));
    record->setIcon(QIcon::fromTheme(QStringLiteral("media-record")));
    record->setToolTip(i18n("Start/stop recording a macro (i.e., keyboard action sequence)."));
    actionCollection()->setDefaultShortcut(record, QKeySequence(QStringLiteral("Ctrl+Shift+K"), QKeySequence::PortableText));
    connect(record, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotRecord);
    plugin->m_recordAction = record;

    // create cancel action
    QAction *cancel = actionCollection()->addAction(QStringLiteral("keyboardmacros_cancel"));
    cancel->setText(i18n("&Cancel Macro Recording"));
    cancel->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    cancel->setToolTip(i18n("Cancel ongoing recording (and keep the previous macro as the current one)."));
    actionCollection()->setDefaultShortcut(cancel, QKeySequence(QStringLiteral("Ctrl+Alt+Shift+K"), QKeySequence::PortableText));
    cancel->setEnabled(false);
    connect(cancel, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotCancel);
    plugin->m_cancelAction = cancel;

    // create play action
    QAction *play = actionCollection()->addAction(QStringLiteral("keyboardmacros_play"));
    play->setText(i18n("&Play Macro"));
    play->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    play->setToolTip(i18n("Play current macro (i.e., execute the last recorded keyboard action sequence)."));
    actionCollection()->setDefaultShortcut(play, QKeySequence(QStringLiteral("Ctrl+Alt+K"), QKeySequence::PortableText));
    play->setEnabled(false);
    connect(play, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotPlay);
    plugin->m_playAction = play;

    // create save action
    QAction *save = actionCollection()->addAction(QStringLiteral("keyboardmacros_save"));
    save->setText(i18n("&Save Current Macro"));
    save->setIcon(QIcon::fromTheme(QStringLiteral("media-playlist-append")));
    save->setToolTip(i18n("Give a name to the current macro and persistently save it."));
    actionCollection()->setDefaultShortcut(save, QKeySequence(QStringLiteral("Alt+Shift+K"), QKeySequence::PortableText));
    save->setEnabled(false);
    connect(save, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotSave);
    plugin->m_saveAction = save;

    // create load named menu
    m_loadMenu = new KActionMenu(i18n("&Load Named Macro..."), this);
    m_loadMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_load"), m_loadMenu);
    m_loadMenu->setToolTip(i18n("Load a named macro as the current one."));
    m_loadMenu->setEnabled(!plugin->m_namedMacros.isEmpty());

    // create play named menu
    m_playMenu = new KActionMenu(i18n("&Play Named Macro..."), this);
    m_playMenu->setIcon(QIcon::fromTheme(QStringLiteral("auto-type")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_play"), m_playMenu);
    m_playMenu->setToolTip(i18n("Play a named macro without loading it."));
    m_playMenu->setEnabled(!plugin->m_namedMacros.isEmpty());

    // create wipe named menu
    m_wipeMenu = new KActionMenu(i18n("&Wipe Named Macro..."), this);
    m_wipeMenu->setIcon(QIcon::fromTheme(QStringLiteral("delete")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_wipe"), m_wipeMenu);
    m_wipeMenu->setToolTip(i18n("Wipe a named macro."));
    m_wipeMenu->setEnabled(!plugin->m_namedMacros.isEmpty());

    // add named macros to our menus
    for (const auto &[name, macro] : plugin->m_namedMacros.toStdMap()) {
        addNamedMacro(name, macro);
    }

    // add Keyboard Macros actions to the GUI
    mainwindow->guiFactory()->addClient(this);
}

KeyboardMacrosPluginView::~KeyboardMacrosPluginView()
{
    // remove Keyboard Macros actions from the GUI
    m_mainWindow->guiFactory()->removeClient(this);
}

void KeyboardMacrosPluginView::addNamedMacro(const QString &name, const Macro &macro)
{
    QAction *action;
    QString definition = name + QStringLiteral(": ") + macro.toString();

    // add load action
    action = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_load_") + name);
    action->setText(QStringLiteral("Load ") + definition);
    action->setToolTip(i18n("Load the '%1' macro as the current one.", name));
    action->setEnabled(true);
    connect(action, &QAction::triggered, m_plugin, [this, name] {
        m_plugin->slotLoadNamed(name);
    });
    m_loadMenu->addAction(action);
    // remember load action pointer
    m_namedMacrosLoadActions.insert(name, action);
    // update load menu state
    m_loadMenu->setEnabled(true);

    // add play action
    action = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_play_") + name);
    action->setText(QStringLiteral("Play ") + definition);
    action->setToolTip(i18n("Play the '%1' macro without loading it.", name));
    action->setEnabled(true);
    connect(action, &QAction::triggered, m_plugin, [this, name] {
        m_plugin->slotPlayNamed(name);
    });
    m_playMenu->addAction(action);
    // remember play action pointer
    m_namedMacrosPlayActions.insert(name, action);
    // update play menu state
    m_playMenu->setEnabled(true);

    // add wipe action
    action = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_wipe_") + name);
    action->setText(QStringLiteral("Wipe ") + definition);
    action->setToolTip(i18n("Wipe the '%1' macro.", name));
    action->setEnabled(true);
    connect(action, &QAction::triggered, m_plugin, [this, name] {
        m_plugin->slotWipeNamed(name);
    });
    m_wipeMenu->addAction(action);
    // remember wipe action pointer
    m_namedMacrosWipeActions.insert(name, action);
    // update wipe menu state
    m_wipeMenu->setEnabled(true);
}

void KeyboardMacrosPluginView::removeNamedMacro(const QString &name)
{
    QAction *action;

    // remove load action
    action = m_namedMacrosLoadActions.value(name);
    m_loadMenu->removeAction(action);
    actionCollection()->removeAction(action);
    // forget load action pointer
    m_namedMacrosLoadActions.remove(name);
    // update load menu state
    m_loadMenu->setEnabled(!m_namedMacrosLoadActions.isEmpty());

    // remove play action
    action = m_namedMacrosPlayActions.value(name);
    m_playMenu->removeAction(action);
    actionCollection()->removeAction(action);
    // forget play action pointer
    m_namedMacrosPlayActions.remove(name);
    // update play menu state
    m_playMenu->setEnabled(!m_namedMacrosPlayActions.isEmpty());

    // remove wipe action
    action = m_namedMacrosWipeActions.value(name);
    m_wipeMenu->removeAction(action);
    actionCollection()->removeAction(action);
    // forget wipe action pointer
    m_namedMacrosWipeActions.remove(name);
    // update wipe menu state
    m_wipeMenu->setEnabled(!m_namedMacrosWipeActions.isEmpty());
}

// END

// BEGIN Plugin commands to manage named keyboard macros

KeyboardMacrosPluginCommands::KeyboardMacrosPluginCommands(KeyboardMacrosPlugin *plugin)
    : KTextEditor::Command(QStringList() << QStringLiteral("kmsave") << QStringLiteral("kmload") << QStringLiteral("kmplay") << QStringLiteral("kmwipe"),
                           plugin)
    , m_plugin(plugin)
{
}

bool KeyboardMacrosPluginCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &)
{
    const QStringList &actionAndName = cmd.split(QRegularExpression(QStringLiteral("\\s+")));
    const QString &action = actionAndName.at(0);
    // kmplay can take either zero or one argument, all other commands require exactly one
    if (actionAndName.length() > 2 || (action != QStringLiteral("kmplay") && actionAndName.length() != 2)) {
        msg = i18n("Usage: %1 <name>.", action);
        return false;
    }
    if (action == QStringLiteral("kmplay")) {
        // set focus on the view otherwise the macro is executed in the command line
        view->setFocus();
        if (actionAndName.length() == 1) {
            // no argument: play the current macro
            m_plugin->play();
            return true;
        } else {
            // otherwise play the given macro
            const QString &name = actionAndName.at(1);
            if (!m_plugin->play(name)) {
                msg = i18n("No keyboard macro named '%1' found.", name);
                return false;
            }
            return true;
        }
    }
    const QString &name = actionAndName.at(1);
    if (action == QStringLiteral("kmsave")) {
        if (!m_plugin->save(name)) {
            msg = i18n("Cannot save empty keyboard macro.");
            return false;
        }
        return true;
    } else if (action == QStringLiteral("kmload")) {
        if (!m_plugin->load(name)) {
            msg = i18n("No keyboard macro named '%1' found.", name);
            return false;
        }
        return true;
    } else if (action == QStringLiteral("kmwipe")) {
        if (!m_plugin->wipe(name)) {
            msg = i18n("No keyboard macro named '%1' found.", name);
            return false;
        }
        return true;
    }
    return false;
}

bool KeyboardMacrosPluginCommands::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
    QString macros;
    if (!m_plugin->m_namedMacros.keys().isEmpty()) {
        macros = QStringLiteral("<p><b>Named macros:</b> ") + QStringList(m_plugin->m_namedMacros.keys()).join(QStringLiteral(", ")) + QStringLiteral(".</p>");
    }
    if (cmd == QStringLiteral("kmsave")) {
        msg = i18n("<qt><p>Usage: <code>kmsave &lt;name&gt;</code></p><p>Save current keyboard macro as <code>&lt;name&gt;</code>.</p>%1</qt>", macros);
        return true;
    } else if (cmd == QStringLiteral("kmload")) {
        msg = i18n("<qt><p>Usage: <code>kmload &lt;name&gt;</code></p><p>Load saved keyboard macro <code>&lt;name&gt;</code> as current macro.</p>%1</qt>",
                   macros);
        return true;
    } else if (cmd == QStringLiteral("kmplay")) {
        msg = i18n("<qt><p>Usage: <code>kmplay &lt;name&gt;</code></p><p>Play saved keyboard macro <code>&lt;name&gt;</code> without loading it.</p>%1</qt>",
                   macros);
        return true;
    } else if (cmd == QStringLiteral("kmwipe")) {
        msg = i18n("<qt><p>Usage: <code>kmwipe &lt;name&gt;</code></p><p>Wipe saved keyboard macro <code>&lt;name&gt;</code>.</p>%1</qt>", macros);
        return true;
    }
    return false;
}

// END

// required for KeyboardMacrosPluginFactory vtable
#include "keyboardmacrosplugin.moc"
