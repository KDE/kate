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
#include <QLockFile>
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
#include <KStringHandler>
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
    m_storageLock = new QLockFile(m_storage + QStringLiteral(".lock"));
}

KeyboardMacrosPlugin::~KeyboardMacrosPlugin()
{
    saveNamedMacros();
    delete m_storageLock;
    delete m_commands;
}

QObject *KeyboardMacrosPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    // avoid reloading macros from storage when creating a new mainWindow from an existing instance
    static bool namedMacrosLoaded = false;
    if (!namedMacrosLoaded) {
        loadNamedMacros();
        namedMacrosLoaded = true;
    }
    QPointer<KeyboardMacrosPluginView> pluginView = new KeyboardMacrosPluginView(this, mainWindow);
    m_pluginViews.append(pluginView);
    return pluginView;
}

void KeyboardMacrosPlugin::clearPluginViews()
{
    // when Kate depends on Qt6, we can directly use QList::removeIf
    for (auto &pluginView : m_pluginViews) {
        if (pluginView.isNull()) {
            m_pluginViews.removeOne(pluginView);
        }
    }
}

void KeyboardMacrosPlugin::loadNamedMacros(bool locked)
{
    if (!locked && !m_storageLock->tryLock()) {
        sendMessage(i18n("Could not acquire macros storage lock; abort loading macros."), true);
        return;
    }
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
    if (!locked) {
        m_storageLock->unlock();
    }
}

