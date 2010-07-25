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
#include "outputstyle.h"

#include <kcolorscheme.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kconfiggroup.h>
#include <kglobalsettings.h>

DataOutputModel::DataOutputModel(QObject *parent)
: QSqlQueryModel(parent)
{
  m_styles.insert("text",     new OutputStyle());
  m_styles.insert("number",   new OutputStyle());
  m_styles.insert("null",     new OutputStyle());
  m_styles.insert("blob",     new OutputStyle());
  m_styles.insert("datetime", new OutputStyle());
  m_styles.insert("bool",     new OutputStyle());

  readConfig();
}


DataOutputModel::~DataOutputModel()
{
  qDeleteAll(m_styles);
}


void DataOutputModel::readConfig()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  KConfigGroup group = config.group("OutputCustomization");

  KColorScheme scheme(QPalette::Active, KColorScheme::View);

  foreach(QString k, m_styles.keys())
  {
    OutputStyle *s = m_styles[k];

    KConfigGroup g = group.group(k);

    s->foreground = scheme.foreground();
    s->background = scheme.background();
    s->font = KGlobalSettings::generalFont();

    QFont dummy = g.readEntry("font", KGlobalSettings::generalFont());

    s->font.setBold(dummy.bold());
    s->font.setItalic(dummy.italic());
    s->font.setUnderline(dummy.underline());
    s->font.setStrikeOut(dummy.strikeOut());
    s->foreground.setColor(g.readEntry("foregroundColor", s->foreground.color()));
    s->background.setColor(g.readEntry("backgroundColor", s->background.color()));
  }
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
    if (role == Qt::FontRole)
      return QVariant(m_styles.value("null")->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value("null")->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value("null")->background);
    if (role == Qt::DisplayRole)
      return QVariant("NULL");
  }

  if (value.type() == QVariant::ByteArray)
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value("blob")->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value("blob")->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value("blob")->background);
    if (role == Qt::DisplayRole)
      return QVariant(value.toByteArray().left(255));

    return QSqlQueryModel::data(index, role);
  }

  if (isNumeric(value.type()))
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value("number")->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value("number")->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value("number")->background);
    if (role == Qt::TextAlignmentRole)
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::DisplayRole)
      return QVariant(value.toString());
  }

  if (value.type() == QVariant::Bool)
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value("bool")->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value("bool")->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value("bool")->background);
    if (role == Qt::DisplayRole)
      return QVariant(value.toBool() ? "True" : "False");
  }

  if (value.type() == QVariant::Date ||
      value.type() == QVariant::Time ||
      value.type() == QVariant::DateTime )
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value("datetime")->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value("datetime")->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value("datetime")->background);
  }

  if (role == Qt::FontRole)
    return QVariant(m_styles.value("text")->font);
  if (role == Qt::ForegroundRole)
    return QVariant(m_styles.value("text")->foreground);
  if (role == Qt::BackgroundRole)
    return QVariant(m_styles.value("text")->background);
  if (role == Qt::TextAlignmentRole)
    return QVariant(Qt::AlignVCenter);

  return QSqlQueryModel::data(index, role);
}
