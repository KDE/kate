/* This file is part of the KDE libraries
   Copyright (C) 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>

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
#include "kateschema.h"
#include "kateschema.moc"

#include "kateconfig.h"
#include "kateglobal.h"
#include "kateview.h"
#include "katerenderer.h"

#include <kcolorscheme.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>
//END


//BEGIN KateSchemaManager
QString KateSchemaManager::normalSchema ()
{
  return KGlobal::mainComponent().aboutData()->appName () + QString (" - Normal");
}

QString KateSchemaManager::printingSchema ()
{
  return KGlobal::mainComponent().aboutData()->appName () + QString (" - Printing");
}

KateSchemaManager::KateSchemaManager ()
    : m_config ("kateschemarc", KConfig::NoGlobals)
{
  update ();
}

KateSchemaManager::~KateSchemaManager ()
{
}

//
// read the types from config file and update the internal list
//
void KateSchemaManager::update (bool readfromfile)
{
  if (readfromfile)
    m_config.reparseConfiguration ();

  m_schemas = m_config.groupList();
  m_schemas.sort ();

  m_schemas.removeAll (printingSchema());
  m_schemas.removeAll (normalSchema());
  m_schemas.prepend (printingSchema());
  m_schemas.prepend (normalSchema());
}

//
// get the right group
// special handling of the default schemas ;)
//
KConfigGroup KateSchemaManager::schema (uint number)
{
  if ((number>1) && (number < (uint)m_schemas.count()))
    return m_config.group (m_schemas[number]);
  else if (number == 1)
    return m_config.group (printingSchema());
  else
    return m_config.group (normalSchema());
}

void KateSchemaManager::addSchema (const QString &t)
{
  m_config.group(t).writeEntry("Color Background", KColorScheme(QPalette::Active, KColorScheme::View).background().color());

  update (false);
}

void KateSchemaManager::removeSchema (uint number)
{
  if (number >= (uint)m_schemas.count())
    return;

  if (number < 2)
    return;

  m_config.deleteGroup (name (number));

  update (false);
}

bool KateSchemaManager::validSchema (uint number)
{
  if (number < (uint)m_schemas.count())
    return true;

  return false;
}

bool KateSchemaManager::validSchema (const QString &name)
{
  if (name == normalSchema() || name == printingSchema())
    return true;

  for (int i = 0; i < m_schemas.size(); ++i)
    if (m_schemas[i] == name)
      return true;

  return false;
}

uint KateSchemaManager::number (const QString &name)
{
  if (name == normalSchema())
    return 0;

  if (name == printingSchema())
    return 1;

  int i;
  if ((i = m_schemas.indexOf(name)) > -1)
    return i;

  return 0;
}

QString KateSchemaManager::name (uint number)
{
  if ((number>1) && (number < (uint)m_schemas.count()))
    return m_schemas[number];
  else if (number == 1)
    return printingSchema();

  return normalSchema();
}
//END


//BEGIN SCHEMA ACTION -- the 'View->Schema' menu action
void KateViewSchemaAction::init()
{
  m_group=0;
  m_view = 0;
  last = 0;

  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewSchemaAction::updateMenu (KateView *view)
{
  m_view = view;
}

void KateViewSchemaAction::slotAboutToShow()
{
  KateView *view=m_view;
  int count = KateGlobal::self()->schemaManager()->list().count();

  if (!m_group) {
   m_group=new QActionGroup(menu());
   m_group->setExclusive(true);

  }

  for (int z=0; z<count; z++)
  {
    QString hlName = KateGlobal::self()->schemaManager()->list().operator[](z);

    if (!names.contains(hlName))
    {
      names << hlName;
      QAction *a=menu()->addAction ( hlName, this, SLOT(setSchema()));
      a->setData(hlName);
      a->setCheckable(true);
      a->setActionGroup(m_group);
	//FIXME EXCLUSIVE
    }
  }

  if (!view) return;

  QString id=view->renderer()->config()->schema();
   foreach(QAction *a,menu()->actions()) {
	a->setChecked(a->data().toString()==id);

	}
//FIXME
#if 0
  popupMenu()->setItemChecked (last, false);
  popupMenu()->setItemChecked (view->renderer()->config()->schema()+1, true);

  last = view->renderer()->config()->schema()+1;
#endif
}

void KateViewSchemaAction::setSchema () {
  QAction *action = qobject_cast<QAction*>(sender());

  if (!action) return;
  QString mode=action->data().toString();

  KateView *view=m_view;

  if (view)
    view->renderer()->config()->setSchema (mode);
}
//END SCHEMA ACTION

// kate: space-indent on; indent-width 2; replace-tabs on;
