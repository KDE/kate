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

#include "kateprojectworker.h"
#include "kateproject.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QTime>

KateProjectWorker::KateProjectWorker (QObject *project)
  : QObject ()
  , m_project (project)
{
}

KateProjectWorker::~KateProjectWorker ()
{
}

void KateProjectWorker::loadProject (QString baseDir, QVariantMap projectMap)
{
  /**
   * setup project base directory
   * this should be FIX after initial setting
   */
  Q_ASSERT (m_baseDir.isEmpty() || (m_baseDir == baseDir));
  m_baseDir = baseDir;

  /**
   * Create dummy top level parent item and empty map inside shared pointers
   * then load the project recursively
   */
  KateProjectSharedQStandardItem topLevel (new QStandardItem ());
  KateProjectSharedQMapStringItem file2Item (new QMap<QString, QStandardItem *> ());
  loadProject (topLevel.data(), projectMap, file2Item.data());

  /**
   * create some local backup of some data we need for further processing!
   */
  QStringList files = file2Item->keys ();

  /**
   * feed back our results
   */
  QMetaObject::invokeMethod (m_project, "loadProjectDone", Qt::QueuedConnection, Q_ARG(KateProjectSharedQStandardItem, topLevel), Q_ARG(KateProjectSharedQMapStringItem, file2Item));

  /**
   * load index
   */
  loadIndex (files);
}

void KateProjectWorker::loadProject (QStandardItem *parent, const QVariantMap &project, QMap<QString, QStandardItem *> *file2Item)
{
  /**
   * recurse to sub-projects FIRST
   */
  QVariantList subGroups = project["projects"].toList ();
  foreach (const QVariant &subGroupVariant, subGroups) {
    /**
     * convert to map and get name, else skip
     */
    QVariantMap subProject = subGroupVariant.toMap ();
    if (subProject["name"].toString().isEmpty())
      continue;

    /**
     * recurse
     */
    QStandardItem *subProjectItem = new KateProjectItem (KateProjectItem::Project, subProject["name"].toString());
    loadProject (subProjectItem, subProject, file2Item);
    parent->appendRow (subProjectItem);
  }

  /**
   * load all specified files
   */
  QVariantList files = project["files"].toList ();
  foreach (const QVariant &fileVariant, files)
    loadFilesEntry (parent, fileVariant.toMap (), file2Item);
}

/**
 * small helper to construct directory parent items
 * @param dir2Item map for path => item
 * @param path current path we need item for
 * @return correct parent item for given path, will reuse existing ones
 */
static QStandardItem *directoryParent (QMap<QString, QStandardItem *> &dir2Item, QString path)
{
  /**
   * throw away simple /
   */
  if (path == "/")
    path = "";

  /**
   * quick check: dir already seen?
   */
  if (dir2Item.contains (path))
    return dir2Item[path];

  /**
   * else: construct recursively
   */
  int slashIndex = path.lastIndexOf ('/');

  /**
   * no slash?
   * simple, no recursion, append new item toplevel
   */
  if (slashIndex < 0) {
    dir2Item[path] = new KateProjectItem (KateProjectItem::Directory, path);
    dir2Item[""]->appendRow (dir2Item[path]);
    return dir2Item[path];
  }

  /**
   * else, split and recurse
   */
  QString leftPart = path.left (slashIndex);
  QString rightPart = path.right (path.size() - (slashIndex + 1));

  /**
   * special handling if / with nothing on one side are found
   */
  if (leftPart.isEmpty() || rightPart.isEmpty ())
    return directoryParent (dir2Item, leftPart.isEmpty() ? rightPart : leftPart);

  /**
   * else: recurse on left side
   */
  dir2Item[path] = new KateProjectItem (KateProjectItem::Directory, rightPart);
  directoryParent (dir2Item, leftPart)->appendRow (dir2Item[path]);
  return dir2Item[path];
}

