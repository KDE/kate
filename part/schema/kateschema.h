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

#include "katehighlight.h"
#include "katedialogs.h"

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtGui/QFont>

#include <kconfig.h>
#include <kaction.h>

class KateView;
class KateStyleTreeWidget;

class QAction;
class QActionGroup;
class KComboBox;
class KTabWidget;

class KateSchemaManager
{
  public:
    KateSchemaManager ();
    ~KateSchemaManager ();

    /**
     * Schema Config changed, update all renderers
     */
    void update (bool readfromfile = true);

    /**
     * return kconfig with right group set or set to Normal if not there
     */
    KConfigGroup schema (uint number);

    void addSchema (const QString &t);

    void removeSchema (uint number);

    /**
     * is this schema valid ? (does it exist ?)
     */
    bool validSchema (uint number);
    bool validSchema (const QString &name);

    /**
     * if not found, defaults to 0
     */
    uint number (const QString &name);

    /**
     * group names in the end, no i18n involved
     */
    QString name (uint number);

    /**
     * Don't modify, list with the names of the schemas (i18n name for the default ones)
     */
    const QStringList &list () { return m_schemas; }

    static QString normalSchema ();
    static QString printingSchema ();

  private:
    KConfig m_config;
    QStringList m_schemas;
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
