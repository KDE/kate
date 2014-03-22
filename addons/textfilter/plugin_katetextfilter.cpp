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
#include <ktexteditor/message.h>

#include <kdialog.h>
#include <QAction>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <klocalizedstring.h>
#include <cassert>
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
  KAction* a = actionCollection()->addAction("edit_filter");
  a->setText(i18n("Filter Te&xt..."));
  a->setShortcut(Qt::CTRL + Qt::Key_Backslash);
  connect(a, SIGNAL(triggered(bool)), plugin, SLOT(slotEditFilter()));

  mainwindow->guiFactory()->addClient(this);
}

PluginViewKateTextFilter::~PluginViewKateTextFilter()
{
  mainWindow()->guiFactory()->removeClient (this);
}

PluginKateTextFilter::PluginKateTextFilter(QObject* parent, const QVariantList&)
  : Kate::Plugin((Kate::Application *)parent, "kate-text-filter-plugin")
  , KTextEditor::Command(QStringList dummy("textfilter"))
  , m_pFilterProcess(NULL)
  , copyResult(false)
  , mergeOutput(true)
{
}

PluginKateTextFilter::~PluginKateTextFilter()
{
  delete m_pFilterProcess;
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
  const QString block = QString::fromLocal8Bit(m_pFilterProcess->readAllStandardError());
  if (mergeOutput)
    m_strFilterOutput += block;
  else
    m_stderrOutput += block;
}

void PluginKateTextFilter::slotFilterProcessExited(int, QProcess::ExitStatus)
{
  KTextEditor::View* kv(application()->activeMainWindow()->activeView());
  if (!kv) return;

  // Is there any error output to display?
  if (!mergeOutput && !m_stderrOutput.isEmpty())
  {
      QPointer<KTextEditor::Message> message = new KTextEditor::Message(
          i18nc(
              "@info"
            , "<title>Result of:</title><nl /><pre><code>$ %1\n<nl />%2</code></pre>"
            , m_last_command
            , m_stderrOutput
            )
        , KTextEditor::Message::Error
        );
      message->setWordWrap(true);
      message->setAutoHide(1000);
      kv->document()->postMessage(message);
  }

  if (copyResult) {
    QApplication::clipboard()->setText(m_strFilterOutput);
    return;
  }

  // Do not even try to change the document if no result collected...
  if (m_strFilterOutput.isEmpty())
    return;

  kv->document()->startEditing();

  KTextEditor::Cursor start = kv->cursorPosition();
  if (kv->selection()) {
    start = kv->selectionRange().start();
    kv->removeSelectionText();
  }

  kv->setCursorPosition(start); // for block selection

  kv->insertText(m_strFilterOutput);
  kv->document()->endEditing();
}


static void slipInFilter(KProcess & proc, KTextEditor::View & view, QString command)
{
  QString inputText;

  if (view.selection()) {
    inputText = view.selectionText();
  }

  proc.clearProgram ();
  proc.setShellCommand(command);

  proc.start();
  QByteArray encoded = inputText.toLocal8Bit();
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

  KTextEditor::View* kv(application()->activeMainWindow()->activeView());
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
  copyResult = config.readEntry("Copy result", false);
  mergeOutput = config.readEntry("Merge output", true);
  ui.filterBox->setMaxCount(10);
  ui.filterBox->setHistoryItems(items, true);
  ui.copyResult->setChecked(copyResult);
  ui.mergeOutput->setChecked(mergeOutput);

  connect(ui.filterBox, SIGNAL(activated(QString)), &dialog, SIGNAL(okClicked()));

  if (dialog.exec() == QDialog::Accepted) {
    copyResult = ui.copyResult->isChecked();
    mergeOutput = ui.mergeOutput->isChecked();
    const QString filter = ui.filterBox->currentText();
    if (!filter.isEmpty()) {
      ui.filterBox->addToHistory(filter);
      config.writeEntry("Completion list", ui.filterBox->historyItems());
      config.writeEntry("Copy result", copyResult);
      config.writeEntry("Merge output", mergeOutput);
      m_last_command = filter;
      runFilter(kv, filter);
    }
  }
}

void PluginKateTextFilter::runFilter(KTextEditor::View *kv, const QString &filter)
{
  m_strFilterOutput.clear();
  m_stderrOutput.clear();

  if (!m_pFilterProcess)
  {
    m_pFilterProcess = new KProcess;

    connect (m_pFilterProcess, SIGNAL(readyReadStandardOutput()),
             this, SLOT(slotFilterReceivedStdout()));

    connect (m_pFilterProcess, SIGNAL(readyReadStandardError()),
             this, SLOT(slotFilterReceivedStderr()));

    connect (m_pFilterProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
             this, SLOT(slotFilterProcessExited(int,QProcess::ExitStatus)));
  }
  m_pFilterProcess->setOutputChannelMode(
      mergeOutput ? KProcess::MergedChannels : KProcess::SeparateChannels
    );

  slipInFilter(*m_pFilterProcess, *kv, filter);
}

//BEGIN Kate::Command methods
bool PluginKateTextFilter::help(KTextEditor::View *, const QString&, QString &msg)
{
  msg = i18n("<qt><p>Usage: <code>textfilter COMMAND</code></p>"
             "<p>Replace the selection with the output of the specified shell command.</p></qt>");
  return true;
}

bool PluginKateTextFilter::exec(KTextEditor::View *v, const QString &cmd, QString &msg)
{
  QString filter = cmd.section(' ', 1).trimmed();

  if (filter.isEmpty()) {
    msg = i18n("Usage: textfilter COMMAND");
    return false;
  }

  runFilter(v, filter);
  return true;
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
