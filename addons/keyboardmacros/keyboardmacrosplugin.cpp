/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "keyboardmacrosplugin.h"

#include <QAction>
#include <QApplication>
#include <QCompleter>
#include <QCoreApplication>
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
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include "keycombination.h"
#include "macro.h"

K_PLUGIN_FACTORY_WITH_JSON(KeyboardMacrosPluginFactory, "keyboardmacrosplugin.json", registerPlugin<KeyboardMacrosPlugin>();)

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

void KeyboardMacrosPlugin::displayMessage(const QString &text, KTextEditor::Message::MessageType type)
{
    delete m_message;
    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        return;
    }
    m_message = new KTextEditor::Message(i18n("<b>Keyboard Macros:</b> %1", text), type);
    m_message->setIcon(QIcon::fromTheme(QStringLiteral("input-keyboard")));
    m_message->setWordWrap(true);
    m_message->setPosition(KTextEditor::Message::BottomInView);
    m_message->setAutoHide(type == KTextEditor::Message::Information ? 600000 : 1500);
    m_message->setAutoHideMode(KTextEditor::Message::Immediate);
    m_message->setView(view);
    view->document()->postMessage(m_message);
}

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
        qDebug() << "[KeyboardMacrosPlugin] key combination:" << kc;
        m_tape.append(kc);
        return false;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

void KeyboardMacrosPlugin::record()
{
    // start recording
    qDebug() << "[KeyboardMacrosPlugin] start recording";
    // install our spy on currently focused widget
    m_focusWidget = qApp->focusWidget();
    m_focusWidget->installEventFilter(this);
    // update recording status
    m_recording = true;
    // update GUI
    m_recordAction->setText(i18n("End Macro &Recording"));
    m_cancelAction->setEnabled(true);
    m_playAction->setEnabled(true);
    // connect focus change events
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &KeyboardMacrosPlugin::applicationStateChanged);
    connect(qApp, &QGuiApplication::focusObjectChanged, this, &KeyboardMacrosPlugin::focusObjectChanged);
    // display feedback
    displayMessage(i18n("Recording…"), KTextEditor::Message::Information);
}

void KeyboardMacrosPlugin::stop(bool save)
{
    // stop recording
    qDebug() << "[KeyboardMacrosPlugin]" << (save ? "end" : "cancel") << "recording";
    // uninstall our spy
    m_focusWidget->removeEventFilter(this);
    // update recording status
    m_recording = false;
    if (save) { // end recording
        // delete current macro
        m_macro.clear();
        // replace it with the tape
        m_macro.swap(m_tape);
        // clear tape
        m_tape.clear();
        // update GUI
        m_playAction->setEnabled(!m_macro.isEmpty());
        m_saveNamedAction->setEnabled(!m_macro.isEmpty());
    } else { // cancel recording
        // delete tape
        m_tape.clear();
    }
    // update GUI
    m_recordAction->setText(i18n("&Record Macro..."));
    m_cancelAction->setEnabled(false);
    // disconnect focus change events
    disconnect(qApp, &QGuiApplication::applicationStateChanged, this, &KeyboardMacrosPlugin::applicationStateChanged);
    disconnect(qApp, &QGuiApplication::focusObjectChanged, this, &KeyboardMacrosPlugin::focusObjectChanged);
    // display feedback
    displayMessage(i18n("Recording %1", (save ? i18n("ended") : i18n("canceled"))), KTextEditor::Message::Positive);
}

void KeyboardMacrosPlugin::cancel()
{
    stop(false);
}

bool KeyboardMacrosPlugin::play(const QString &name)
{
    Macro m;
    if (!name.isEmpty() && m_namedMacros.contains(name)) {
        m = m_namedMacros.value(name);
        qDebug() << "[KeyboardMacrosPlugin] playing macro:" << name;
    } else if (name.isEmpty() && !m_macro.isEmpty()) {
        m = m_macro;
        qDebug() << "[KeyboardMacrosPlugin] playing macro!";
    } else {
        return false;
    }
    Macro::Iterator it;
    for (it = m.begin(); it != m.end(); it++) {
        QKeyEvent *keyEvent;
        // send key press
        keyEvent = (*it).keyPress();
        qApp->sendEvent(qApp->focusWidget(), keyEvent);
        delete keyEvent;
        // send key release
        keyEvent = (*it).keyRelease();
        qApp->sendEvent(qApp->focusWidget(), keyEvent);
        delete keyEvent;
    }
    return true;
}

