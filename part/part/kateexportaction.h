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

#ifndef _KATE_EXPORTACTION_H_
#define _KATE_EXPORTACTION_H_

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
	QGuardedPtr<Kate::Document>  m_doc;
	QStringList filter;
	void init();

protected slots:
	void filterChoosen(int);
};

#endif
