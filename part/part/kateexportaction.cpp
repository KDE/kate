/***************************************************************************
                          kateexportaction.cpp  -  description
                             -------------------
    begin                : Sat 16 December 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
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
