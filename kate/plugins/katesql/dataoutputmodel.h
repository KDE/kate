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

#include <qsqlquerymodel.h>
#include <qcolor.h>
#include <qfont.h>

/// only provide colors for null and blob values
class DataOutputModel : public QSqlQueryModel
{
  Q_OBJECT

  public:
    DataOutputModel(QObject *parent = 0);

    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    void readConfig();

  private:
    bool isNumeric(QVariant::Type type) const;

  private:
    QColor m_nullBackgroundColor;
    QColor m_blobBackgroundColor;
    QFont m_nullFont;
};

#endif // DATAOUTPUTMODEL_H
