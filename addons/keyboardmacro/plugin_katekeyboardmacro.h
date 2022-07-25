/***************************************************************************
                         plugin_katekeyboardmacro.h  -  description
                            -------------------
   begin                : FRE Feb 23 2001
   copyright            : (C) 2001 by Joseph Wenninger
   email                : jowenn@bigfoot.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/

#ifndef PLUGIN_KATEKEYBOARDMACROS_H
#define PLUGIN_KATEKEYBOARDMACROS_H

#include <KTextEditor/Application>
#include <KTextEditor/Command>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <KProcess>
#include <QVariantList>

class PluginKateKeyboardMacro : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    /**
     * Plugin constructor.
     */
    explicit PluginKateKeyboardMacro(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~PluginKateKeyboardMacro() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void runFilter(KTextEditor::View *kv, const QString &filter);

private:
    QString m_strFilterOutput;
    QString m_stderrOutput;
    QString m_last_command;
    KProcess *m_pFilterProcess = nullptr;
    QStringList completionList;
    bool copyResult = false;
    bool mergeOutput = false;
    bool newDocument = false;
    KTextEditor::MainWindow *m_mainWindow;
public Q_SLOTS:
    void slotEditFilter();
    void slotFilterReceivedStdout();
    void slotFilterReceivedStderr();
    void slotFilterProcessExited(int exitCode, QProcess::ExitStatus exitStatus);
};

class PluginKateKeyboardMacroCommand : public KTextEditor::Command
{
    Q_OBJECT

public:
    PluginKateKeyboardMacroCommand(PluginKateKeyboardMacro *plugin);
    // Kate::Command
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;

private:
    PluginKateKeyboardMacro *m_plugin;
};

/**
 * Plugin view to merge the actions into the UI
 */
class PluginViewKateKeyboardMacro : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Construct plugin view
     * @param plugin our plugin
     * @param mainwindows the mainwindow for this view
     */
    explicit PluginViewKateKeyboardMacro(PluginKateKeyboardMacro *plugin, KTextEditor::MainWindow *mainwindow);

    /**
     * Our Destructor
     */
    ~PluginViewKateKeyboardMacro() override;

private:
    /**
     * the main window we belong to
     */
    KTextEditor::MainWindow *m_mainWindow;
};

#endif
