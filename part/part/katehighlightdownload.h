/***************************************************************************
                          katehighlightdownload.h  -  description
                             -------------------
    begin                : Sat 31 March 2001
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

#ifndef _KATE_HIGHLIGHTDOWNLOAD_H_
#define _KATE_HIGHLIGHTDOWNLOAD_H_

#include "kateglobal.h"

#include <kdialogbase.h>
#include <kio/jobclasses.h>

#define HLDOWNLOADPATH "http://kate.sourceforge.net/highlight/update4.xml"

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

