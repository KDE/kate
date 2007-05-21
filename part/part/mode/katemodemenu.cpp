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
#include "katemodemenu.h"
#include "katemodemenu.moc"

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

#define KATE_FT_HOWMANY 1024
//END Includes

void KateModeMenu::init()
{
  m_doc = 0;

  connect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );

  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

KateModeMenu::~ KateModeMenu( )
{
  qDeleteAll(subMenus);
}

void KateModeMenu::updateMenu (KTextEditor::Document *doc)
{
  m_doc = (KateDocument *)doc;
}

void KateModeMenu::slotAboutToShow()
{
  KateDocument *doc=m_doc;
  int count = KateGlobal::self()->modeManager()->list().count();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateGlobal::self()->modeManager()->list().at(z)->name;
    QString hlSection = KateGlobal::self()->modeManager()->list().at(z)->section;

    if ( !hlSection.isEmpty() && !names.contains(hlName) )
    {
      if (!subMenusName.contains(hlSection))
      {
        subMenusName << hlSection;
        QMenu *qmenu = new QMenu (hlSection);
        connect( qmenu, SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
        subMenus.append(qmenu);
        menu()->addMenu (qmenu);
      }

      int m = subMenusName.indexOf (hlSection);
      names << hlName;
      QAction *action = subMenus.at(m)->addAction ( hlName );
      action->setCheckable( true );
      action->setData( hlName );
    }
    else if (!names.contains(hlName))
    {
      names << hlName;

      disconnect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
      connect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );

      QAction *action = menu()->addAction ( hlName );
      action->setCheckable( true );
      action->setData( hlName );
    }
  }

  if (!doc) return;

  for (int i=0;i<subMenus.count();i++)
  {
    QList<QAction*> actions = subMenus.at( i )->actions();
    for ( int j = 0; j < actions.count(); ++j )
      actions[ j ]->setChecked( false );
  }

  QList<QAction*> actions = menu()->actions();
  for ( int i = 0; i < actions.count(); ++i )
    actions[ i ]->setChecked( false );

  if (doc->fileType().isEmpty() || doc->fileType() == "None") {
    for ( int i = 0; i < actions.count(); ++i ) {
      if ( actions[ i ]->data().toString() == "None" )
        actions[ i ]->setChecked( true );
    }
  } else {
    if (!doc->fileType().isEmpty())
    {
      const KateFileType& t = KateGlobal::self()->modeManager()->fileType(doc->fileType());
      int i = subMenusName.indexOf (t.section);
      if (i >= 0 && subMenus.at(i)) {
        QList<QAction*> actions = subMenus.at( i )->actions();
        for ( int j = 0; j < actions.count(); ++j ) {
          if ( actions[ j ]->data().toString() == doc->fileType() )
            actions[ j ]->setChecked( true );
        }
      } else {
        QList<QAction*> actions = menu()->actions();
        for ( int j = 0; j < actions.count(); ++j ) {
          if ( actions[ j ]->data().toString().isEmpty() )
            actions[ j ]->setChecked( true );
        }
      }
    }
  }
}

void KateModeMenu::setType (QAction *action)
{
  KateDocument *doc=m_doc;

  if (doc) {
    doc->updateFileType(action->data().toString(), true);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
