/***************************************************************************
                          kateexportaction.h  -  description
                             -------------------
    begin                : Sat 16th December 2001
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

#ifndef _KATE_EXPORTACTION_H_
#define _KATE_EXPORTACTION_H_

#include "kateglobal.h"

#include "../interfaces/document.h"

#include <kaction.h>
#include <qstringlist.h>
#include <qguardedptr.h>

class KateExportAction: public Kate::ActionMenu
{
	Q_OBJECT
public:
	KateExportAction(const QString& text, QObject* parent = 0, const char* name = 0)
       : Kate::ActionMenu(text, parent, name) { init(); };

	~KateExportAction(){;}
      
   void updateMenu (Kate::Document *doc);

private:
	QGuardedPtr<Kate::Document>  myDoc;
	QStringList filter;
	void init();

protected slots:
	void filterChoosen(int);
};

#endif
