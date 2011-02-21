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
#include <kprocess.h>

#include <kpluginfactory.h>
#include <kauthorized.h>
#include <kactioncollection.h>

#include <qapplication.h>
#include <qclipboard.h>

K_PLUGIN_FACTORY(PluginKateTextFilterFactory, registerPlugin<PluginKateTextFilter>();)
K_EXPORT_PLUGIN(PluginKateTextFilterFactory("katetextfilter"))

PluginViewKateTextFilter::PluginViewKateTextFilter(PluginKateTextFilter *plugin,
                                                   Kate::MainWindow *mainwindow)
  : Kate::PluginView(mainwindow)
  , Kate::XMLGUIClient(PluginKateTextFilterFactory::componentData())
  , m_plugin(plugin)
{
  KAction *a = actionCollection()->addAction("edit_filter");
  a->setText(i18n("Filter Te&xt..."));
  a->setShortcut(Qt::CTRL + Qt::Key_Backslash);
  connect(a, SIGNAL(triggered(bool)), plugin, SLOT(slotEditFilter()));

  mainwindow->guiFactory()->addClient (this);
}

PluginViewKateTextFilter::~PluginViewKateTextFilter()
{
  mainWindow()->guiFactory()->removeClient (this);
}

PluginKateTextFilter::PluginKateTextFilter(QObject* parent, const QVariantList&)
  : Kate::Plugin((Kate::Application *)parent, "kate-text-filter-plugin")
  , KTextEditor::Command()
  , m_pFilterProcess(NULL)
  , pasteResult(true)
{
  KTextEditor::CommandInterface* cmdIface =
    qobject_cast<KTextEditor::CommandInterface*>(application()->editor());

  if (cmdIface) {
    cmdIface->registerCommand(this);
  }
}

PluginKateTextFilter::~PluginKateTextFilter()
{
  delete m_pFilterProcess;
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

void PluginKateTextFilter::slotFilterReceivedStdout()
{
  m_strFilterOutput += QString::fromLocal8Bit(m_pFilterProcess->readAllStandardOutput());
}


void PluginKateTextFilter::slotFilterReceivedStderr ()
{
  m_strFilterOutput += QString::fromLocal8Bit(m_pFilterProcess->readAllStandardError());
}

void PluginKateTextFilter::slotFilterProcessExited (int exitCode, QProcess::ExitStatus exitStatus)
{
  KTextEditor::View * kv (application()->activeMainWindow()->activeView());
  if (!kv) return;

  if (!pasteResult) {
    QApplication::clipboard()->setText(m_strFilterOutput);
    return;
  }

  kv->document()->startEditing();

  KTextEditor::Cursor start = kv->cursorPosition();
  if (kv->selection()) {
    start = kv->selectionRange().start();
    kv->removeSelectionText();
  }

  kv->setCursorPosition(start); // for block selection

  kv->insertText(m_strFilterOutput);
  kv->document()->endEditing();
  m_strFilterOutput = "";
}


static void slipInFilter (KProcess & proc, KTextEditor::View & view, QString command)
{
  QString inputText;

  if (view.selection()) {
    inputText = view.selectionText ();
  }

  proc.clearProgram ();
  proc.setShellCommand(command);

  proc.start();
  QByteArray encoded = inputText.toLocal8Bit ();
  proc.write(encoded);
  proc.closeWriteChannel();
  //  TODO: Put up a modal dialog to defend the text from further
  //  keystrokes while the command is out. With a cancel button...
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

  KDialog dialog(application()->activeMainWindow()->window());
  dialog.setCaption("Text Filter");
  dialog.setButtons(KDialog::Cancel | KDialog::Ok);
  dialog.setDefaultButton(KDialog::Ok);

  QWidget* widget = new QWidget(&dialog);
  Ui::TextFilterWidget ui;
  ui.setupUi(widget);
  ui.filterBox->setFocus();
  dialog.setMainWidget(widget);

  KConfigGroup config(KGlobal::config(), "PluginTextFilter");
  QStringList items = config.readEntry("Completion list", QStringList());
  ui.filterBox->setMaxCount(10);
  ui.filterBox->setHistoryItems(items, true);

  connect(ui.filterBox, SIGNAL(activated(const QString&)), &dialog, SIGNAL(okClicked()));

  if (dialog.exec() == QDialog::Accepted) {
    pasteResult = !ui.checkBox->isChecked();
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

  if (!m_pFilterProcess)
  {
    m_pFilterProcess = new KProcess;
    m_pFilterProcess->setOutputChannelMode(KProcess::MergedChannels);

    connect (m_pFilterProcess, SIGNAL(readyReadStandardOutput()),
             this, SLOT(slotFilterReceivedStdout()));

    connect (m_pFilterProcess, SIGNAL(readyReadStandardError()),
             this, SLOT(slotFilterReceivedStderr()));

    connect (m_pFilterProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
             this, SLOT(slotFilterProcessExited(int, QProcess::ExitStatus))) ;
  }

  slipInFilter (*m_pFilterProcess, *kv, filter);
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
  QString filter = cmd.section(" ", 1).trimmed();

  if (filter.isEmpty()) {
    msg = i18n("Usage: textfilter COMMAND");
    return false;
  }

  pasteResult = true;
  runFilter(v, filter);
  return true;
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
