/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QAction>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QRegularExpression>
#include <QString>

#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KStringHandler>
#include <KXMLGUIFactory>

#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include "keyboardmacrospluginview.h"

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
    // deregister this view from the plugin
    m_plugin->m_pluginViews.removeOne(this);
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
