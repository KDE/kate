/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2008-2014 Dominik Haumann <dhaumann kde org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QHash>
#include <QMutex>
#include <QString>
#include <QStringList>

class KateBtDatabase
{
public:
    KateBtDatabase()
    {
    }
    ~KateBtDatabase()
    {
    }

    void loadFromFile(const QString &url);
    void saveToFile(const QString &url) const;

    QString value(const QString &key);

    void add(const QString &folder, const QStringList &files);

    int size() const;

private:
    mutable QMutex mutex;
    QHash<QString, QStringList> db;
};

// kate: space-indent on; indent-width 4; replace-tabs on;
