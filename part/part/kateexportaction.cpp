/***************************************************************************
                          kateexportaction.cpp  -  description
                             -------------------
    begin                : Sat 16 December 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
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
 ***************************************************************************/

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
	myDoc=0L;
}

void KateExportAction::updateMenu (Kate::Document *doc)
{
  myDoc = doc;
}

void KateExportAction::filterChoosen(int id)
{
  Kate::Document *doc = myDoc;

  if (!doc)
    return;

	doc->exportAs(*filter.at(id));
}
