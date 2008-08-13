/* This file is part of the KDE project
   Copyright (C) 2008 Dominik Haumann <dhaumann kde org>

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

#ifndef KATE_FINDINFILES_OPTIONS_H
#define KATE_FINDINFILES_OPTIONS_H

#include <QString>
#include <QStringList>

class KConfigGroup;

class KateFindInFilesOptions
{
  public:
    KateFindInFilesOptions();
    ~KateFindInFilesOptions();

    static KateFindInFilesOptions& self();

    void load(const KConfigGroup& config);
    void save(KConfigGroup& config);

    QStringList searchItems();
    QStringList searchPaths();
    QStringList searchFilters();

    void addSearchItem(const QString& item);
    void addSearchPath(const QString& path);
    void addSearchFilter(const QString& filter);

    bool recursive() const;
    void setRecursive(bool recursive);

    bool caseSensitive() const;
    void setCaseSensitive(bool casesensitive);

    bool regExp() const;
    void setRegExp(bool regexp);

  private:
    bool m_recursive;
    bool m_casesensitive;
    bool m_regexp;

    QStringList m_searchItems;
    QStringList m_searchPaths;
    QStringList m_searchFilters;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
