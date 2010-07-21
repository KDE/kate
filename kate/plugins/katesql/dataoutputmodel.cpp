/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

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

#include "dataoutputmodel.h"

#include <kdebug.h>
#include <kglobal.h>
#include <kconfiggroup.h>
#include <kglobalsettings.h>

DataOutputModel::DataOutputModel(QObject *parent)
: QSqlQueryModel(parent)
{
  readConfig();

  m_nullFont = KGlobalSettings::generalFont();
  m_nullFont.setItalic(true);
}


void DataOutputModel::readConfig()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  m_nullBackgroundColor = config.readEntry("NullBackgroundColor", QColor::fromRgb(255,255,191));
  m_blobBackgroundColor = config.readEntry("BlobBackgroundColor", QColor::fromRgb(255,255,191));
}


bool DataOutputModel::isNumeric(QVariant::Type type) const
{
  return (type > 1 && type < 7);
}


QVariant DataOutputModel::data(const QModelIndex &index, int role) const
{
  // provide quickly the raw value
  // used by operations that works on large amount of data, like copy or export
  if (role == Qt::UserRole)
    return QSqlQueryModel::data(index, Qt::DisplayRole);

  QVariant value(QSqlQueryModel::data(index, Qt::DisplayRole));

  if (value.isNull())
  {
    if (role == Qt::BackgroundColorRole)
      return QVariant(m_nullBackgroundColor);
    if (role == Qt::DisplayRole)
      return QVariant("NULL");
    if (role == Qt::FontRole)
      return QVariant(m_nullFont);
  }

  if (value.type() == QVariant::ByteArray)
  {
    if (role == Qt::BackgroundColorRole)
      return QVariant(m_blobBackgroundColor);

    if (role == Qt::DisplayRole)
      return QVariant(value.toByteArray().left(255));
//       return QVariant("<...>");

    return QSqlQueryModel::data(index, role);
  }

//   if (role == Qt::FontRole)
//     return QVariant(KGlobalSettings::fixedFont());

  if (role == Qt::TextAlignmentRole)
  {
    if (isNumeric(value.type()))
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    return QVariant(Qt::AlignVCenter);
  }

  if (role == Qt::DisplayRole && isNumeric(value.type()))
    return QVariant(value.toString());

  if (role == Qt::DisplayRole && value.type() == QVariant::Bool)
    return QVariant(value.toBool() ? "True" : "False");

  return QSqlQueryModel::data(index, role);
}
