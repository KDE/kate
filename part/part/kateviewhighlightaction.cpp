/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#include "kateviewhighlightaction.h"
#include "kateviewhighlightaction.moc"
#include "kateview.h"
#include "katedocument.h"
#include "katehighlight.h"
#include <kpopupmenu.h>

void KateViewHighlightAction::init()
{
	subMenus.setAutoDelete( true );
  m_doc = 0L;
	connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewHighlightAction::updateMenu (Kate::Document *doc)
{
  m_doc = doc;
}

void KateViewHighlightAction::slotAboutToShow()
{
  kdDebug(13000)<<"KateViewHighlightAction::slotAboutToShow()"<<endl;

  Kate::Document *doc=m_doc;

  int count = HlManager::self()->highlights();
  static QString oldActiveSec;
  static int oldActiveID;

  for (int z=0; z<count; z++)
  {
    QString hlName = HlManager::self()->hlName (z);
    QString hlSection = HlManager::self()->hlSection (z);

    if ((hlSection != "") && (names.contains(hlName) < 1))
    {
      if (subMenusName.contains(hlSection) < 1)
      {
        subMenusName << hlSection;
        QPopupMenu *menu = new QPopupMenu ();
        subMenus.append(menu);
        popupMenu()->insertItem (hlSection, menu);
      }

      int m = subMenusName.findIndex (hlSection);
      names << hlName;
      subMenus.at(m)->insertItem ( hlName, this, SLOT(setHl(int)), 0,  z);
    }
    else if (names.contains(hlName) < 1)
    {
      names << hlName;
      popupMenu()->insertItem ( hlName, this, SLOT(setHl(int)), 0,  z);
    }
  }

  if (!doc) return;

  for (uint i=0;i<subMenus.count();i++)
  {
    for (uint i2=0;i2<subMenus.at(i)->count();i2++)
      	subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
  }
  popupMenu()->setItemChecked (0, false);

  int i = subMenusName.findIndex (HlManager::self()->hlSection(doc->hlMode()));
  if (i >= 0 && subMenus.at(i))
    subMenus.at(i)->setItemChecked (doc->hlMode(), true);
  else
    popupMenu()->setItemChecked (0, true);

  oldActiveSec = HlManager::self()->hlSection (doc->hlMode());
  oldActiveID = (doc->hlMode());
}

void KateViewHighlightAction::setHl (int mode)
{
  Kate::Document *doc=m_doc;
  
  if (!doc) return;

  doc->setHlMode((uint)mode);
}
