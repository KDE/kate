/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2000 Scott Manson <sdmanson@alltel.net>

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

#ifndef _KATE_SYNTAXDOCUMENT_H_
#define _KATE_SYNTAXDOCUMENT_H_

#include "kateglobal.h"

#include <qdom.h>
#include <qptrlist.h>
#include <qstringlist.h>

class QStringList;

class syntaxModeListItem
{
  public:
    QString name;
    QString section;
    QString mimetype;
    QString extension;
    QString identifier;
};

class syntaxContextData
{
  public:
    QDomElement parent;
    QDomElement currentGroup;
    QDomElement item;
};

typedef QPtrList<syntaxModeListItem> SyntaxModeList;

class SyntaxDocument : public QDomDocument
{
  public:
    SyntaxDocument();
    ~SyntaxDocument();

    QStringList& finddata(const QString& mainGroup,const QString& type,bool clearList=true);
    SyntaxModeList modeList();

    syntaxContextData* getGroupInfo(const QString& langName, const QString &group);
    void freeGroupInfo(syntaxContextData* data);
    syntaxContextData* getConfig(const QString& mainGroupName, const QString &Config);       
    bool nextItem(syntaxContextData* data);
    bool nextGroup(syntaxContextData* data);
    syntaxContextData* getSubItems(syntaxContextData* data);
    QString groupItemData(const syntaxContextData* data,const QString& name);
    QString groupData(const syntaxContextData* data,const QString& name);
    void setIdentifier(const QString& identifier);

  private:
     void setupModeList(bool force=false);
     QString currentFile;
    SyntaxModeList myModeList;
    QStringList m_data;
};

#endif
