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

#ifndef DATAOUTPUTMODEL_H
#define DATAOUTPUTMODEL_H

struct OutputStyle;

#include "cachedsqlquerymodel.h"

#include <qcolor.h>
#include <qfont.h>


/// provide colors and styles
class DataOutputModel : public CachedSqlQueryModel
{
  Q_OBJECT

  public:
    DataOutputModel(QObject *parent = nullptr);
    ~DataOutputModel() override;

    bool useSystemLocale() const;
    void setUseSystemLocale(bool useSystemLocale);

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    void clear() override;
    void readConfig();

  private:
    QHash<QString,OutputStyle*> m_styles;
    bool m_useSystemLocale;
};

#endif // DATAOUTPUTMODEL_H
