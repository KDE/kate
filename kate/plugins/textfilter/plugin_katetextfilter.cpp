/***************************************************************************
                          plugin_katetextfilter.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger <jowenn@bigfoot.com>
    copyright            : (C) 2009 Dominik Haumann <dhaumann kde org>
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

#include "ui_textfilterwidget.h"

#include <ktexteditor/editor.h>

#include <kdialog.h>

#include <kaction.h>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <cassert>
#include <kdebug.h>
#include <qstring.h>
#include <klineedit.h>
#include <kinputdialog.h>
#include <k3process.h>

#include <kgenericfactory.h>
#include <kauthorized.h>
#include <kactioncollection.h>
K_EXPORT_COMPONENT_FACTORY(katetextfilterplugin, KGenericFactory<PluginKateTextFilter>("katetextfilter"))

PluginViewKateTextFilter::PluginViewKateTextFilter(PluginKateTextFilter *plugin,
                                                   Kate::MainWindow *mainwindow)
  : Kate::PluginView(mainwindow)
  , KXMLGUIClient()
  , m_plugin(plugin)
{
  setComponentData (KComponentData("kate"));

  KAction *a = actionCollection()->addAction("edit_filter");
  a->setText(i18n("Filter Te&xt..."));
  a->setShortcut(Qt::CTRL + Qt::Key_Backslash);
  connect(a, SIGNAL(triggered(bool)), plugin, SLOT(slotEditFilter()));

  setXMLFile("plugins/katetextfilter/ui.rc");
  mainwindow->guiFactory()->addClient (this);
}

PluginViewKateTextFilter::~PluginViewKateTextFilter()
{
  mainWindow()->guiFactory()->removeClient (this);
}

PluginKateTextFilter::PluginKateTextFilter(QObject* parent, const QStringList&)
  : Kate::Plugin((Kate::Application *)parent, "kate-text-filter-plugin")
  , KTextEditor::Command()
  , m_pFilterShellProcess(NULL)
{
  KTextEditor::CommandInterface* cmdIface =
    qobject_cast<KTextEditor::CommandInterface*>(application()->editor());

  if (cmdIface) {
    cmdIface->registerCommand(this);
  }
}

PluginKateTextFilter::~PluginKateTextFilter()
{
  delete m_pFilterShellProcess;
  KTextEditor::CommandInterface* cmdIface =
    qobject_cast<KTextEditor::CommandInterface*>(application()->editor());

  if (cmdIface) {
    cmdIface->unregisterCommand(this);
  }
}


Kate::PluginView *PluginKateTextFilter::createView (Kate::MainWindow *mainWindow)
{
  return new PluginViewKateTextFilter(this, mainWindow);
}

void PluginKateTextFilter::slotFilterReceivedStdout(K3Process * pProcess, char * got, int len)
{
  Q_ASSERT(pProcess == m_pFilterShellProcess);

  if (got && len) {
    m_strFilterOutput += QString::fromLocal8Bit(got, len);
  }
}


void PluginKateTextFilter::slotFilterReceivedStderr (K3Process * pProcess, char * got, int len)
{
  slotFilterReceivedStdout (pProcess, got, len);
}


void PluginKateTextFilter::slotFilterProcessExited (K3Process * pProcess)
{
  Q_ASSERT(pProcess == m_pFilterShellProcess);

  KTextEditor::View * kv (application()->activeMainWindow()->activeView());
  if (!kv) return;
  kv->document()->startEditing();

  KTextEditor::Cursor start = kv->selectionRange().start();
  if (kv->selection()) {
    kv->removeSelectionText();
  }

  kv->setCursorPosition(start); // for block selection

  kv -> insertText (m_strFilterOutput);
  kv->document()->endEditing();
  m_strFilterOutput = "";
}


static void slipInFilter (K3ShellProcess & shell, KTextEditor::View & view, QString command)
{
  if(!view.selection()) return;
  QString marked = view.selectionText ();
  if(marked.isEmpty()) return;

  shell.clearArguments ();
  shell << command;

  shell.start(K3Process::NotifyOnExit, K3Process::All);
  QByteArray encoded = marked.toLocal8Bit ();
  shell.writeStdin (encoded, encoded.length ());
  //  TODO: Put up a modal dialog to defend the text from further
  //  keystrokes while the command is out. With a cancel button...
}


void PluginKateTextFilter::slotFilterCloseStdin (K3Process * pProcess)
{
  Q_ASSERT(pProcess == m_pFilterShellProcess);
  pProcess -> closeStdin ();
}


void PluginKateTextFilter::slotEditFilter()
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

  bool ok = false;

  KDialog *dialog = new KDialog(application()->activeMainWindow()->window());
  dialog->setCaption("Text Filter");
  dialog->setButtons(KDialog::Cancel | KDialog::Ok);
  dialog->setDefaultButton(KDialog::Ok);

  QWidget* widget = new QWidget(dialog);
  Ui::TextFilterWidget ui;
  ui.setupUi(widget);
  ui.filterBox->setFocus();
  dialog->setMainWidget(widget);

  KConfigGroup config(KGlobal::config(), "PluginTextFilter");
  QStringList items = config.readEntry("Completion list", QStringList());
  ui.filterBox->setMaxCount(10);
  ui.filterBox->setHistoryItems(items, true);

  connect(ui.filterBox, SIGNAL(activated(const QString&)), dialog, SIGNAL(okClicked()));

  if (dialog->exec() == QDialog::Accepted) {
    const QString filter = ui.filterBox->currentText();
    if (!filter.isEmpty()) {
      ui.filterBox->addToHistory(filter);
      config.writeEntry("Completion list", ui.filterBox->historyItems());
      runFilter(kv, filter);
    }
  }
}

void PluginKateTextFilter::runFilter(KTextEditor::View *kv, const QString &filter)
{
  m_strFilterOutput = "";

  if (!m_pFilterShellProcess)
  {
    m_pFilterShellProcess = new K3ShellProcess;

    connect (m_pFilterShellProcess, SIGNAL(wroteStdin(K3Process *)),
             this, SLOT(slotFilterCloseStdin (K3Process *)));

    connect (m_pFilterShellProcess, SIGNAL(receivedStdout(K3Process*,char*,int)),
             this, SLOT(slotFilterReceivedStdout(K3Process*,char*,int)));

    connect (m_pFilterShellProcess, SIGNAL(receivedStderr(K3Process*,char*,int)),
             this, SLOT(slotFilterReceivedStderr(K3Process*,char*,int)));

    connect (m_pFilterShellProcess, SIGNAL(processExited(K3Process*)),
             this, SLOT(slotFilterProcessExited(K3Process*))) ;
  }

  slipInFilter (*m_pFilterShellProcess, *kv, filter);
}

//BEGIN Kate::Command methods
const QStringList &PluginKateTextFilter::cmds()
{
  static QStringList dummy("textfilter");
  return dummy;
}

bool PluginKateTextFilter::help(KTextEditor::View *, const QString&, QString &msg)
{
  msg = i18n("<qt><p>Usage: <code>textfilter COMMAND</code></p>"
             "<p>Replace the selection with the output of the specified shell command.</p></qt>");
  return true;
}

bool PluginKateTextFilter::exec(KTextEditor::View *v, const QString &cmd, QString &msg)
{
  if (!v->selection()) {
    msg = i18n("You need to have a selection to use textfilter");
    return false;
  }

  QString filter = cmd.section(" ", 1).trimmed();

  if (filter.isEmpty()) {
    msg = i18n("Usage: textfilter COMMAND");
    return false;
  }

  runFilter(v, filter);
  return true;
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
