/* This file is part of the KDE project
   Copyright 2008 Dominik Haumann <dhaumann kde org>

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

#include "btdatabase.h"

#include <QFile>
#include <QDataStream>
#include <QDir>

#include <kdebug.h>

void KateBtDatabase::loadFromFile(const QString& url)
{
  QFile file(url);
  if (file.open(QIODevice::ReadOnly)) {
    QMutexLocker locker(&mutex);
    QDataStream ds(&file);
    ds >> db;
  }
  kDebug() << "Number of entries in the backtrace database:" << db.size();
}

void KateBtDatabase::saveToFile(const QString& url) const
{
  QFile file(url);
  if (file.open(QIODevice::WriteOnly)) {
    QMutexLocker locker(&mutex);
    QDataStream ds(&file);
    ds << db;
  }
}

QString KateBtDatabase::value(const QString& key)
{
  // key is either of the form "foo/bar.txt" or only "bar.txt"
  QString file = key;
  QStringList sl = key.split('/');
  if (sl.size() > 1) {
    file = sl[1];
  }

  QMutexLocker locker(&mutex);
  if (db.contains(file)) {
    const QStringList& sl = db.value(file);
    for (int i = 0; i < sl.size(); ++i) {
      if (sl[i].indexOf(key) != -1) {
        return sl[i];
      }
    }
    // try to use the first one
    if (sl.size() > 0) {
      return sl[0];
    }
  }

  return QString();
}

void KateBtDatabase::add(const QString& folder, const QStringList& files)
{
  QMutexLocker locker(&mutex);
  foreach (const QString& file, files) {
    QStringList& sl = db[file];
    QString entry = QDir::fromNativeSeparators(folder + '/' + file);
    if (!sl.contains(entry)) {
      sl << entry;
    }
  }
}

int KateBtDatabase::size() const
{
  QMutexLocker locker(&mutex);
  return db.size();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