void KateProjectWorker::loadFilesEntry (QStandardItem *parent, const QVariantMap &filesEntry, QMap<QString, QStandardItem *> *file2Item)
{
  /**
   * get directory to open or skip
   */
  QDir dir (m_baseDir);
  if (!dir.cd (filesEntry["directory"].toString()))
    return;

  /**
   * get recursive attribute, default is TRUE
   */
  const bool recursive = !filesEntry.contains ("recursive") || filesEntry["recursive"].toBool();

  /**
   * now: choose between different methodes to get files in the directory
   */
  QStringList files;

  /**
   * use GIT
   */
  if (filesEntry["git"].toBool()) {
    /**
     * try to run git with ls-files for this directory
     */
    QProcess git;
    git.setWorkingDirectory (dir.absolutePath());
    QStringList args;
    args << "ls-files" << ".";
    git.start("git", args);
    if (!git.waitForStarted() || !git.waitForFinished())
      return;

    /**
     * get output and split up into files
     */
    QStringList relFiles = QString::fromLocal8Bit (git.readAllStandardOutput ()).split (QRegExp("[\n\r]"), QString::SkipEmptyParts);

    /**
     * prepend the directory path
     */
    foreach (QString relFile, relFiles) {
      /**
       * skip non-direct files if not recursive
       */
      if (!recursive && (relFile.indexOf ("/") != -1))
        continue;

      files.append (dir.absolutePath() + "/" + relFile);
    }
  }

  /**
   * use SVN
   */
  else if (filesEntry["svn"].toBool()) {
    /**
     * try to run git with ls-files for this directory
     */
    QProcess svn;
    svn.setWorkingDirectory (dir.absolutePath());
    QStringList args;
    args << "status" << "--verbose" << ".";
    if (recursive)
      args << "--depth=infinity";
    else
      args << "--depth=files";
    svn.start("svn", args);
    if (!svn.waitForStarted() || !svn.waitForFinished())
      return;

    /**
     * get output and split up into lines
     */
    QStringList lines = QString::fromLocal8Bit (svn.readAllStandardOutput ()).split (QRegExp("[\n\r]"), QString::SkipEmptyParts);

    /**
     * remove start of line that is no filename, sort out unknown and ignore
     */
    bool first = true;
    int prefixLength = -1;
    foreach (QString line, lines) {
        /**
         * get length of stuff to cut
         */
        if (first) {
          /**
           * try to find ., else fail
           */
          prefixLength = line.lastIndexOf (".");
          if (prefixLength < 0)
                break;

          /**
           * skip first
           */
          first = false;
          continue;
        }

        /**
         * get file, if not unknown or ignored
         * prepend directory path
         */
        if ((line.size() > prefixLength) && line[0] != '?' && line[0] != 'I')
          files.append (dir.absolutePath() + "/" + line.right (line.size() - prefixLength));
    }
  }

  else {
    files = filesEntry["list"].toStringList();

    /**
    * fallback to use QDirIterator and search files ourself!
    */
    if (files.empty()) {
      /**
      * default filter: only files!
      */
      dir.setFilter (QDir::Files);

      /**
      * set name filters, if any
      */
      QStringList filters = filesEntry["filters"].toStringList();
      if (!filters.isEmpty())
        dir.setNameFilters (filters);

      /**
      * construct flags for iterator
      */
      QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
      if (recursive)
        flags = flags | QDirIterator::Subdirectories;

      /**
      * create iterator and collect all files
      */
      QDirIterator dirIterator (dir, flags);
      while (dirIterator.hasNext()) {
        dirIterator.next();
        files.append (dirIterator.filePath());
      }
    }
  }

  /**
   * sort them
   */
  files.sort ();

  /**
   * construct paths first in tree and items in a map
   */
  QMap<QString, QStandardItem *> dir2Item;
  dir2Item[""] = parent;
  QList<QPair<QStandardItem *, QStandardItem *> > item2ParentPath;
  foreach (QString filePath, files) {
    /**
     * get file info and skip NON-files
     */
    QFileInfo fileInfo (filePath);
    if (!fileInfo.isFile())
      continue;

    /**
      * skip dupes
      */
     if (file2Item->contains(filePath))
       continue;

     /**
      * construct the item with right directory prefix
      * already hang in directories in tree
      */
     QStandardItem *fileItem = new KateProjectItem (KateProjectItem::File, fileInfo.fileName());
     fileItem->setData(filePath,Qt::ToolTipRole);
     item2ParentPath.append (QPair<QStandardItem *, QStandardItem *>(fileItem, directoryParent(dir2Item, dir.relativeFilePath (fileInfo.absolutePath()))));
     fileItem->setData (filePath, Qt::UserRole);
     (*file2Item)[filePath] = fileItem;
  }

  /**
   * plug in the file items to the tree
   */
  QList<QPair<QStandardItem *, QStandardItem *> >::const_iterator i = item2ParentPath.constBegin();
  while (i != item2ParentPath.constEnd()) {
    i->second->appendRow (i->first);
    ++i;
  }
}

void KateProjectWorker::loadIndex (const QStringList &files)
{
  /**
   * create new index, this will do the loading in the constructor
   * wrap it into shared pointer for transfer to main thread
   */
  KateProjectSharedProjectIndex index (new KateProjectIndex(files));

  /**
   * send new index object back to project
   */
  QMetaObject::invokeMethod (m_project, "loadIndexDone", Qt::QueuedConnection, Q_ARG(KateProjectSharedProjectIndex, index));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
