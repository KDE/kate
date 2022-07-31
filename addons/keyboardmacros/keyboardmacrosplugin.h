/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLUGIN_KATEKEYBOARDMACRO_H
#define PLUGIN_KATEKEYBOARDMACRO_H

#include <QKeyEvent>
#include <QList>

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

typedef QList<QKeyEvent *> Macro;

class KeyboardMacrosPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    friend class KeyboardMacrosPluginView;

public:
    explicit KeyboardMacrosPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~KeyboardMacrosPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void sendMessage(const QString &text, bool error);

Q_SIGNALS:
    void message(const QVariantMap &message);

public:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QWidget *m_focusWidget = nullptr;

    QAction *m_recordAction = nullptr;
    QAction *m_cancelAction = nullptr;
    QAction *m_playAction = nullptr;

    bool m_recording = false;
    Macro m_tape;
    Macro m_macro;

    void record();
    void stop(bool save);
    void cancel();
    bool play();

    void applicationStateChanged(Qt::ApplicationState state);

public Q_SLOTS:
    void slotRecord();
    void slotCancel();
    void slotPlay();
};

/**
 * Plugin view to add our actions to the gui
 */
class KeyboardMacrosPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit KeyboardMacrosPluginView(KeyboardMacrosPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~KeyboardMacrosPluginView() override;

private:
    KTextEditor::MainWindow *m_mainWindow;
};

#endif
