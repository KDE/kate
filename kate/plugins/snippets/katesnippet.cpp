/* This file is part of the KDE project
   Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
   Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "katesnippet.h"

KateSnippet::KateSnippet(QString key, QString value, Q3ListViewItem *lvi)
    : m_key(key), m_value(value), m_listViewItem(lvi)
{
}

KateSnippet::~KateSnippet()
{
}

// kate: space-indent on; indent-width 2; replace-tabs on;
