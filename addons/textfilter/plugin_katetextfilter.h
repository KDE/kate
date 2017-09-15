 /***************************************************************************
                          plugin_katetextfilter.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLUGIN_KATETEXTFILTER_H
#define PLUGIN_KATETEXTFILTER_H

#include <KTextEditor/Plugin>
#include <KTextEditor/Application>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KTextEditor/Command>

#include <KProcess>
#include <QVariantList>

class PluginKateTextFilter : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    /**
     * Plugin constructor.
     */
    explicit PluginKateTextFilter(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~PluginKateTextFilter() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void runFilter(KTextEditor::View *kv, const QString & filter);

  private:
    QString  m_strFilterOutput;
    QString  m_stderrOutput;
    QString  m_last_command;
    KProcess * m_pFilterProcess;
    QStringList completionList;
    bool copyResult;
    bool mergeOutput;
  public Q_SLOTS:
    void slotEditFilter ();
    void slotFilterReceivedStdout();
    void slotFilterReceivedStderr();
    void slotFilterProcessExited(int exitCode, QProcess::ExitStatus exitStatus);
};

class PluginKateTextFilterCommand : public KTextEditor::Command
{
   Q_OBJECT

public:
   PluginKateTextFilterCommand (PluginKateTextFilter *plugin);
    // Kate::Command
    bool exec (KTextEditor::View *view, const QString &cmd, QString &msg,
                      const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool help (KTextEditor::View *view, const QString &cmd, QString &msg) override;

private:
    PluginKateTextFilter *m_plugin;
};

/**
 * Plugin view to merge the actions into the UI
 */
class PluginViewKateTextFilter: public QObject, public KXMLGUIClient
{
    Q_OBJECT

    public:
        /**
         * Construct plugin view
         * @param plugin our plugin
         * @param mainwindows the mainwindow for this view
         */
        explicit PluginViewKateTextFilter(PluginKateTextFilter *plugin, KTextEditor::MainWindow *mainwindow);

        /**
         * Our Destructor
         */
        ~PluginViewKateTextFilter() override;

    private:
        /**
         * the main window we belong to
         */
        KTextEditor::MainWindow *m_mainWindow;
};

#endif
