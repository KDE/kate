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
#include <kconfiggroup.h>
//END

//BEGIN KateSchemaManager
KateSchemaManager::KateSchemaManager ()
    : m_config ("kateschemarc", KConfig::NoGlobals)
{
}

KConfigGroup KateSchemaManager::schema (const QString &name)
{
  return m_config.group (name);
}

KateSchema KateSchemaManager::schemaData (const QString &name)
{
  KConfigGroup cg (schema (name));
  KateSchema schema;
  schema.rawName = name;
  schema.shippedDefaultSchema = cg.readEntry ("ShippedDefaultSchema", 0);
  return schema;
}

static bool schemasCompare (const KateSchema &s1, const KateSchema &s2)
{
  if (s1.shippedDefaultSchema > s2.shippedDefaultSchema)
    return true;
  
  return s1.translatedName().localeAwareCompare(s1.translatedName()) < 0;
}

QList<KateSchema> KateSchemaManager::list ()
{
  QList<KateSchema> schemas;
  Q_FOREACH (QString s, m_config.groupList())
    schemas.append (schemaData (s));
    
  // sort: prio given by default schema and name
  qSort(schemas.begin(), schemas.end(), schemasCompare);
    
  return schemas;
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
  
  QList<KateSchema> schemas = KateGlobal::self()->schemaManager()->list ();

  if (!m_group) {
   m_group=new QActionGroup(menu());
   m_group->setExclusive(true);

  }

  for (int z=0; z< schemas.count(); z++)
  {
    QString hlName = schemas[z].translatedName();

    if (!names.contains(hlName))
    {
      names << hlName;
      QAction *a=menu()->addAction ( hlName, this, SLOT(setSchema()));
      a->setData(schemas[z].rawName);
      a->setCheckable(true);
      a->setActionGroup(m_group);
    }
  }

  if (!view) return;

  QString id=view->renderer()->config()->schema();
  foreach(QAction *a,menu()->actions()) {
	a->setChecked(a->data().toString()==id);

  }
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
