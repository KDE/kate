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

// $Id$

#include "katesyntaxdocument.h"
#include <qfile.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qstringlist.h>
#include <kapp.h>
#include <kconfig.h>

SyntaxDocument::SyntaxDocument() : QDomDocument()
{
  currentFile="";
  setupModeList();
  myModeList.setAutoDelete( true );
}

void SyntaxDocument::setIdentifier(const QString& identifier)
{
  if (currentFile!=identifier)
  {
    QFile f( identifier );

    if ( f.open(IO_ReadOnly) )
    {
      setContent(&f);
      currentFile=identifier;
      f.close();
    }
    else
      KMessageBox::error( 0L, i18n("Can't open %1").arg(identifier) );
  }
}

SyntaxDocument::~SyntaxDocument()
{
}

void SyntaxDocument::setupModeList(bool force)
{
  if (myModeList.count() > 0) return;

  KConfig config("katesyntaxhighlightingrc");
  KStandardDirs *dirs = KGlobal::dirs();

  QStringList list=dirs->findAllResources("data","kate/syntax/*.xml",false,true);

  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    QString Group="Highlighting_Cache"+*it;

    if ((config.hasGroup(Group)) && (!force))
    {
      config.setGroup(Group);
      syntaxModeListItem *mli=new syntaxModeListItem;
      mli->name = config.readEntry("name","");
      mli->section = config.readEntry("section","");
      mli->mimetype = config.readEntry("mimetype","");
      mli->extension = config.readEntry("extension","");
      mli->identifier = *it;
      myModeList.append(mli);
    }
    else
    {
      QFile f(*it);

      if (f.open(IO_ReadOnly))
      {
        setContent(&f);
        f.close();
        QDomElement n = documentElement();
        if (!n.isNull())
        {
          QDomElement e=n.toElement();

          if (e.tagName()=="language")
          {
            syntaxModeListItem *mli=new syntaxModeListItem;
            mli->name = e.attribute("name");
            mli->section = e.attribute("section");
            mli->mimetype = e.attribute("mimetype");
            mli->extension = e.attribute("extensions");

            if (mli->section.isEmpty())
              mli->section=i18n("Other");

            mli->identifier = *it;

            config.setGroup(Group);
            config.writeEntry("name",mli->name);
            config.writeEntry("section",mli->section);
            config.writeEntry("mimetype",mli->mimetype);
            config.writeEntry("extension",mli->extension);

            myModeList.append(mli);
          }
        }
      }
    }
  }

  config.sync();
}

SyntaxModeList SyntaxDocument::modeList()
{
  return myModeList;
}

bool SyntaxDocument::nextGroup( syntaxContextData* data)
{
  if(!data) return false;

  if (data->currentGroup.isNull())
    data->currentGroup=data->parent.firstChild().toElement();
  else
    data->currentGroup=data->currentGroup.nextSibling().toElement();

  data->item=QDomElement();

  if (data->currentGroup.isNull())
    return false;
  else
    return true;
}

bool SyntaxDocument::nextItem( syntaxContextData* data)
{
  if(!data) return false;

  if (data->item.isNull())
    data->item=data->currentGroup.firstChild().toElement();
  else
    data->item=data->item.nextSibling().toElement();

  if (data->item.isNull())
    return false;
  else
    return true;
}

QString SyntaxDocument::groupItemData( const syntaxContextData* data,const QString& name)
{
  if(!data)
    return QString::null;

  if ( (!data->item.isNull()) && (name.isEmpty()))
    return data->item.tagName();

  if (!data->item.isNull())
    return data->item.attribute(name);
  else
    return QString();
}

QString SyntaxDocument::groupData( const syntaxContextData* data,const QString& name)
{
  if(!data)
    return QString::null;

  if (!data->currentGroup.isNull())
    return data->currentGroup.attribute(name);
  else
    return QString::null;
}

void SyntaxDocument::freeGroupInfo( syntaxContextData* data)
{
  if (data)
    delete data;
}

syntaxContextData* SyntaxDocument::getSubItems(syntaxContextData* data)
{
  syntaxContextData *retval=new syntaxContextData;

  if (data != 0)
  {
    retval->parent=data->currentGroup;
    retval->currentGroup=data->item;
    retval->item=QDomElement();
   }

  return retval;
}

syntaxContextData* SyntaxDocument::getConfig(const QString& mainGroupName, const QString &Config)
{
  QDomElement docElem = documentElement();
  QDomNode n = docElem.firstChild();

  while (!n.isNull())
  {
    kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (outer loop) " <<endl;
    QDomElement e=n.toElement();

    if (e.tagName().compare(mainGroupName)==0 )
    {
      QDomNode n1=e.firstChild();

      while (!n1.isNull())
      {
        kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (inner loop) " <<endl;
        QDomElement e1=n1.toElement();

        if (e1.tagName()==Config)
        {
          syntaxContextData *data=new ( syntaxContextData);
          data->item=e1;
          return data;
        }

        n1=e1.nextSibling();
      }

      kdDebug(13010) << "WARNING :returning null " << k_lineinfo << endl;
      return 0;
    }

    n=e.nextSibling();
  }

  kdDebug(13010) << "WARNING :returning null " << k_lineinfo << endl;
  return 0;
}



syntaxContextData* SyntaxDocument::getGroupInfo(const QString& mainGroupName, const QString &group)
{
  QDomElement docElem = documentElement();
  QDomNode n = docElem.firstChild();

  while (!n.isNull())
  {
    kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (outer loop) " <<endl;
    QDomElement e=n.toElement();

    if (e.tagName().compare(mainGroupName)==0 )
    {
      QDomNode n1=e.firstChild();

      while (!n1.isNull())
      {
        kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (inner loop) " <<endl;
        QDomElement e1=n1.toElement();

        if (e1.tagName()==group+"s")
        {
          syntaxContextData *data=new ( syntaxContextData);
          data->parent=e1;
          return data;
        }

        n1=e1.nextSibling();
      }

      kdDebug(13010) << "WARNING :returning null " << k_lineinfo << endl;
      return 0;
    }

    n=e.nextSibling();
  }

  kdDebug(13010) << "WARNING :returning null " << k_lineinfo << endl;
  return 0;
}


QStringList& SyntaxDocument::finddata(const QString& mainGroup,const QString& type,bool clearList)
{
  QDomElement e  = documentElement();
  if (clearList)
    m_data.clear();

  for(QDomNode n=e.firstChild(); !n.isNull(); n=n.nextSibling())
  {
    if (n.toElement().tagName()==mainGroup)
    {
      QDomNodeList nodelist1=n.toElement().elementsByTagName("list");

      for (uint l=0; l<nodelist1.count();l++)
      {
        if (nodelist1.item(l).toElement().attribute("name")==type)
        {
          n=nodelist1.item(l).toElement();
          QDomNodeList childlist=n.childNodes();

          for (uint i=0; i<childlist.count();i++)
            m_data+=childlist.item(i).toElement().text().stripWhiteSpace();

          break;
        }
      }

      break;
    }
  }

  return m_data;
}
