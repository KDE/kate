/* This file is part of the KDE libraries
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

#ifndef __KATE_SCHEMA_H__
#define __KATE_SCHEMA_H__

#include <kactionmenu.h>
#include <kconfig.h>
#include <klocale.h>

#include <QtCore/QStringList>
#include <QtCore/QPointer>

class KateView;
class QActionGroup;

class KateSchema
{
  public:
    QString rawName;
    int shippedDefaultSchema;
    
    /**
     * construct translated name for shipped schemas
     */
    QString translatedName () const {
      return shippedDefaultSchema ? i18nc("Color Schema", rawName.toUtf8()) : rawName;
    }
};

class KateSchemaManager
{
  public:
    KateSchemaManager ();
    
    /**
     * Config
     */
    KConfig &config ()
    {
      return m_config;
    }

    /**
     * return kconfiggroup for the given schema
     */
    KConfigGroup schema (const QString &name);
    
    /**
     * return schema data for on schema
     */
    KateSchema schemaData (const QString &name);

    /**
     * Constructs list of schemas atm known in config object
     */
    QList<KateSchema> list ();

  private:
    KConfig m_config;
};


class KateViewSchemaAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewSchemaAction(const QString& text, QObject *parent)
       : KActionMenu(text, parent) { init(); }

    void updateMenu (KateView *view);

  private:
    void init();

    QPointer<KateView> m_view;
    QStringList names;
    QActionGroup *m_group;
    int last;

  public  Q_SLOTS:
    void slotAboutToShow();

  private Q_SLOTS:
    void setSchema();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
