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

#include "kateproject.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <KMimeType>
#include <KIconLoader>

#include <qjson/parser.h>

KateProject::KateProject ()
  : QObject ()
  , m_model (new QStandardItemModel (this))
{
}

KateProject::~KateProject ()
{
}

bool KateProject::load (const QString &fileName)
{
  /**
   * bail out if already fileName set!
   */
  if (!m_fileName.isEmpty())
    return false;

  /**
   * set new filename
   */
  m_fileName = fileName;

  /**
   * trigger reload
   */
  return reload ();
}

bool KateProject::reload ()
{
  /**
   * open the file for reading, bail out on error!
   */
  QFile file (m_fileName);
  if (!file.open (QFile::ReadOnly))
    return false;

  /**
   * parse the whole file, bail out again on error!
   */
  bool ok = true;
  QJson::Parser parser;
  QVariant project = parser.parse (&file, &ok);
  if (!ok)
    return false;

  /**
   * now: get global group
   */
  QVariantMap globalGroup = project.toMap ();

  /**
   * no name, bad => bail out
   */
  if (globalGroup["name"].toString().isEmpty())
    return false;

  /**
   * setup global attributes in this object
   */
  m_name = globalGroup["name"].toString();

  /**
   * now, clear model once and load other stuff that is possible in all groups
   */
  m_model->clear ();
  m_files.clear ();
  loadGroup (m_model->invisibleRootItem(), globalGroup);

  /**
   * done ok ;)
   */
  return true;
}

void KateProject::loadGroup (QStandardItem *parent, const QVariantMap &group)
{
  /**
   * recurse to sub-groups FIRST
   */
  QVariantList subGroups = group["groups"].toList ();
  foreach (const QVariant &subGroupVariant, subGroups) {
    /**
     * convert to map and get name, else skip
     */
    QVariantMap subGroup = subGroupVariant.toMap ();
    if (subGroup["name"].toString().isEmpty())
      continue;

    /**
     * recurse
     */
    QStandardItem *subGroupItem = new QStandardItem (QIcon (KIconLoader::global ()->loadIcon ("folder-documents", KIconLoader::Small)), subGroup["name"].toString());
    loadGroup (subGroupItem, subGroup);
    parent->appendRow (subGroupItem);
  }

  /**
   * load all specified files
   */
  QVariantList files = group["files"].toList ();
  foreach (const QVariant &fileVariant, files) {
    /**
     * convert to map
     */
    QVariantMap file = fileVariant.toMap ();

    /**
     * now, which kind of file spec we have?
     */

    /**
     * directory: xxx?
     */
    if (!file["directory"].toString().isEmpty()) {
      loadDirectory (parent, file);
      continue;
    }
  }
}

/**
 * small helper to construct directory parent items
 * @param dir2Item map for path => item
 * @param dirIcon directory icon
 * @param path current path we need item for
 * @return correct parent item for given path, will reuse existing ones
 */
static QStandardItem *directoryParent (QMap<QString, QStandardItem *> &dir2Item, const QIcon &dirIcon, QString path)
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
    dir2Item[path] = new QStandardItem (dirIcon, path);
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
    return directoryParent (dir2Item, dirIcon, leftPart.isEmpty() ? rightPart : leftPart);

  /**
   * else: recurse on left side
   */
  dir2Item[path] = new QStandardItem (dirIcon, rightPart);
  directoryParent (dir2Item, dirIcon, leftPart)->appendRow (dir2Item[path]);
  return dir2Item[path];
}

void KateProject::loadDirectory (QStandardItem *parent, const QVariantMap &directory)
{
  /**
   * get directory to open or skip
   */
  QDir dir (QFileInfo (m_fileName).absoluteDir());
  if (!dir.cd (directory["directory"].toString()))
    return;

  /**
   * default filter: only files!
   */
  dir.setFilter (QDir::Files);

  /**
   * set name filters, if any
   */
  QStringList filters = directory["filters"].toStringList();
  if (!filters.isEmpty())
    dir.setNameFilters (filters);

  /**
   * construct flags for iterator
   */
  QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
  if (directory["recursive"].toBool())
    flags = flags | QDirIterator::Subdirectories;

  /**
   * create iterator and collect all files
   */
  QDirIterator dirIterator (dir, flags);
  QStringList files;
  while (dirIterator.hasNext()) {
     dirIterator.next();
     files.append (dirIterator.filePath());
  }

  /**
   * sort them
   */
  files.sort ();

  /**
   * append to FULL files list
   */
  m_files += files;

  /**
   * construct paths first in tree and items in a map
   */
  QMap<QString, QStandardItem *> dir2Item;
  dir2Item[""] = parent;
  QIcon dirIcon (KIconLoader::global ()->loadIcon ("folder", KIconLoader::Small));
  QList<QPair<QStandardItem *, QStandardItem *> > item2ParentPath;
  foreach (QString filePath, files) {
     /**
      * get the right icon for the file
      */
     QString iconName = KMimeType::iconNameForUrl(KUrl::fromPath(filePath));
     QIcon icon (KIconLoader::global ()->loadMimeTypeIcon (iconName, KIconLoader::Small));

     /**
      * construct the item with right directory prefix
      * already hang in directories in tree
      */
     QFileInfo fileInfo (filePath);
     QStandardItem *fileItem = new QStandardItem (icon, fileInfo.fileName());
     item2ParentPath.append (QPair<QStandardItem *, QStandardItem *>(fileItem, directoryParent(dir2Item, dirIcon, dir.relativeFilePath (fileInfo.absolutePath()))));
     fileItem->setData (filePath, Qt::UserRole);
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

// kate: space-indent on; indent-width 2; replace-tabs on;
