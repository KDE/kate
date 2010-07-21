/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *  Copyright (C) 2010 Diana-Victoria Tiriplica <diana.tiriplica@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katerecoverbar.h"
#include "ui_recoverwidget.h"
#include "kateswapfile.h"
#include "kateview.h"
#include "katedocument.h"

#include <kprocess.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <krun.h>

#include <QWhatsThis>

//BEGIN KateRecoverBar
KateRecoverBar::KateRecoverBar(KateView *view, QWidget *parent)
  : KateViewBarWidget( false, parent )
  , m_view ( view )
  , m_ui (new Ui::RecoverWidget())
  , m_proc(0)
{
  m_ui->setupUi( centralWidget() );
  
  // clicking on the "Help" link pops up the content as what's this
  connect(m_ui->lblSwap, SIGNAL(linkActivated(const QString&)),
          this, SLOT(showWhatsThis(const QString&)));

  // set icons, but keep text from ui file
  m_ui->btnRecover->setGuiItem(KGuiItem(m_ui->btnRecover->text(), KIcon("edit-redo")));
  m_ui->btnDiscard->setGuiItem(KStandardGuiItem::discard());
  m_ui->lblIcon->setPixmap(KIcon("dialog-warning").pixmap(64, 64));

  // use queued connections because this (all) KateRecoverBar widgets are deleted
  connect(m_ui->btnRecover, SIGNAL(clicked()), m_view->doc()->swapFile(), SLOT(recover()), Qt::QueuedConnection);
  connect(m_ui->btnDiscard, SIGNAL(clicked()), m_view->doc()->swapFile(), SLOT(discard()), Qt::QueuedConnection);
  connect(m_ui->btnDiff, SIGNAL(clicked()), this, SLOT(viewDiff()));
}

KateRecoverBar::~KateRecoverBar ()
{
  delete m_ui;
}

void KateRecoverBar::showWhatsThis(const QString& text)
{
  QWhatsThis::showText(QCursor::pos(), text);
}

void KateRecoverBar::viewDiff()
{
  // create a document with the recovered data
  KateDocument recoverDoc;
  recoverDoc.setText(m_view->doc()->text());

  QString path = m_view->doc()->swapFile()->fileName();

  if (path.isNull())
    return;

  QFile swp(path);
  if (!swp.open(QIODevice::ReadOnly)) {
    kWarning( 13020 ) << "Can't open swap file";
    return;
  }

  QDataStream stream(&swp);

  recoverDoc.swapFile()->recover(stream);

  // create a KProcess proc
  m_proc = new KProcess(this);
  m_proc->setOutputChannelMode(KProcess::MergedChannels);
  *m_proc << "diff" << "-u" << "-" <<  m_view->doc()->url().toLocalFile();

  connect(m_proc, SIGNAL(readyRead()), this, SLOT(slotDataAvailable()));
  connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotDiffFinished()));

  setCursor(Qt::WaitCursor);

  // disable the "View Changes" button, so the user won't click it twice
  m_ui->btnDiff->setEnabled(false);

  m_diffContent.clear();

  m_proc->start();

  QTextStream ts(m_proc);
  int lineCount = recoverDoc.lines();
  for (int line = 0; line < lineCount; ++line)
    ts << recoverDoc.line(line) << '\n';
  ts.flush();
  m_proc->closeWriteChannel();
}

void KateRecoverBar::slotDataAvailable()
{
  // collect diff output
  m_diffContent += m_proc->readAll();
}

void KateRecoverBar::slotDiffFinished()
{
  m_ui->btnDiff->setEnabled(true);
  unsetCursor();

  // get the exit status to check whether diff command run successfully
  const QProcess::ExitStatus es = m_proc->exitStatus();
  delete m_proc;
  m_proc = 0;

  KTemporaryFile tempFile;
  tempFile.setSuffix(".diff");
  if (!tempFile.open()) {
    kWarning( 13020 ) << "Can't open diff temporary file";
    return;
  }

  // write the buffered data to the temporary file
  tempFile.write(m_diffContent);

  // check exit status
  if (es != QProcess::NormalExit)
  {
    KMessageBox::sorry(this,
                      i18n("The diff command failed. Please make sure that "
                          "diff(1) is installed and in your PATH."),
                      i18n("Error Creating Diff"));
    return;
  }

  // sanity check: is there any diff content?
  if ( tempFile.size() == 0 )
  {
    KMessageBox::information(this,
                            i18n("The files are identical."),
                            i18n("Diff Output"));
    return;
  }

  tempFile.setAutoRemove(false);
  KUrl url = KUrl::fromPath(tempFile.fileName());

  // KRun::runUrl should delete the file, once the client exits
  KRun::runUrl(url, "text/x-patch", this, true );
}

//END KateRecoverBar

// kate: space-indent on; indent-width 2; replace-tabs on;
