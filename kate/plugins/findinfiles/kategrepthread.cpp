/* This file is part of the KDE project
   Copyright (C) 2007 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kategrepthread.h"
#include "kategrepthread.moc"

#include <kdebug.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

KateGrepThread::KateGrepThread(QWidget *parent, QWidget *parentTab,
                               const QString &dir, bool recursive,
                               const QStringList &fileWildcards,
                               const QList<QRegExp> &searchPattern)
    : QThread (parent), m_parentTab(parentTab), m_cancel (false)
    , m_recursive (recursive), m_fileWildcards (fileWildcards)
    , m_searchPattern (searchPattern)
{
  m_workQueue << dir;
}

KateGrepThread::~KateGrepThread ()
{}

void KateGrepThread::run ()
{
  // do the real work
  while (!m_cancel && !m_workQueue.isEmpty())
  {
    QDir currentDir (m_workQueue.takeFirst());

    // if not readable, skip it
    if (!currentDir.isReadable ())
      continue;

    // only add subdirs to worklist if we should do recursive search
    if (m_recursive)
    {
      // append all dirs to the workqueue
      QFileInfoList currentSubDirs = currentDir.entryInfoList (QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
  
      // append them to the workqueue, if readable
      for (int i = 0; i < currentSubDirs.size(); ++i)
        m_workQueue << currentSubDirs.at(i).absoluteFilePath ();
    }

    // work with all files in this dir..., use wildcards for them...
    QFileInfoList currentFiles = currentDir.entryInfoList (m_fileWildcards, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);

    // iterate over all files
    for (int i = 0; !m_cancel && i < currentFiles.size(); ++i)
      grepInFile (currentFiles.at(i).absoluteFilePath (), currentFiles.at(i).fileName());
  }
}

void KateGrepThread::grepInFile (const QString &fileName, const QString &baseName)
{
  QFile file (fileName);

  // can't read file, return
  if (!file.open(QFile::ReadOnly))
    return;

  // try to extract data
  QTextStream stream (&file);

  QStringList lines;
  int lineNumber = 0;
  while (!m_cancel && !stream.atEnd ())
  {
    // enough lines gathered, try to match them...
    if (lines.size() == m_searchPattern.size())
    {
      int firstColumn = -1;
      for (int i = 0; i < m_searchPattern.size(); ++i)
      {
        int column = m_searchPattern.at(i).indexIn (lines.at(i));
        if (column == -1)
        {
          firstColumn = -1; // reset firstColumn
          break;
        }
        else if (i == 0) // remember first column
          firstColumn = column;
      }

      // found match...
      if (firstColumn != -1)
      {
        kDebug () << "found match: " << fileName << " : " << lineNumber;
        emit foundMatch (fileName, lineNumber, firstColumn, baseName, lines.at (0), m_parentTab);
      }

      // remove first line...
      lines.pop_front ();
      ++lineNumber;
    }

    lines.append (stream.readLine());
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
