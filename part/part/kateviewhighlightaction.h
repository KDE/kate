/***************************************************************************
                          katehighlightaction.cpp  -  description
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

#ifndef _KATEVIEW_HIGHLIGHTACTION_H_
#define _KATEVIEW_HIGHLIGHTACTION_H_

#include "kateglobal.h"

#include "../interfaces/document.h"

#include <kaction.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qpopupmenu.h>
#include <qguardedptr.h>

class KateViewHighlightAction: public Kate::ActionMenu
{
  Q_OBJECT
  
  public:
    KateViewHighlightAction(const QString& text, QObject* parent = 0, const char* name = 0)
       : Kate::ActionMenu(text, parent, name) { init(); };

    ~KateViewHighlightAction(){;};
    
    void updateMenu (Kate::Document *doc);

  private:
    QGuardedPtr<Kate::Document>  myDoc;
    void init();
    QStringList subMenusName;
    QStringList names;
    QPtrList<QPopupMenu> subMenus;

  public  slots:
    void slotAboutToShow();

  private slots:
    void setHl (int mode);
};

#endif