bool KeyboardMacrosPlugin::save(const QString &name)
{
    // we don't need to save macros that do nothing
    if (m_macro.isEmpty()) {
        return false;
    }
    qDebug() << "[KeyboardMacrosPlugin] saving macro:" << name;
    m_namedMacros.insert(name, m_macro);
    // update GUI
    m_loadNamedAction->setEnabled(true);
    // m_playNamedAction->setEnabled(true);
    m_deleteNamedAction->setEnabled(true);
    // display feedback
    displayMessage(i18n("Saved '%1'", name), KTextEditor::Message::Positive);
    return true;
}

bool KeyboardMacrosPlugin::load(const QString &name)
{
    if (!m_namedMacros.contains(name)) {
        return false;
    }
    qDebug() << "[KeyboardMacrosPlugin] loading macro:" << name;
    // clear current macro
    m_macro.clear();
    // load named macro
    m_macro = m_namedMacros.value(name);
    // update GUI
    m_playAction->setEnabled(true);
    // display feedback
    displayMessage(i18n("Loaded '%1'", name), KTextEditor::Message::Positive);
    return true;
}

bool KeyboardMacrosPlugin::remove(const QString &name)
{
    if (!m_namedMacros.contains(name)) {
        return false;
    }
    qDebug() << "[KeyboardMacrosPlugin] removing macro:" << name;
    m_namedMacros.remove(name);
    // update GUI
    m_loadNamedAction->setEnabled(!m_namedMacros.isEmpty());
    // m_playNamedAction->setEnabled(!m_namedMacros.isEmpty());
    m_deleteNamedAction->setEnabled(!m_namedMacros.isEmpty());
    // display feedback
    displayMessage(i18n("Deleted '%1'", name), KTextEditor::Message::Positive);
    return true;
}

