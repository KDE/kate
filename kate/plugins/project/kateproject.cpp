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
  
  qDebug ("name %s", qPrintable(m_name));
  
  /**
   * now, clear model once and load other stuff that is possible in all groups
   */
  m_model->clear ();
  loadGroup (m_model->invisibleRootItem(), globalGroup);
  
  /**
   * done ok ;)
   */
  return true;
}

void KateProject::loadGroup (QStandardItem *parent, const QVariantMap &group)
{
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
  
  /**
   * recurse to sub-groups
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
    QStandardItem *subGroupItem = new QStandardItem (subGroup["name"].toString());
    loadGroup (subGroupItem, subGroup);
    parent->appendRow (subGroupItem);
  }
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
   * create iterator
   */
  QDirIterator dirIterator (dir, flags);
  while (dirIterator.hasNext()) {
     dirIterator.next();
     QStandardItem *fileItem = new QStandardItem (dirIterator.fileName());
     parent->appendRow (fileItem);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
