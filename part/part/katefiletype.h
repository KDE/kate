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
#include <qptrvector.h>

#include "../interfaces/document.h"

class KateDocument;

class KConfig;

class KateFileType
{
  public:
    int number;
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
     * -1 if none found !
     */
    int fileType (KateDocument *doc);

    /**
     * Don't store the pointer somewhere longer times, won't be valid after the next update()
     */
    KateFileType *fileType (uint number);

    /**
     * Don't modify
     */
    QPtrVector<KateFileType> *list () { return &m_types; }

  private:
    int wildcardsFind (const QString &fileName);

  private:
    KConfig *m_config;
    QPtrVector<KateFileType> m_types;
};

class KateFileTypeConfigTab : public Kate::ConfigPage
{
  Q_OBJECT

  public:
    KateFileTypeConfigTab( QWidget *parent );

  public slots:
  void apply();
  void reload();
  void reset();
  void defaults();

  protected:
};

#endif
