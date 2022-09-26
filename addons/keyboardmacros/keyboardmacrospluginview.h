/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGINVIEW_H
#define KEYBOARDMACROSPLUGINVIEW_H

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>

#include <KActionMenu>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include "keyboardmacrosplugin.h"

/**
 * Plugin view to add keyboard macros actions to the GUI
 */
class KeyboardMacrosPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    KeyboardMacrosPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QPointer<QAction> m_recordAction;
    QPointer<QAction> m_cancelAction;
    QPointer<QAction> m_playAction;
    QPointer<QAction> m_saveAction;
    QPointer<KActionMenu> m_loadMenu;
    QMap<QString, QPointer<QAction>> m_namedMacrosLoadActions;
    QPointer<KActionMenu> m_playMenu;
    QMap<QString, QPointer<QAction>> m_namedMacrosPlayActions;
    QPointer<KActionMenu> m_wipeMenu;
    QMap<QString, QPointer<QAction>> m_namedMacrosWipeActions;

public:
    explicit KeyboardMacrosPluginView(KeyboardMacrosPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~KeyboardMacrosPluginView() override;

    // shortcut getter
    QKeySequence recordActionShortcut() const;
    QKeySequence playActionShortcut() const;

    // GUI update helpers
    void recordingOn();
    void recordingOff();
    void macroLoaded(bool enable);
    void addNamedMacro(const QString &name, const QString &description);
    void removeNamedMacro(const QString &name);

    // Action slots
public Q_SLOTS:
    void slotRecord();
    void slotCancel();
    void slotPlay();
    void slotSave();
    void slotLoadNamed(const QString &name = QString());
    void slotPlayNamed(const QString &name = QString());
    void slotWipeNamed(const QString &name = QString());
};

#endif
