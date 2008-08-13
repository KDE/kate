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

KateFindInFilesOptions& KateFindInFilesOptions::self()
{
  static KateFindInFilesOptions fifo;
  return fifo;
}

void KateFindInFilesOptions::load(const KConfigGroup& config)
{
  // now restore new session settings
  m_searchItems = config.readEntry("LastSearchItems", QStringList());
  m_searchPaths = config.readEntry("LastSearchPaths", QStringList());
  m_searchFilters = config.readEntry("LastSearchFiles", QStringList());

  // sane default values
  if (m_searchFilters.isEmpty())
  {
    // if there are no entries, most probably the first Kate start.
    // Initialize with default values.
    m_searchFilters << "*"
    << "*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c"
    << "*.cpp,*.cc,*.C,*.cxx,*.c"
    << "*.h,*.hxx,*.idl";
  }

  m_casesensitive = config.readEntry("CaseSensitive", true);
  m_recursive = config.readEntry("Recursive", true);
  m_regexp = config.readEntry("RegExp", false);
}

void KateFindInFilesOptions::save(KConfigGroup& config)
{
  config.writeEntry("LastSearchItems", m_searchItems);
  config.writeEntry("LastSearchPaths", m_searchPaths);
  config.writeEntry("LastSearchFiles", m_searchFilters);

  config.writeEntry("Recursive", m_recursive);
  config.writeEntry("CaseSensitive", m_casesensitive);
  config.writeEntry("RegExp", m_regexp);
}

QStringList KateFindInFilesOptions::searchItems()
{
  return m_searchItems;
}

QStringList KateFindInFilesOptions::searchPaths()
{
  return m_searchPaths;
}

QStringList KateFindInFilesOptions::searchFilters()
{
  return m_searchFilters;
}

void KateFindInFilesOptions::addSearchItem(const QString& item)
{
  m_searchItems.removeAll(item);
  m_searchItems.prepend(item);
  while (m_searchItems.count() > 10)
    m_searchItems.removeLast();

  if (this != &self())
    self().addSearchItem(item);
}

void KateFindInFilesOptions::addSearchPath(const QString& path)
{
  m_searchPaths.removeAll(path);
  m_searchPaths.prepend(path);
  while (m_searchPaths.count() > 10)
    m_searchPaths.removeLast();

  if (this != &self())
    self().addSearchPath(path);
}

void KateFindInFilesOptions::addSearchFilter(const QString& filter)
{
  m_searchFilters.removeAll(filter);
  m_searchFilters.prepend(filter);
  while (m_searchFilters.count() > 10)
    m_searchFilters.removeLast();

  if (this != &self())
    self().addSearchFilter(filter);
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

// kate: space-indent on; indent-width 2; replace-tabs on;
