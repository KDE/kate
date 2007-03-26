/***************************************************************************
                          plugin_katetextfilter.cpp  -  description
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

#include "plugin_katetextfilter.h"
#include "plugin_katetextfilter.moc"

#include <ktexteditor/editor.h>

#include <kaction.h>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <cassert>
#include <kdebug.h>
#include <qstring.h>
#include <klineedit.h>
#include <kinputdialog.h>
#define POP_(x) kDebug(13000) << #x " = " << flush << x << endl

#include <kgenericfactory.h>
#include <kauthorized.h>
#include <kactioncollection.h>
K_EXPORT_COMPONENT_FACTORY( katetextfilterplugin, KGenericFactory<PluginKateTextFilter>( "katetextfilter" ) )

PluginViewKateTextFilter::PluginViewKateTextFilter(PluginKateTextFilter *plugin,Kate::MainWindow *mainwindow)
  : Kate::PluginView(mainwindow),KXMLGUIClient()
{
    QAction *a = actionCollection()->addAction("edit_filter");
    a->setText(i18n("Filter Te&xt..."));
    a->setShortcut( Qt::CTRL + Qt::Key_Backslash );
    connect( a, SIGNAL(triggered(bool)), plugin, SLOT(slotEditFilter()) );

    setComponentData (KComponentData("kate"));
    setXMLFile( "plugins/katetextfilter/ui.rc" );
    mainwindow->guiFactory()->addClient (this);
}

PluginViewKateTextFilter::~PluginViewKateTextFilter()
{
      mainWindow()->guiFactory()->removeClient (this);
}

PluginKateTextFilter::PluginKateTextFilter( QObject* parent, const QStringList& )
  : Kate::Plugin ( (Kate::Application *)parent, "kate-text-filter-plugin" ),
    KTextEditor::Command(),
    m_pFilterShellProcess (NULL)
{
  KTextEditor::CommandInterface* cmdIface = qobject_cast<KTextEditor::CommandInterface*>(application()->editor());
  if( cmdIface ) cmdIface->registerCommand( this );
}

PluginKateTextFilter::~PluginKateTextFilter()
{
  delete m_pFilterShellProcess;
  KTextEditor::CommandInterface* cmdIface = qobject_cast<KTextEditor::CommandInterface*>(application()->editor());
  if( cmdIface ) cmdIface->unregisterCommand( this );
}


Kate::PluginView *PluginKateTextFilter::createView (Kate::MainWindow *mainWindow)
{
    return new PluginViewKateTextFilter(this,mainWindow);
}

	void
PluginKateTextFilter::slotFilterReceivedStdout (K3Process * pProcess, char * got, int len)
{

	assert (pProcess == m_pFilterShellProcess);

	if (got && len)
		{
		m_strFilterOutput += QString::fromLocal8Bit( got, len );
//		POP_(m_strFilterOutput);
		}

}


	void
PluginKateTextFilter::slotFilterReceivedStderr (K3Process * pProcess, char * got, int len)
	{
	slotFilterReceivedStdout (pProcess, got, len);
	}


	void
PluginKateTextFilter::slotFilterProcessExited (K3Process * pProcess)
{

	assert (pProcess == m_pFilterShellProcess);
	KTextEditor::View * kv (application()->activeMainWindow()->activeView());
	if (!kv) return;
	kv->document()->startEditing();
	if( kv->selection () )
	  kv->removeSelectionText();
	kv -> insertText (m_strFilterOutput);
	kv->document()->endEditing();
	m_strFilterOutput = "";

}


        static void  //  PCP
slipInFilter (K3ShellProcess & shell, KTextEditor::View & view, QString command)
{
  if( !view.selection() ) return;
  QString marked = view.selectionText ();
  if( marked.isEmpty())
      return;
//  POP_(command.latin1 ());
  shell.clearArguments ();
  shell << command;

  shell.start (K3Process::NotifyOnExit, K3Process::All);
  shell.writeStdin (marked.toLocal8Bit (), marked.length ());
  //  TODO: Put up a modal dialog to defend the text from further
  //  keystrokes while the command is out. With a cancel button...

}


	void
PluginKateTextFilter::slotFilterCloseStdin (K3Process * pProcess)
	{
	assert (pProcess == m_pFilterShellProcess);
	pProcess -> closeStdin ();
	}


                 void
PluginKateTextFilter::slotEditFilter ()  //  PCP
{
  if (!KAuthorized::authorizeKAction("shell_access")) {
      KMessageBox::sorry(0,i18n(
          "You are not allowed to execute arbitrary external applications. If "
          "you want to be able to do this, contact your system administrator."),
          i18n("Access Restrictions"));
      return;
  }
  if (!application()->activeMainWindow())
    return;

  KTextEditor::View * kv (application()->activeMainWindow()->activeView());
  if (!kv) return;

  bool ok(false);
  QString text ( KInputDialog::getText(i18n("Filter"),"",i18n("Enter command to pipe selected text through:"),&ok));
  if ( ok && (!text.isEmpty () ))
    runFilter( kv, text );
}

void PluginKateTextFilter::runFilter( KTextEditor::View *kv, const QString &filter )
{
  m_strFilterOutput = "";

  if (!m_pFilterShellProcess)
  {
    m_pFilterShellProcess = new K3ShellProcess;

    connect ( m_pFilterShellProcess, SIGNAL(wroteStdin(K3Process *)),
              this, SLOT(slotFilterCloseStdin (K3Process *)));

    connect ( m_pFilterShellProcess, SIGNAL(receivedStdout(K3Process*,char*,int)),
              this, SLOT(slotFilterReceivedStdout(K3Process*,char*,int)) );

    connect ( m_pFilterShellProcess, SIGNAL(receivedStderr(K3Process*,char*,int)),
              this, SLOT(slotFilterReceivedStderr(K3Process*,char*,int)) );

    connect ( m_pFilterShellProcess, SIGNAL(processExited(K3Process*)),
              this, SLOT(slotFilterProcessExited(K3Process*) ) ) ;
  }

  slipInFilter (*m_pFilterShellProcess, *kv, filter);
}

//BEGIN Kate::Command methods
const QStringList &PluginKateTextFilter::cmds()
{
  static QStringList dummy("textfilter");
  return dummy;
}

bool PluginKateTextFilter::help( KTextEditor::View *, const QString&, QString &msg )
{
  msg = i18n(
      "<qt><p>Usage: <code>textfilter COMMAND</code></p>"
      "<p>Replace the selection with the output of the specified shell command.</p></qt>");
  return true;
}

bool PluginKateTextFilter::exec( KTextEditor::View *v, const QString &cmd, QString &msg )
{
  if (! v->selection() )
  {
    msg = i18n("You need to have a selection to use textfilter");
    return false;
  }

  QString filter = cmd.section( " ", 1 ).trimmed();

  if ( filter.isEmpty() )
  {
    msg = i18n("Usage: textfilter COMMAND");
    return false;
  }

  runFilter( v, filter );
  return true;
}
//END
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
