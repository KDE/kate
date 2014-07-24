/* This file is part of the KDE project
   Copyright 2008-2014 Dominik Haumann <dhaumann kde org>

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

#ifndef KATE_BACKTRACEDB_H
#define KATE_BACKTRACEDB_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QMutex>

class KateBtDatabase
{
public:
    KateBtDatabase() {}
    ~KateBtDatabase() {}

    void loadFromFile(const QString &url);
    void saveToFile(const QString &url) const;

    QString value(const QString &key);

    void add(const QString &folder, const QStringList &files);

    int size() const;

private:
    mutable QMutex mutex;
    QHash<QString, QStringList> db;
};

#endif //KATE_BACKTRACEDB_H

// kate: space-indent on; indent-width 4; replace-tabs on;
