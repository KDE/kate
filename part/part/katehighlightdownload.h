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

#ifndef _KATE_HIGHLIGHTDOWNLOAD_H_
#define _KATE_HIGHLIGHTDOWNLOAD_H_

#include <kdialogbase.h>
#include <kio/jobclasses.h>

#define HLDOWNLOADPATH "http://www.kde.org/apps/kate/hl/update.xml"

class HlDownloadDialog: public KDialogBase
{
	Q_OBJECT
public:
	HlDownloadDialog(QWidget *parent, const char *name, bool modal);
	~HlDownloadDialog();
private:
	class QListView	*list;
	class QString listData;
private slots:
	void listDataReceived(KIO::Job *, const QByteArray &data);

public slots:
	void slotUser1();

};

#endif

