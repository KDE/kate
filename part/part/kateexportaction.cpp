/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "kateexportaction.h"
#include "kateexportaction.moc"
#include <kpopupmenu.h>
#include <klocale.h>


void KateExportAction::init()
{
	filter.clear();
	filter<<QString("kate_html_export");
	popupMenu()->insertItem (i18n("&HTML..."),0);
	connect(popupMenu(),SIGNAL(activated(int)),this,SLOT(filterChoosen(int)));
	m_doc=0L;
}

void KateExportAction::updateMenu (Kate::Document *doc)
{
  m_doc = doc;
}

void KateExportAction::filterChoosen(int id)
{
  Kate::Document *doc = m_doc;

  if (!doc)
    return;

	doc->exportAs(*filter.at(id));
}
