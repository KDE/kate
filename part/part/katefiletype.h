/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

   Based on KHTML Factory from Simon Hausmann <hausmann@kde.org>

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

#ifndef __kate_filetype_h__
#define __kate_filetype_h__

#include <qstringlist.h>
#include <qdict.h>
#include <qptrvector.h>

class KateDocument;

class KConfig;

class KateFileType
{
  public:
    uint number;
    QString name;
    QString section;
    QStringList wildcards;
    QStringList mimetypes;
    int priority;
};

class KateFileTypeManager
{
  public:
    KateFileTypeManager ();
    ~KateFileTypeManager ();

    /**
     * File Type Config changed, update all docs (which will take care of views/renderers)
     */
    void update ();

    /**
     * get the right fileType for the given document
     */
    QString fileType (KateDocument *doc);

    /**
     * return name of fileType with the number number ;)
     */
    QString fileType (uint number);

    /**
     * Does this fileType exist at all ?
     * An empty fileType always exists, that means no type at all
     */
    bool exists (const QString &fileType);

    /**
     * Is the fileType the default filetype (means empty string, which has no settings at all ?)
     */
    bool isDefault (const QString &fileType);

    /**
     * Don't store the pointer somewhere longer times, won't be valid after the next update()
     */
    KateFileType *fileType (const QString &name);

    /**
     * Don't modify
     */
    QPtrVector<KateFileType> *list () { return &m_typesNum; }

  private:
    int wildcardsFind (const QString &fileName);

  private:
    KConfig *m_config;
    QDict<KateFileType> m_types;
    QPtrVector<KateFileType> m_typesNum;
};

#endif
