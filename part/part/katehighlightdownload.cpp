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

#include "katehighlightdownload.h"
#include "katehighlightdownload.moc"
#include "katehighlight.h"
#include <klocale.h>
#include <kio/job.h>
#include <qlistview.h>
#include <kurl.h>
#include <kdebug.h>
#include <qdom.h>
#include <kio/netaccess.h>
#include <kglobal.h>
#include <kstandarddirs.h>

HlDownloadDialog::HlDownloadDialog(QWidget *parent, const char *name, bool modal)
  :KDialogBase(KDialogBase::Swallow, i18n("Highlight Download"), User1|Cancel, User1, parent, name, modal,false,i18n("&Install"))
{
	setMainWidget( list=new QListView(this));
	list->addColumn(i18n("Name"));
	list->addColumn(i18n("Installed"));
	list->addColumn(i18n("Latest"));
	list->addColumn(i18n("Release date"));
	list->setSelectionMode(QListView::Multi);
	KIO::TransferJob *getIt=KIO::get(KURL(HLDOWNLOADPATH), true, true );
	connect(getIt,SIGNAL(data(KIO::Job *, const QByteArray &)),
		this, SLOT(listDataReceived(KIO::Job *, const QByteArray &)));
//        void data( KIO::Job *, const QByteArray &data);

}

HlDownloadDialog::~HlDownloadDialog(){}

void HlDownloadDialog::listDataReceived(KIO::Job *, const QByteArray &data)
{
	listData+=QString(data);
	kdDebug(13000)<<QString("CurrentListData: ")<<listData<<endl<<endl;
	kdDebug(13000)<<QString("Data length: %1").arg(data.size())<<endl;
	kdDebug(13000)<<QString("listData length: %1").arg(listData.length())<<endl;
	if (data.size()==0)
	{
		if (listData.length()>0)
		{
			QString installedVersion;
			HlManager *hlm=HlManager::self();
			QDomDocument doc;
			doc.setContent(listData);
			QDomElement DocElem=doc.documentElement();
			QDomNode n=DocElem.firstChild();
			Highlight *hl;

			if (n.isNull()) kdDebug(13000)<<"There is no usable childnode"<<endl;
			while (!n.isNull())
			{
				installedVersion="    --";

				QDomElement e=n.toElement();
				if (!e.isNull())
				kdDebug(13000)<<QString("NAME: ")<<e.tagName()<<QString(" - ")<<e.attribute("name")<<endl;
				n=n.nextSibling();
				
				QString Name=e.attribute("name");
				
				for (int i=0;i<hlm->highlights();i++)
				{
					hl=hlm->getHl(i);
					if (hl->name()==Name)
					{
						installedVersion="    "+hl->version();
						break;
					}
				}

				(void) new QListViewItem(list,e.attribute("name"),installedVersion,e.attribute("version"),e.attribute("date"),e.attribute("url"));
			}
		}
	}
}

void HlDownloadDialog::slotUser1()
{
	QString destdir=KGlobal::dirs()->saveLocation("data","katepart/syntax/");
	for (QListViewItem *it=list->firstChild();it;it=it->nextSibling())
	{
		if (list->isSelected(it))
		{
			KURL src(it->text(4));
			QString filename=src.filename(false);
			QString dest = destdir+filename;
	

			KIO::NetAccess::download(src,dest);
		}
	}

}
