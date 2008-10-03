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

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/commandinterface.h>

#include <kxmlguiclient.h>

class K3Process;
class K3ShellProcess;

class PluginKateTextFilter : public Kate::Plugin, public KTextEditor::Command
{
  Q_OBJECT

  public:
    explicit PluginKateTextFilter( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~PluginKateTextFilter();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    // Kate::Command
    const QStringList& cmds ();
    bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
    bool help (KTextEditor::View *view, const QString &cmd, QString &msg);
  private:
    void runFilter( KTextEditor::View *kv, const QString & filter );

  private:
    QString  m_strFilterOutput;
    K3ShellProcess * m_pFilterShellProcess;
    QStringList completionList;
  public slots:
    void slotEditFilter ();
    void slotFilterReceivedStdout (K3Process * pProcess, char * got, int len);
    void slotFilterReceivedStderr (K3Process * pProcess, char * got, int len);
    void slotFilterProcessExited (K3Process * pProcess);
    void slotFilterCloseStdin (K3Process *);
};

class PluginViewKateTextFilter: public Kate::PluginView, public KXMLGUIClient {
  Q_OBJECT

  public:
    PluginViewKateTextFilter(PluginKateTextFilter *plugin, Kate::MainWindow *mainwindow);
    virtual ~PluginViewKateTextFilter();
};

#endif // PLUGIN_KATETEXTFILTER_H

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
