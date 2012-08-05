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

#include <QFile>

#include <qjson/parser.h>

KateProject::KateProject (QObject *parent)
  : QObject (parent)
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
  
  // now: get the data
  QVariantMap globalMap = project.toMap ();
  qDebug ("name %s", qPrintable(globalMap["name"].toString()));
  
  
  /**
   * done ok ;)
   */
  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
