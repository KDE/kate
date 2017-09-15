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

#ifndef CACHEDSQLQUERYMODEL_H
#define CACHEDSQLQUERYMODEL_H

#include <qsqlquerymodel.h>
#include <qsqlrecord.h>
#include <qcontiguouscache.h>

class CachedSqlQueryModel : public QSqlQueryModel
{
  Q_OBJECT
public:
  explicit CachedSqlQueryModel(QObject *parent = nullptr, int cacheCapacity = 1000);

  QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override;
  QSqlRecord record(int row) const;
  void clear() override;

  int cacheCapacity() const;

public Q_SLOTS:
  void clearCache();
  void setCacheCapacity(int);

  protected:
    void queryChange() override;

private:
  void cacheRecords(int from, int to) const;

  mutable QContiguousCache<QSqlRecord> cache;
};

#endif // CACHEDSQLQUERYMODEL_H