void KeyboardMacrosPlugin::saveNamedMacros()
{
    if (!m_storageLock->tryLock()) {
        sendMessage(i18n("Could not acquire macros storage lock; abort saving macros."), true);
        return;
    }
    // first keep a copy of the named macros of our instance
    QMap<QString, Macro> ourNamedMacros;
    ourNamedMacros.swap(m_namedMacros);
    // then reload from storage in case another instance saved macros since we first loaded ours from storage
    loadNamedMacros(true);
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
    m_storageLock->unlock();
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
    clearPluginViews();
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
    // call clearPluginViews once
    clearPluginViews();
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
        for (auto &pluginView : m_pluginViews) {
            pluginView->macroLoaded(!m_macro.isEmpty());
        }
    } else { // cancel recording
        // clear tape
        m_tape.clear();
    }
    // update GUI
    for (auto &pluginView : m_pluginViews) {
        pluginView->recordingOff();
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
    clearPluginViews();
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
    clearPluginViews();
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
    clearPluginViews();
    for (auto &pluginView : m_pluginViews) {
        pluginView->removeNamedMacro(name);
    }
    // display feedback
    displayMessage(i18n("Wiped '%1'", name), KTextEditor::Message::Positive);
    return true;
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

    KActionMenu *menu = new KActionMenu(i18n("&Keyboard Macros"), this);
    menu->setIcon(QIcon::fromTheme(QStringLiteral("input-keyboard")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros"), menu);
    menu->setToolTip(i18n("Record and play keyboard macros."));
    menu->setEnabled(true);

    // create record action
    m_recordAction = actionCollection()->addAction(QStringLiteral("keyboardmacros_record"));
    m_recordAction->setText(i18n("&Record Macro..."));
    m_recordAction->setIcon(QIcon::fromTheme(QStringLiteral("media-record")));
    m_recordAction->setToolTip(i18n("Start/stop recording a macro (i.e., keyboard action sequence)."));
    actionCollection()->setDefaultShortcut(m_recordAction, QKeySequence(QStringLiteral("Ctrl+Shift+K"), QKeySequence::PortableText));
    connect(m_recordAction, &QAction::triggered, plugin, [this] {
        slotRecord();
    });
    menu->addAction(m_recordAction);

    // create cancel action
    m_cancelAction = actionCollection()->addAction(QStringLiteral("keyboardmacros_cancel"));
    m_cancelAction->setText(i18n("&Cancel Macro Recording"));
    m_cancelAction->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_cancelAction->setToolTip(i18n("Cancel ongoing recording (and keep the previous macro as the current one)."));
    actionCollection()->setDefaultShortcut(m_cancelAction, QKeySequence(QStringLiteral("Ctrl+Alt+Shift+K"), QKeySequence::PortableText));
    m_cancelAction->setEnabled(false);
    connect(m_cancelAction, &QAction::triggered, plugin, [this] {
        slotCancel();
    });
    menu->addAction(m_cancelAction);

    // create play action
    m_playAction = actionCollection()->addAction(QStringLiteral("keyboardmacros_play"));
    m_playAction->setText(i18n("&Play Macro"));
    m_playAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    m_playAction->setToolTip(i18n("Play current macro (i.e., execute the last recorded keyboard action sequence)."));
    actionCollection()->setDefaultShortcut(m_playAction, QKeySequence(QStringLiteral("Ctrl+Alt+K"), QKeySequence::PortableText));
    m_playAction->setEnabled(false);
    connect(m_playAction, &QAction::triggered, plugin, [this] {
        slotPlay();
    });
    menu->addAction(m_playAction);

    // create save action
    m_saveAction = actionCollection()->addAction(QStringLiteral("keyboardmacros_save"));
    m_saveAction->setText(i18n("&Save Current Macro"));
    m_saveAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playlist-append")));
    m_saveAction->setToolTip(i18n("Give a name to the current macro and persistently save it."));
    actionCollection()->setDefaultShortcut(m_saveAction, QKeySequence(QStringLiteral("Alt+Shift+K"), QKeySequence::PortableText));
    m_saveAction->setEnabled(false);
    connect(m_saveAction, &QAction::triggered, plugin, [this] {
        slotSave();
    });
    menu->addAction(m_saveAction);

    // add separator
    menu->addSeparator();

    // create load named menu
    m_loadMenu = new KActionMenu(i18n("&Load Named Macro..."), menu);
    m_loadMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_load"), m_loadMenu);
    m_loadMenu->setToolTip(i18n("Load a named macro as the current one."));
    m_loadMenu->setEnabled(!plugin->m_namedMacros.isEmpty());
    menu->addAction(m_loadMenu);

    // create play named menu
    m_playMenu = new KActionMenu(i18n("&Play Named Macro..."), menu);
    m_playMenu->setIcon(QIcon::fromTheme(QStringLiteral("auto-type")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_play"), m_playMenu);
    m_playMenu->setToolTip(i18n("Play a named macro without loading it."));
    m_playMenu->setEnabled(!plugin->m_namedMacros.isEmpty());
    menu->addAction(m_playMenu);

    // create wipe named menu
    m_wipeMenu = new KActionMenu(i18n("&Wipe Named Macro..."), menu);
    m_wipeMenu->setIcon(QIcon::fromTheme(QStringLiteral("delete")));
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_wipe"), m_wipeMenu);
    m_wipeMenu->setToolTip(i18n("Wipe a named macro."));
    m_wipeMenu->setEnabled(!plugin->m_namedMacros.isEmpty());
    menu->addAction(m_wipeMenu);

    // add named macros to our menus
    for (const auto &[name, macro] : plugin->m_namedMacros.toStdMap()) {
        addNamedMacro(name, macro.toString());
    }

    // update current state if necessary
    if (plugin->m_recording) {
        recordingOn();
    }
    if (!plugin->m_macro.isEmpty()) {
        macroLoaded(true);
    }

    // add Keyboard Macros actions to the GUI
    mainwindow->guiFactory()->addClient(this);
}

KeyboardMacrosPluginView::~KeyboardMacrosPluginView()
{
    // remove Keyboard Macros actions from the GUI
    m_mainWindow->guiFactory()->removeClient(this);
}

QKeySequence KeyboardMacrosPluginView::recordActionShortcut() const
{
    return m_recordAction->shortcut();
}

QKeySequence KeyboardMacrosPluginView::playActionShortcut() const
{
    return m_playAction->shortcut();
}

void KeyboardMacrosPluginView::recordingOn()
{
    m_recordAction->setText(i18n("End Macro &Recording"));
    m_recordAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
    m_cancelAction->setEnabled(true);
    m_playAction->setEnabled(true);
}

void KeyboardMacrosPluginView::recordingOff()
{
    m_recordAction->setText(i18n("&Record Macro..."));
    m_recordAction->setIcon(QIcon::fromTheme(QStringLiteral("media-record")));
    m_cancelAction->setEnabled(false);
}

void KeyboardMacrosPluginView::macroLoaded(bool enable)
{
    m_playAction->setEnabled(enable);
    m_saveAction->setEnabled(enable);
}

void KeyboardMacrosPluginView::addNamedMacro(const QString &name, const QString &description)
{
    QAction *action;
    QString label = KStringHandler::rsqueeze(name + QStringLiteral(": ") + description, 50)
                        // avoid unwanted accelerators
                        .replace(QRegularExpression(QStringLiteral("&(?!&)")), QStringLiteral("&&"));

    // add load action
    action = new QAction(QStringLiteral("Load ") + label);
    action->setToolTip(i18n("Load the '%1' macro as the current one.", name));
    action->setEnabled(true);
    connect(action, &QAction::triggered, m_plugin, [this, name] {
        slotLoadNamed(name);
    });
    m_loadMenu->addAction(action);
    // remember load action pointer
    m_namedMacrosLoadActions.insert(name, action);
    // update load menu state
    m_loadMenu->setEnabled(true);

    // add play action
    action = new QAction(QStringLiteral("Play ") + label);
    action->setToolTip(i18n("Play the '%1' macro without loading it.", name));
    action->setEnabled(true);
    connect(action, &QAction::triggered, m_plugin, [this, name] {
        slotPlayNamed(name);
    });
    m_playMenu->addAction(action);
    // add the play action to the collection (a user may want to set a shortcut for a macro they use very often)
    actionCollection()->addAction(QStringLiteral("keyboardmacros_named_play_") + name, action);
    // remember play action pointer
    m_namedMacrosPlayActions.insert(name, action);
    // update play menu state
    m_playMenu->setEnabled(true);

    // add wipe action
    action = new QAction(QStringLiteral("Wipe ") + label);
    action->setToolTip(i18n("Wipe the '%1' macro.", name));
    action->setEnabled(true);
    connect(action, &QAction::triggered, m_plugin, [this, name] {
        slotWipeNamed(name);
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

// BEGIN Action slots

void KeyboardMacrosPluginView::slotRecord()
{
    if (m_plugin->m_recording) {
        m_plugin->stop(true);
    } else {
        m_plugin->record();
    }
}

void KeyboardMacrosPluginView::slotPlay()
{
    if (m_plugin->m_recording) {
        m_plugin->stop(true);
    }
    m_plugin->play();
}

void KeyboardMacrosPluginView::slotCancel()
{
    if (!m_plugin->m_recording) {
        return;
    }
    m_plugin->cancel();
}

void KeyboardMacrosPluginView::slotSave()
{
    if (m_plugin->m_recording) {
        return;
    }
    bool ok;
    QString name =
        QInputDialog::getText(m_mainWindow->window(), i18n("Keyboard Macros"), i18n("Save current macro as?"), QLineEdit::Normal, QStringLiteral(""), &ok);
    if (!ok || name.isEmpty()) {
        return;
    }
    m_plugin->save(name);
}

void KeyboardMacrosPluginView::slotLoadNamed(const QString &name)
{
    if (m_plugin->m_recording) {
        return;
    }
    if (name.isEmpty()) {
        return;
    }
    m_plugin->load(name);
}

void KeyboardMacrosPluginView::slotPlayNamed(const QString &name)
{
    if (m_plugin->m_recording) {
        return;
    }
    if (name.isEmpty()) {
        return;
    }
    m_plugin->play(name);
}

void KeyboardMacrosPluginView::slotWipeNamed(const QString &name)
{
    if (m_plugin->m_recording) {
        return;
    }
    if (QMessageBox::question(m_mainWindow->window(), i18n("Keyboard Macros"), i18n("Wipe the '%1' macro?", name)) != QMessageBox::Yes) {
        return;
    }
    m_plugin->wipe(name);
}

// END

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
    const QString &action = actionAndName[0];
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
            const QString &name = actionAndName[1];
            if (!m_plugin->play(name)) {
                msg = i18n("No keyboard macro named '%1' found.", name);
                return false;
            }
            return true;
        }
    }
    const QString &name = actionAndName[1];
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
