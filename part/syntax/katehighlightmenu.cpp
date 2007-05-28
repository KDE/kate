/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katehighlightmenu.h"
#include "katehighlightmenu.moc"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katesyntaxdocument.h"

#include "ui_filetypeconfigwidget.h"

#include <kconfig.h>
#include <kmimetype.h>
#include <kmimetypechooser.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <klocale.h>
#include <kmenu.h>

#include <QtCore/QRegExp>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <kvbox.h>
//END Includes

KateHighlightingMenu::~KateHighlightingMenu()
{
  qDeleteAll (subMenus);
}

void KateHighlightingMenu::init()
{
  m_doc = 0;

  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateHighlightingMenu::updateMenu (KateDocument *doc)
{
  m_doc = doc;
}

void KateHighlightingMenu::slotAboutToShow()
{
  for (int z=0; z < KateHlManager::self()->highlights(); z++)
  {
    QString hlName = KateHlManager::self()->hlNameTranslated (z);
    QString hlSection = KateHlManager::self()->hlSection (z);

    if (!KateHlManager::self()->hlHidden(z))
    {
      if ( !hlSection.isEmpty() && !names.contains(hlName) )
      {
        if (!subMenusName.contains(hlSection))
        {
          subMenusName << hlSection;
          QMenu *qmenu = new QMenu ('&'+hlSection);
          subMenus.append(qmenu);
          menu()->addMenu( qmenu );
        }

        int m = subMenusName.indexOf (hlSection);
        names << hlName;
        QAction *a=subMenus.at(m)->addAction( '&' + hlName, this, SLOT(setHl()));
        a->setData(KateHlManager::self()->hlName (z));
        a->setCheckable(true);
        subActions.append(a);
      }
      else if (!names.contains(hlName))
      {
        names << hlName;
        QAction *a=menu()->addAction ( '&' + hlName, this, SLOT(setHl()));
        a->setData(KateHlManager::self()->hlName (z));
        a->setCheckable(true);
        subActions.append(a);
      }
    }
  }

  if (!m_doc) return;
  QString mode=m_doc->highlightingMode();
  for (int i=0;i<subActions.count();i++) {
        subActions[i]->setChecked(subActions[i]->data().toString()==mode);
  }
}

void KateHighlightingMenu::setHl ()
{
  if (!m_doc || !sender()) return;
  QAction *action=qobject_cast<QAction*>(sender());
  if (!action) return;
  QString mode=action->data().toString();
  m_doc->setHighlightingMode(mode);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
