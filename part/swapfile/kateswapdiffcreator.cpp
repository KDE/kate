/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010-2012 Dominik Haumann <dhaumann kde org>
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

#include "kateswapdiffcreator.h"
#include "kateswapfile.h"
#include "katedocument.h"

#include <kprocess.h>
#include <kmessagebox.h>
#include <krun.h>
#include <klocale.h>

//BEGIN SwapDiffCreator
SwapDiffCreator::SwapDiffCreator(Kate::SwapFile* swapFile)
  : QObject (swapFile)
  , m_swapFile (swapFile)
  , m_proc(0)
{
}

SwapDiffCreator::~SwapDiffCreator ()
{
}

void SwapDiffCreator::viewDiff()
{
  QString path = m_swapFile->fileName();
  if (path.isNull())
    return;

  QFile swp(path);
  if (!swp.open(QIODevice::ReadOnly)) {
    kWarning( 13020 ) << "Can't open swap file";
    return;
  }

  // create all needed tempfiles
  m_originalFile.setSuffix(".original");
  m_recoveredFile.setSuffix(".recovered");
  m_diffFile.setSuffix(".diff");

  if (!m_originalFile.open() || !m_recoveredFile.open() || !m_diffFile.open()) {
    kWarning( 13020 ) << "Can't open temporary files needed for diffing";
    return;
  }

  // truncate files, just in case
  m_originalFile.resize (0);
  m_recoveredFile.resize (0);
  m_diffFile.resize (0);

  // create a document with the recovered data
  KateDocument recoverDoc;
  recoverDoc.setText(m_swapFile->document()->text());

  // store original text in a file as utf-8 and close it
  {
    QTextStream stream (&m_originalFile);
    stream.setCodec (QTextCodec::codecForName("UTF-8"));
    stream << recoverDoc.text ();
  }
  m_originalFile.close ();

  // recover data
  QDataStream stream(&swp);
  recoverDoc.swapFile()->recover(stream, false);

  // store recovered text in a file as utf-8 and close it
  {
    QTextStream stream (&m_recoveredFile);
    stream.setCodec (QTextCodec::codecForName("UTF-8"));
    stream << recoverDoc.text ();
  }
  m_recoveredFile.close ();

  // create a KProcess proc for diff
  m_proc = new KProcess(this);
  m_proc->setOutputChannelMode(KProcess::MergedChannels);
  *m_proc << "diff" << "-u" <<  m_originalFile.fileName() << m_recoveredFile.fileName();

  connect(m_proc, SIGNAL(readyRead()), this, SLOT(slotDataAvailable()));
  connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotDiffFinished()));

//   setCursor(Qt::WaitCursor);

  m_proc->start();

  QTextStream ts(m_proc);
  int lineCount = recoverDoc.lines();
  for (int line = 0; line < lineCount; ++line)
    ts << recoverDoc.line(line) << '\n';
  ts.flush();
  m_proc->closeWriteChannel();
}

void SwapDiffCreator::slotDataAvailable()
{
  // collect diff output
  m_diffFile.write (m_proc->readAll());
}

void SwapDiffCreator::slotDiffFinished()
{
  // collect last diff output, if any
  m_diffFile.write (m_proc->readAll());

  // get the exit status to check whether diff command run successfully
  const QProcess::ExitStatus es = m_proc->exitStatus();
  delete m_proc;
  m_proc = 0;

  // check exit status
  if (es != QProcess::NormalExit)
  {
    KMessageBox::sorry(0,
                      i18n("The diff command failed. Please make sure that "
                          "diff(1) is installed and in your PATH."),
                      i18n("Error Creating Diff"));
    deleteLater();
    return;
  }

  // sanity check: is there any diff content?
  if ( m_diffFile.size() == 0 )
  {
    KMessageBox::information(0,
                            i18n("The files are identical."),
                            i18n("Diff Output"));
    deleteLater();
    return;
  }

  // close diffFile and avoid removal, KRun will do that later!
  m_diffFile.close ();
  m_diffFile.setAutoRemove (false);

  // KRun::runUrl should delete the file, once the client exits
  KRun::runUrl (KUrl::fromPath(m_diffFile.fileName()), "text/x-patch", m_swapFile->document()->activeView(), true );

  deleteLater();
}

//END SwapDiffCreator

// kate: space-indent on; indent-width 2; replace-tabs on;