QString KeyboardMacrosPlugin::queryName(const QString &query, const QString &action)
{
    QInputDialog dialog(qApp->focusWidget());
    dialog.setWindowTitle(i18n("Keyboard Macros"));
    dialog.setLabelText(query);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setOkButtonText(action);
    QLineEdit *lineEdit = dialog.findChild<QLineEdit *>();
    QCompleter *completer = new QCompleter(QStringList(m_namedMacros.keys()), lineEdit);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    if (lineEdit != nullptr) {
        lineEdit->setCompleter(completer);
    }
    if (dialog.exec() != QDialog::Accepted) {
        return QString();
    }
    delete completer;
    return dialog.textValue();
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
    QJsonObject::ConstIterator it;
    for (it = json.constBegin(); it != json.constEnd(); ++it) {
        m_namedMacros.insert(it.key(), Macro(it.value()));
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
    QMap<QString, Macro>::ConstIterator it;
    for (it = m_namedMacros.constBegin(); it != m_namedMacros.constEnd(); ++it) {
        json.insert(it.key(), it.value().toJson());
    }
    storage.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    storage.close();
}

void KeyboardMacrosPlugin::focusObjectChanged(QObject *focusObject)
{
    qDebug() << "[KeyboardMacrosPlugin] focusObjectChanged:" << focusObject;
    QWidget *focusWidget = dynamic_cast<QWidget *>(focusObject);
    if (focusWidget == nullptr || focusWidget == m_focusWidget) {
        return;
    }
    // update which widget we filter events from when the focus has changed
    m_focusWidget->removeEventFilter(this);
    m_focusWidget = focusWidget;
    m_focusWidget->installEventFilter(this);
}

void KeyboardMacrosPlugin::applicationStateChanged(Qt::ApplicationState state)
{
    qDebug() << "[KeyboardMacrosPlugin] applicationStateChanged:" << state;
    // somehow keeping our event filter on while the app is out of focus made Kate crash, we fix that here
    switch (state) {
    case Qt::ApplicationSuspended:
        sendMessage(i18n("Application suspended, aborting record."), true);
        cancel();
        break;
    case Qt::ApplicationHidden:
    case Qt::ApplicationInactive:
        m_focusWidget->removeEventFilter(this);
        break;
    case Qt::ApplicationActive:
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

void KeyboardMacrosPlugin::slotSaveNamed()
{
    if (m_recording) {
        return;
    }
    QString name = queryName(i18n("Under which name should the current macro be saved?"), i18n("Save Macro"));
    if (name.isEmpty()) {
        return;
    }
    save(name);
}

void KeyboardMacrosPlugin::slotLoadNamed()
{
    if (m_recording) {
        return;
    }
    QString name = queryName(i18n("Which named macro do you want to load?"), i18n("Load Macro"));
    if (name.isEmpty()) {
        return;
    }
    load(name);
}

// void KeyboardMacrosPlugin::slotPlayNamed()
// {
//     if (m_recording) {
//         return;
//     }
//     QWidget *focused = qApp->focusWidget();
//     qDebug() << focused;
//     QString name = queryName(i18n("Which named macro do you want to play?"), i18n("Play Macro"));
//     if (name.isEmpty()) {
//         return;
//     }
//     // set focus back to the widget which had it before the dialog otherwise the macro is
//     // sometimes played with focus on the deleted input dialog which makes Kate crash
//     focused->setFocus(); // FIXME: this "fix" isn't enough
//     play(name);
// }

void KeyboardMacrosPlugin::slotDeleteNamed()
{
    if (m_recording) {
        return;
    }
    QString name = queryName(i18n("Which named macro do you want to delete?"), i18n("Delete Macro"));
    if (name.isEmpty()) {
        return;
    }
    remove(name);
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
    QAction *record = actionCollection()->addAction(QStringLiteral("keyboardmacros_record"));
    record->setText(i18n("&Record Macro..."));
    record->setToolTip(i18n("Start/stop recording a macro (i.e., keyboard action sequence)."));
    actionCollection()->setDefaultShortcut(record, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
    connect(record, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotRecord);
    plugin->m_recordAction = record;

    // create cancel action
    QAction *cancel = actionCollection()->addAction(QStringLiteral("keyboardmacros_cancel"));
    cancel->setText(i18n("&Cancel Macro Recording"));
    cancel->setToolTip(i18n("Cancel ongoing recording (and keep the previous macro as the current one)."));
    actionCollection()->setDefaultShortcut(cancel, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_K, Qt::Key_C));
    cancel->setEnabled(false);
    connect(cancel, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotCancel);
    plugin->m_cancelAction = cancel;

    // create play action
    QAction *play = actionCollection()->addAction(QStringLiteral("keyboardmacros_play"));
    play->setText(i18n("&Play Macro"));
    play->setToolTip(i18n("Play current macro (i.e., execute the last recorded keyboard action sequence)."));
    actionCollection()->setDefaultShortcut(play, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_K));
    play->setEnabled(false);
    connect(play, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotPlay);
    plugin->m_playAction = play;

    // create save named action
    QAction *saveNamed = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_save"));
    saveNamed->setText(i18n("&Save Current Macro"));
    saveNamed->setToolTip(i18n("Give a name to the current macro and persistently save it."));
    actionCollection()->setDefaultShortcut(saveNamed, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_K, Qt::Key_S));
    saveNamed->setEnabled(false);
    connect(saveNamed, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotSaveNamed);
    plugin->m_saveNamedAction = saveNamed;

    // create load named action
    QAction *loadNamed = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_load"));
    loadNamed->setText(i18n("&Load Named Macro"));
    loadNamed->setToolTip(i18n("Load a named macro as the current one."));
    actionCollection()->setDefaultShortcut(loadNamed, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_K, Qt::Key_L));
    loadNamed->setEnabled(!plugin->m_namedMacros.isEmpty());
    connect(loadNamed, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotLoadNamed);
    plugin->m_loadNamedAction = loadNamed;

    // // create play named action
    // QAction *playNamed = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_play"));
    // playNamed->setText(i18n("&Play Named Macro"));
    // playNamed->setToolTip(i18n("Play a named macro without loading it."));
    // actionCollection()->setDefaultShortcut(playNamed, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_K, Qt::Key_P));
    // playNamed->setEnabled(!plugin->m_namedMacros.isEmpty());
    // connect(playNamed, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotPlayNamed);
    // plugin->m_playNamedAction = playNamed;

    // create delete named action
    QAction *deleteNamed = actionCollection()->addAction(QStringLiteral("keyboardmacros_named_delete"));
    deleteNamed->setText(i18n("&Delete Named Macro"));
    deleteNamed->setToolTip(i18n("Delete a named macro."));
    actionCollection()->setDefaultShortcut(deleteNamed, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_K, Qt::Key_D));
    deleteNamed->setEnabled(!plugin->m_namedMacros.isEmpty());
    connect(deleteNamed, &QAction::triggered, plugin, &KeyboardMacrosPlugin::slotDeleteNamed);
    plugin->m_deleteNamedAction = deleteNamed;

    // add Keyboard Macros actions to the GUI
    mainwindow->guiFactory()->addClient(this);
}

