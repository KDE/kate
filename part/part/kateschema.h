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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kate_schema_h__
#define __kate_schema_h__

#include <qstringlist.h>

#include <kconfig.h>

class KateSchemaManager
{
  public:
    KateSchemaManager ();
    ~KateSchemaManager ();

    /**
     * Schema Config changed, update all docs (which will take care of views/renderers)
     */
    void update ();

    /**
     * return kconfig with right group set or set to Normal if not there
     */
    KConfig *schema (uint number);

    /**
     * Don't modify
     */
    const QStringList &list () { return m_schemas; }

  private:
    int wildcardsFind (const QString &fileName);

  private:
    KConfig m_config;
    QStringList m_schemas;
};

#endif
