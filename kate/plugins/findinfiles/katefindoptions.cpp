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

#include "katefindoptions.h"

#include <KConfigGroup>

KateFindInFilesOptions::KateFindInFilesOptions()
{
}

KateFindInFilesOptions::~KateFindInFilesOptions()
{
}

KateFindInFilesOptions::KateFindInFilesOptions(const KateFindInFilesOptions& copy)
{
  *this = copy;
}

KateFindInFilesOptions& KateFindInFilesOptions::operator=(const KateFindInFilesOptions& copy)
{
    m_recursive = copy.m_recursive;
    m_casesensitive = copy.m_casesensitive;
    m_regexp = copy.m_regexp;
    m_followDirectorySymlinks = copy.m_followDirectorySymlinks;
    m_includeHiddenFiles = copy.m_includeHiddenFiles;

    // only the global models are used
}


KateFindInFilesOptions& KateFindInFilesOptions::self()
{
  static KateFindInFilesOptions fifo;
  return fifo;
}

void KateFindInFilesOptions::load(const KConfigGroup& config)
{
  if (this != &self()) {
    self().load(config);
    return;
  }

  // now restore new session settings
  m_searchItems.setStringList(config.readEntry("LastSearchItems", QStringList()));
  m_searchPaths.setStringList(config.readEntry("LastSearchPaths", QStringList()));
  m_searchFilters.setStringList(config.readEntry("LastSearchFiles", QStringList()));

  // sane default values
  if (m_searchFilters.rowCount() == 0)
  {
    // if there are no entries, most probably the first Kate start.
    // Initialize with default values.
    QStringList stringList;
    stringList << "*"
               << "*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c"
               << "*.cpp,*.cc,*.C,*.cxx,*.c"
               << "*.h,*.hxx,*.idl";
    m_searchFilters.setStringList(stringList);
  }

  m_casesensitive = config.readEntry("CaseSensitive", true);
  m_recursive = config.readEntry("Recursive", true);
  m_regexp = config.readEntry("RegExp", false);
  m_followDirectorySymlinks = config.readEntry("FollowDirectorySymlinks", false);
  m_includeHiddenFiles = config.readEntry("IncludeHiddenFiles", false);
}

void KateFindInFilesOptions::save(KConfigGroup& config)
{
  if (this != &self()) {
    self().save(config);
    return;
  }

  // first remove duplicates, as setDuplicatesEnabled does not work for QComboBox with Model
  // further, limit count of entries to 15
  QStringList stringList = m_searchItems.stringList();
  stringList.removeDuplicates();
  if (stringList.count() > 15)
    stringList.erase(stringList.begin() + 15, stringList.end());
  m_searchItems.setStringList(stringList);
  
  stringList = m_searchPaths.stringList();
  stringList.removeDuplicates();
  if (stringList.count() > 15)
    stringList.erase(stringList.begin() + 15, stringList.end());
  m_searchPaths.setStringList(stringList);

  stringList = m_searchFilters.stringList();
  stringList.removeDuplicates();
  if (stringList.count() > 15)
    stringList.erase(stringList.begin() + 15, stringList.end());
  m_searchFilters.setStringList(stringList);

  // now save
  config.writeEntry("LastSearchItems", m_searchItems.stringList());
  config.writeEntry("LastSearchPaths", m_searchPaths.stringList());
  config.writeEntry("LastSearchFiles", m_searchFilters.stringList());

  config.writeEntry("Recursive", m_recursive);
  config.writeEntry("CaseSensitive", m_casesensitive);
  config.writeEntry("RegExp", m_regexp);
  config.writeEntry("FollowDirectorySymlinks", m_followDirectorySymlinks);
  config.writeEntry("IncludeHiddenFiles", m_includeHiddenFiles);
}

QStringListModel* KateFindInFilesOptions::searchItems()
{
  if (this != &self())
    return self().searchItems();
  return &m_searchItems;
}

QStringListModel* KateFindInFilesOptions::searchPaths()
{
  if (this != &self())
    return self().searchPaths();
  return &m_searchPaths;
}

QStringListModel* KateFindInFilesOptions::searchFilters()
{
  if (this != &self())
    return self().searchFilters();
  return &m_searchFilters;
}

bool KateFindInFilesOptions::recursive() const
{
  return m_recursive;
}

void KateFindInFilesOptions::setRecursive(bool recursive)
{
  m_recursive = recursive;

  if (this != &self())
    self().setRecursive(recursive);
}

bool KateFindInFilesOptions::caseSensitive() const
{
  return m_casesensitive;
}

void KateFindInFilesOptions::setCaseSensitive(bool casesensitive)
{
  m_casesensitive = casesensitive;

  if (this != &self())
    self().setCaseSensitive(casesensitive);
}

bool KateFindInFilesOptions::regExp() const
{
  return m_regexp;
}

void KateFindInFilesOptions::setRegExp(bool regexp)
{
  m_regexp = regexp;

  if (this != &self())
    self().setRegExp(regexp);
}

bool KateFindInFilesOptions::followDirectorySymlinks() const
{
  return m_followDirectorySymlinks;
}

void KateFindInFilesOptions::setFollowDirectorySymlinks(bool follow)
{
  m_followDirectorySymlinks = follow;

  if (this != &self())
    self().setFollowDirectorySymlinks(follow);
}

bool KateFindInFilesOptions::includeHiddenFiles() const
{
  return m_includeHiddenFiles;
}

void KateFindInFilesOptions::setIncludeHiddenFiles(bool include)
{
  m_includeHiddenFiles = include;

  if (this != &self())
    self().setIncludeHiddenFiles(include);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