KeyboardMacrosPluginView::~KeyboardMacrosPluginView()
{
    // remove Keyboard Macros actions from the GUI
    m_mainWindow->guiFactory()->removeClient(this);
}

// END

// BEGIN Plugin commands to manage named keyboard macros

KeyboardMacrosPluginCommands::KeyboardMacrosPluginCommands(KeyboardMacrosPlugin *plugin)
    : KTextEditor::Command(QStringList() << QStringLiteral("kmsave") << QStringLiteral("kmload") << QStringLiteral("kmplay") << QStringLiteral("kmdelete"),
                           plugin)
    , m_plugin(plugin)
{
}

bool KeyboardMacrosPluginCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &)
{
    QStringList actionAndName = cmd.split(QRegularExpression(QStringLiteral("\\s+")));
    if (actionAndName.length() != 2) {
        msg = i18n("Usage: %1 <name>.", actionAndName.at(0));
        return false;
    }
    QString action = actionAndName.at(0);
    QString name = actionAndName.at(1);
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
    } else if (action == QStringLiteral("kmdelete")) {
        if (!m_plugin->remove(name)) {
            msg = i18n("No keyboard macro named '%1' found.", name);
            return false;
        }
        return true;
    } else if (action == QStringLiteral("kmplay")) {
        // set focus on the view otherwise the macro is played with focus on the command line
        view->setFocus();
        // then attempt to play the given macro
        if (!m_plugin->play(name)) {
            msg = i18n("No keyboard macro named '%1' found.", name);
            return false;
        }
        return true;
    }
    return false;
}

bool KeyboardMacrosPluginCommands::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
    QString namedMacros;
    if (!m_plugin->m_namedMacros.keys().isEmpty()) {
        namedMacros += QStringLiteral("<p><b>Named macros:</b> ");
        QList<QString> names = m_plugin->m_namedMacros.keys();
        QList<QString>::ConstIterator it;
        namedMacros += names.first();
        for (it = ++names.constBegin(); it != names.constEnd(); ++it) {
            namedMacros += QStringLiteral(", ") + *it;
        }
        namedMacros += QStringLiteral(".</p>");
    }
    if (cmd == QStringLiteral("kmsave")) {
        msg = i18n("<qt><p>Usage: <code>kmsave &lt;name&gt;</code></p><p>Save current keyboard macro as <code>&lt;name&gt;</code>.</p>%1</qt>", namedMacros);
        return true;
    } else if (cmd == QStringLiteral("kmload")) {
        msg = i18n("<qt><p>Usage: <code>kmload &lt;name&gt;</code></p><p>Load saved keyboard macro <code>&lt;name&gt;</code> as current macro.</p>%1</qt>",
                   namedMacros);
        return true;
    } else if (cmd == QStringLiteral("kmdelete")) {
        msg = i18n("<qt><p>Usage: <code>kmdelete &lt;name&gt;</code></p><p>Delete saved keyboard macro <code>&lt;name&gt;</code>.</p>%1</qt>", namedMacros);
        return true;
    } else if (cmd == QStringLiteral("kmplay")) {
        msg = i18n("<qt><p>Usage: <code>kmplay &lt;name&gt;</code></p><p>Play saved keyboard macro <code>&lt;name&gt;</code> without loading it.</p>%1</qt>",
                   namedMacros);
        return true;
    }
    return false;
}

// END

// required for KeyboardMacrosPluginFactory vtable
#include "keyboardmacrosplugin.moc"
