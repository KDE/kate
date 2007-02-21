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
#include <kcompletion.h>
#include <klineedit.h>
#include <kinputdialog.h>
#define POP_(x) kDebug(13000) << #x " = " << flush << x << endl

#include <kgenericfactory.h>
#include <kauthorized.h>
#include <kactioncollection.h>
K_EXPORT_COMPONENT_FACTORY( katetextfilterplugin, KGenericFactory<PluginKateTextFilter>( "katetextfilter" ) )

class PluginView : public KXMLGUIClient
{
  friend class PluginKateTextFilter;

  public:
    Kate::MainWindow *win;
};

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

void PluginKateTextFilter::addView(Kate::MainWindow *win)
{
    PluginView *view = new PluginView ();

    QAction *a = view->actionCollection()->addAction("edit_filter");
    a->setText(i18n("Filter Te&xt..."));
    a->setShortcut( Qt::CTRL + Qt::Key_Backslash );
    connect( a, SIGNAL(triggered(bool)), this, SLOT(slotEditFilter()) );

    view->setComponentData (KComponentData("kate"));
    view->setXMLFile( "plugins/katetextfilter/ui.rc" );
    win->guiFactory()->addClient (view);
    view->win = win;

   m_views.append (view);
}

void PluginKateTextFilter::removeView(Kate::MainWindow *win)
{
  for (int z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.removeAll (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }
}

void PluginKateTextFilter::storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}
 
void PluginKateTextFilter::loadViewConfig(KConfig* config, Kate::MainWindow*win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateTextFilter::storeGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateTextFilter::loadGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

        void
splitString (QString q, char c, QStringList &list)  //  PCP
{

// screw the OnceAndOnlyOnce Principle!

  int pos;
  QString item;

  while ( (pos = q.indexOf(c)) >= 0)
    {
      item = q.left(pos);
      list.append(item);
      q.remove(0,pos+1);
    }
  list.append(q);
}


        static void  //  PCP
slipInNewText (KTextEditor::View & view, QString pre, QString marked, QString post, bool reselect)
{
#ifdef __GNUC__
#warning "kde4: this function is not used for the moment";
#endif

#if 0
  int preDeleteLine = 0, preDeleteCol = 0;
  view.cursorPosition ().position(preDeleteLine, preDeleteCol);

  if (marked.length() > 0)
    view.removeSelectionText ();
  int line = 0, col = 0;
  view.cursorPosition ().position(line, col);
  view.insertText (pre + marked + post);

  //  all this muck to leave the cursor exactly where the user
  //  put it...

  //  Someday we will can all this (unless if it already
  //  is canned and I didn't find it...)

  //  The second part of the if disrespects the display bugs
  //  when we try to reselect. TODO: fix those bugs, and we can
  //  un-break this if...

  //  TODO: fix OnceAndOnlyOnce between this module and plugin_katehtmltools.cpp

  if (reselect && preDeleteLine == line && -1 == marked.indexOf ('\n'))
    if (preDeleteLine == line && preDeleteCol == col)
        {
        view.setCursorPosition ( KTextEditor::Cursor(line, col + pre.length () + marked.length () - 1) );

        for (int x (marked.length());  x--;)
                view.shiftCursorLeft ();
        }
   else
        {
        view.setCursorPosition (line, col += pre.length ());

        for (int x (marked.length());  x--;)
                view.shiftCursorRight ();
        }
#endif
}


        static QString  //  PCP
KatePrompt
        (
        const QString & strTitle,
        const QString & strPrompt,
        QWidget * that,
	QStringList *completionList
        )
{
  //  TODO: Make this a "memory edit" field with a combo box
  //  containing prior entries
return KInputDialog::getText(strTitle,QString::null,strPrompt);
#if 0
  KInputDialog dlg(strPrompt, QString::null,QString::null, that);
  dlg.setCaption(strTitle);
  KCompletion *comple=dlg.lineEdit()->completionObject();
  comple->setItems(*completionList);
  if (dlg.exec()) {
    if (!dlg.text().isEmpty())	{
	comple->addItem(dlg.text());
	(*completionList)=comple->items();
    }
    return dlg.text();
  }
  else
    return "";
#endif
}


	void
PluginKateTextFilter::slotFilterReceivedStdout (KProcess * pProcess, char * got, int len)
{

	assert (pProcess == m_pFilterShellProcess);

	if (got && len)
		{
		m_strFilterOutput += QString::fromLocal8Bit( got, len );
//		POP_(m_strFilterOutput);
		}

}


	void
PluginKateTextFilter::slotFilterReceivedStderr (KProcess * pProcess, char * got, int len)
	{
	slotFilterReceivedStdout (pProcess, got, len);
	}


	void
PluginKateTextFilter::slotFilterProcessExited (KProcess * pProcess)
{

	assert (pProcess == m_pFilterShellProcess);
	KTextEditor::View * kv (application()->activeMainWindow()->activeView());
	if (!kv) return;
	kv->document()->startEditing();
	if( kv->selection () )
	  kv->removeSelectionText();
	kv -> insertText (m_strFilterOutput);
	kv->document()->endEditing();
//	slipInNewText (*kv, "", m_strFilterOutput, "", false);
	m_strFilterOutput = "";

}


        static void  //  PCP
slipInFilter (KShellProcess & shell, KTextEditor::View & view, QString command)
{
  if( !view.selection() ) return;
  QString marked = view.selectionText ();
  if( marked.isEmpty())
      return;
//  POP_(command.latin1 ());
  shell.clearArguments ();
  shell << command;

  shell.start (KProcess::NotifyOnExit, KProcess::All);
  shell.writeStdin (marked.toLocal8Bit (), marked.length ());
  //  TODO: Put up a modal dialog to defend the text from further
  //  keystrokes while the command is out. With a cancel button...

}


	void
PluginKateTextFilter::slotFilterCloseStdin (KProcess * pProcess)
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

  QString text ( KatePrompt ( i18n("Filter"),
                        i18n("Enter command to pipe selected text through:"),
                        (QWidget*)  kv,
			&completionList
                        ) );

  if ( !text.isEmpty () )
    runFilter( kv, text );
}

void PluginKateTextFilter::runFilter( KTextEditor::View *kv, const QString &filter )
{
  m_strFilterOutput = "";

  if (!m_pFilterShellProcess)
  {
    m_pFilterShellProcess = new KShellProcess;

    connect ( m_pFilterShellProcess, SIGNAL(wroteStdin(KProcess *)),
              this, SLOT(slotFilterCloseStdin (KProcess *)));

    connect ( m_pFilterShellProcess, SIGNAL(receivedStdout(KProcess*,char*,int)),
              this, SLOT(slotFilterReceivedStdout(KProcess*,char*,int)) );

    connect ( m_pFilterShellProcess, SIGNAL(receivedStderr(KProcess*,char*,int)),
              this, SLOT(slotFilterReceivedStderr(KProcess*,char*,int)) );

    connect ( m_pFilterShellProcess, SIGNAL(processExited(KProcess*)),
              this, SLOT(slotFilterProcessExited(KProcess*) ) ) ;
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
