/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   SPDX-FileCopyrightText: 2017 Ederag <edera@gmx.fr>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KTextEditor/Plugin>
#include <ktexteditor/configpage.h>
#include <ktexteditor/mainwindow.h>

#include <QKeyEvent>
#include <QList>

#include <KXMLGUIClient>

class QShowEvent;

namespace KParts
{
class ReadOnlyPart;
}

class KateConsole;
class KateKonsolePluginView;
class KPluginFactory;

class KateKonsolePlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    friend class KateKonsolePluginView;

public:
    explicit KateKonsolePlugin(QObject *parent = nullptr, const QVariantList & = QVariantList());
    ~KateKonsolePlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override
    {
        return 1;
    }
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    void readConfig();

    QByteArray previousEditorEnv()
    {
        return m_previousEditorEnv;
    }

private:
    QList<KateKonsolePluginView *> mViews;
    QByteArray m_previousEditorEnv;
};

class KateKonsolePluginView : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    KateKonsolePluginView(KateKonsolePlugin *plugin, KTextEditor::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateKonsolePluginView() override;

    void readConfig();

private:
    KateKonsolePlugin *m_plugin;
    KateConsole *m_console;
};

/**
 * KateConsole
 * This class is used for the internal terminal emulator
 * It uses internally the konsole part, thx to konsole devs :)
 */
class KateConsole : public QWidget, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * construct us
     * @param mw main window
     * @param parent toolview
     */
    KateConsole(KateKonsolePlugin *plugin, KTextEditor::MainWindow *mw, QWidget *parent);

    /**
     * destruct us
     */
    ~KateConsole() override;

    /**
     * sync mode
     */
    enum SyncMode { SyncNothing = 0, SyncCurrentTab = 1, SyncCreateTabPerDir = 2 };

    void readConfig();

    /**
     * cd to dir
     * @param path given local directory
     */
    void cd(const QString &path);

    /**
     * send given text to console
     * @param text commands for console
     */
    void sendInput(const QString &text);

    KTextEditor::MainWindow *mainWindow()
    {
        return m_mw;
    }

    bool eventFilter(QObject *w, QEvent *e) override;

    static KPluginFactory *pluginFactory();

    bool hasKonsole() const
    {
        return pluginFactory() != nullptr;
    }

public Q_SLOTS:
    /**
     * pipe current document to console
     */
    void slotPipeToConsole();

    /**
     * synchronize the konsole with the current document (cd to the directory)
     */
    void slotSync();

    /**
     * synchronize the konsole when the current document's url changes (e.g. save as)
     */
    void slotViewOrUrlChanged(KTextEditor::View *view = nullptr);

    /**
     * When syncing is done by the user, also show the terminal if it is hidden
     */
    void slotManualSync();

    /**
     * run the current document in the konsole
     */
    void slotRun();

    /**
     * split terminal view vertically
     */
    void slotSplitVertical();

    /**
     * split terminal view horizontally
     */
    void slotSplitHorizontal();

    /**
     * open a new tab
     */
    void slotNewTab();

private Q_SLOTS:
    /**
     * the konsole exited ;)
     * handle that, hide the dock
     */
    void slotDestroyed();

    /**
     * construct console if needed
     */
    void loadConsoleIfNeeded(QString directory = QString());

    /**
     * Show or hide the konsole view as appropriate.
     */
    void slotToggleVisibility();

    /**
     * set or clear focus as appropriate.
     */
    void slotToggleFocus();

    /**
     * changes the menu actions text based on focus
     */
    void focusChanged(QWidget *, QWidget *now);

    /**
     * Handle that shortcuts are not eaten by console
     */
    static void overrideShortcut(QKeyEvent *event, bool &override);

    /**
     * hide terminal on Esc key press
     */
    void handleEsc(QEvent *e);

protected:
    /**
     * the konsole get shown
     * @param ev show event
     */
    void showEvent(QShowEvent *ev) override;

    void paintEvent(QPaintEvent *e) override;

private:
    /**
     * plugin factory for the terminal
     */
    static inline KPluginFactory *s_pluginFactory = nullptr;

    /**
     * console part
     */
    KParts::ReadOnlyPart *m_part;

    /**
     * main window of this console
     */
    KTextEditor::MainWindow *m_mw;

    /**
     * toolview for this console
     */
    QWidget *m_toolView;

    KateKonsolePlugin *m_plugin;
    QString m_currentPath;
    QMetaObject::Connection m_urlChangedConnection;

    // current sync mode as read from the config
    SyncMode m_syncMode = SyncNothing;
};

class KateKonsoleConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT
public:
    explicit KateKonsoleConfigPage(QWidget *parent = nullptr, KateKonsolePlugin *plugin = nullptr);
    ~KateKonsoleConfigPage() override
    {
    }

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void reset() override;
    void defaults() override
    {
    }

private:
    class QButtonGroup *m_syncMode;
    class QCheckBox *cbRemoveExtension;
    class QLineEdit *lePrefix;
    class QCheckBox *cbSetEditor;
    class QCheckBox *cbSetEscHideKonsole;
    class QLineEdit *leEscExceptions;
    KateKonsolePlugin *mPlugin;

private Q_SLOTS:
    /**
     * Enable the warning dialog for the next "Run in terminal"
     */
    static void slotEnableRunWarning();
};

Q_DECLARE_METATYPE(KateConsole::SyncMode)
