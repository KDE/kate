/***************************************************************************
                          katehighlightdownload.cpp  -  description
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

#include "katehighlightdownload.h"
#include "katehighlightdownload.moc"
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
  :KDialogBase(KDialogBase::Swallow, i18n("Highlight Download"), User1|Cancel, User1, parent, name, modal,false,i18n("Install"))
{
	setMainWidget( list=new QListView(this));
	list->addColumn(i18n("Name"));
	list->addColumn(i18n("Release date"));
	list->addColumn(i18n("Description"));
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
	kdDebug()<<QString("CurrentListData: ")<<listData<<endl<<endl;
	kdDebug()<<QString("Data length: %1").arg(data.size())<<endl;
	kdDebug()<<QString("listData length: %1").arg(listData.length())<<endl;
	if (data.size()==0)
	{
		if (listData.length()>0)
		{
			QDomDocument doc;
			doc.setContent(listData);
			QDomElement DocElem=doc.documentElement();
			QDomNode n=DocElem.firstChild();
			if (n.isNull()) kdDebug()<<"There is no usable childnode"<<endl;
			while (!n.isNull())
			{
				QDomElement e=n.toElement();
				if (!e.isNull())
				kdDebug()<<QString("NAME: ")<<e.tagName()<<QString(" - ")<<e.attribute("name")<<endl;
				n=n.nextSibling();
				(void) new QListViewItem(list,e.attribute("name"),e.attribute("date"),e.attribute("description"),e.attribute("url"));
			}
		}
	}
}

void HlDownloadDialog::slotUser1()
{
	QString destdir=KGlobal::dirs()->saveLocation("data","kate/syntax/");
	for (QListViewItem *it=list->firstChild();it;it=it->nextSibling())
	{
		if (list->isSelected(it))
		{
			KURL src(it->text(3));
			QString filename=src.filename(false);
			QString dest = destdir+filename;
	

			KIO::NetAccess::download(src,dest);
		}
	}

}
