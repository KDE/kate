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

#include "btfileindexer.h"
#include "btdatabase.h"

#include <QDir>
#include <QDebug>

BtFileIndexer::BtFileIndexer(KateBtDatabase *database)
    : QThread()
    , cancelAsap(false)
    , db(database)
{
}

BtFileIndexer::~BtFileIndexer()
{
}

void BtFileIndexer::setSearchPaths(const QStringList &urls)
{
    searchPaths.clear();
    foreach(const QString &url, urls) {
        if (!searchPaths.contains(url)) {
            searchPaths += url;
        }
    }
}

void BtFileIndexer::setFilter(const QStringList &fileFilter)
{
    filter = fileFilter;
    qDebug() << filter;
}

void BtFileIndexer::run()
{
    if (filter.size() == 0) {
        qDebug() << "Filter is empty. Aborting.";
        return;
    }

    cancelAsap = false;
    for (int i = 0; i < searchPaths.size(); ++i) {
        if (cancelAsap) {
            break;
        }
        indexFiles(searchPaths[i]);
    }
    qDebug() << QStringLiteral("Backtrace file database contains %1 files").arg(db->size());
}

void BtFileIndexer::cancel()
{
    cancelAsap = true;
}

void BtFileIndexer::indexFiles(const QString &url)
{
    QDir dir(url);
    if (!dir.exists()) {
        return;
    }

    QStringList files = dir.entryList(filter, QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::NoDotAndDotDot | QDir::CaseSensitive);
    db->add(url, files);

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::Readable | QDir::NoDotAndDotDot | QDir::CaseSensitive);
    for (int i = 0; i < subdirs.size(); ++i) {
        if (cancelAsap) {
            break;
        }
        indexFiles(url + QLatin1Char('/') + subdirs[i]);
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
