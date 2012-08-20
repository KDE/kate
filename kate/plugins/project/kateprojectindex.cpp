/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "kateprojectindex.h"

#include <QProcess>
#include <QDir>

/**
 * include ctags reading
 */
#include "ctags/readtags.c"

KateProjectIndex::KateProjectIndex (const QStringList &files)
 : m_ctagsIndexFile (QDir::tempPath () + "/kate.project.ctags")
 , m_ctagsIndexHandle (0)
{
  /**
   * load ctags
   */
  loadCtags (files);
}

KateProjectIndex::~KateProjectIndex ()
{
  /**
   * delete ctags handle if any
   */
  if (m_ctagsIndexHandle) {
    tagsClose (m_ctagsIndexHandle);
    m_ctagsIndexHandle = 0;
  }
}

void KateProjectIndex::loadCtags (const QStringList &files)
{
  /**
   * create temporary file
   * if not possible, fail
   */
  if (!m_ctagsIndexFile.open ())
    return;
  
  /**
   * close file again, other process will use it
   */
  m_ctagsIndexFile.close ();
   
  /**
   * try to run ctags for all files in this project
   * output to our ctags index file
   */
  QProcess ctags;
  QStringList args;
  args << "-L" << "-" << "-f" << m_ctagsIndexFile.fileName() << "--fields=+z";
  ctags.start("ctags", args);
  if (!ctags.waitForStarted())
    return;
  
  /**
   * write files list and close write channel
   */
  ctags.write(files.join("\n").toLocal8Bit());
  ctags.closeWriteChannel();
    
  /**
   * wait for done
   */
  if (!ctags.waitForFinished())
    return;
  
  /**
   * try to open ctags file
   */
  tagFileInfo info;
  memset (&info, 0, sizeof (tagFileInfo));
  m_ctagsIndexHandle = tagsOpen (m_ctagsIndexFile.fileName().toLocal8Bit(), &info);
}

void KateProjectIndex::completionMatches (QStandardItemModel &model, KTextEditor::View *view, const KTextEditor::Range & range)
{
  /**
   * abort if no ctags index
   */
  if (!m_ctagsIndexHandle)
    return;
  
  /**
   * word to complete
   * abort if empty
   */
  QByteArray word = view->document()->text(range).toLocal8Bit();
  if (word.isEmpty())
    return;
 
  /**
   * try to search entry
   * fail if none found
   */
  tagEntry entry;
  if (tagsFind (m_ctagsIndexHandle, &entry, word.constData(), TAG_PARTIALMATCH  | TAG_OBSERVECASE) != TagSuccess)
    return;
  
  /**
   * loop over all found tags
   * first one is filled by above find, others by find next
   */
  do {
    /**
     * add new completion item
     */
    model.appendRow (new QStandardItem (QString::fromLocal8Bit(entry.name)));
  printf ("project info created %s\n", entry.name);
  } while (tagsFindNext (m_ctagsIndexHandle, &entry) == TagSuccess);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
