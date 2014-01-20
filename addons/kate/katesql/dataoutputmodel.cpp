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
#include <klocalizedstring.h>
#include <kconfiggroup.h>
#include <KSharedConfig>

#include <QFontDatabase>
#include <QLocale>

inline bool isNumeric(const QVariant::Type type)
{
  return (type > 1 && type < 7);
}


DataOutputModel::DataOutputModel(QObject *parent)
: CachedSqlQueryModel(parent, 1000)
{
  m_useSystemLocale = false;

  m_styles.insert(QLatin1String ("text"),     new OutputStyle());
  m_styles.insert(QLatin1String ("number"),   new OutputStyle());
  m_styles.insert(QLatin1String ("null"),     new OutputStyle());
  m_styles.insert(QLatin1String ("blob"),     new OutputStyle());
  m_styles.insert(QLatin1String ("datetime"), new OutputStyle());
  m_styles.insert(QLatin1String ("bool"),     new OutputStyle());

  readConfig();
}


DataOutputModel::~DataOutputModel()
{
  qDeleteAll(m_styles);
}


void DataOutputModel::clear()
{
  beginResetModel();

  CachedSqlQueryModel::clear();

  endResetModel();
}


void DataOutputModel::readConfig()
{
  KConfigGroup config(KSharedConfig::openConfig(), "KateSQLPlugin");

  KConfigGroup group = config.group("OutputCustomization");

  KColorScheme scheme(QPalette::Active, KColorScheme::View);

  foreach(const QString& k, m_styles.keys())
  {
    OutputStyle *s = m_styles[k];

    KConfigGroup g = group.group(k);

    s->foreground = scheme.foreground();
    s->background = scheme.background();
    s->font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);

    QFont dummy = g.readEntry("font", QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    s->font.setBold(dummy.bold());
    s->font.setItalic(dummy.italic());
    s->font.setUnderline(dummy.underline());
    s->font.setStrikeOut(dummy.strikeOut());
    s->foreground.setColor(g.readEntry("foregroundColor", s->foreground.color()));
    s->background.setColor(g.readEntry("backgroundColor", s->background.color()));
  }

  emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() -1));
}


bool DataOutputModel::useSystemLocale() const
{
  return m_useSystemLocale;
}


void DataOutputModel::setUseSystemLocale( bool useSystemLocale )
{
  m_useSystemLocale = useSystemLocale;

  emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() -1));
}


QVariant DataOutputModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::EditRole)
    return CachedSqlQueryModel::data(index, role);

  const QVariant value(CachedSqlQueryModel::data(index, Qt::DisplayRole));
  const QVariant::Type type = value.type();

  if (value.isNull())
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value(QLatin1String ("null"))->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value(QLatin1String ("null"))->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value(QLatin1String ("null"))->background);
    if (role == Qt::DisplayRole)
      return QVariant(QLatin1String ("NULL"));
  }

  if (type == QVariant::ByteArray)
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value(QLatin1String ("blob"))->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value(QLatin1String ("blob"))->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value(QLatin1String ("blob"))->background);
    if (role == Qt::DisplayRole)
      return QVariant(value.toByteArray().left(255));
  }

  if (isNumeric(type))
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value(QLatin1String ("number"))->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value(QLatin1String ("number"))->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value(QLatin1String ("number"))->background);
    if (role == Qt::TextAlignmentRole)
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::DisplayRole || role == Qt::UserRole)
    {
      if (useSystemLocale())
        return QVariant(value.toString()); //FIXME KF5 KGlobal::locale()->formatNumber(value.toString(), false));
      else
        return QVariant(value.toString());
    }
  }

  if (type == QVariant::Bool)
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value(QLatin1String ("bool"))->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value(QLatin1String ("bool"))->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value(QLatin1String ("bool"))->background);
    if (role == Qt::DisplayRole)
      return QVariant(value.toBool() ? QLatin1String ("True") : QLatin1String ("False"));
  }

  if (type == QVariant::Date ||
      type == QVariant::Time ||
      type == QVariant::DateTime )
  {
    if (role == Qt::FontRole)
      return QVariant(m_styles.value(QLatin1String ("datetime"))->font);
    if (role == Qt::ForegroundRole)
      return QVariant(m_styles.value(QLatin1String ("datetime"))->foreground);
    if (role == Qt::BackgroundRole)
      return QVariant(m_styles.value(QLatin1String ("datetime"))->background);
    if (role == Qt::DisplayRole || role == Qt::UserRole)
    {
      if (useSystemLocale())
      {
        if (type == QVariant::Date)
          return QVariant(QLocale().toString(value.toDate(), QLocale::ShortFormat));
        if (type == QVariant::Time)
          return QVariant(QLocale().toString(value.toTime()));
        if (type == QVariant::DateTime)
          return QVariant(QLocale().toString(value.toDateTime(), QLocale::ShortFormat));
      }
      else // return sql server format
        return QVariant(value.toString());
    }
  }

  if (role == Qt::FontRole)
    return QVariant(m_styles.value(QLatin1String ("text"))->font);
  if (role == Qt::ForegroundRole)
    return QVariant(m_styles.value(QLatin1String ("text"))->foreground);
  if (role == Qt::BackgroundRole)
    return QVariant(m_styles.value(QLatin1String ("text"))->background);
  if (role == Qt::TextAlignmentRole)
    return QVariant(Qt::AlignVCenter);
  if (role == Qt::DisplayRole)
    return value.toString();
  if (role == Qt::UserRole)
    return value;

  return CachedSqlQueryModel::data(index, role);
}
