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

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kconfig.h>

#include <qfile.h>

/** Constructor
    Sets the current file to nothing and build the ModeList (katesyntaxhighlightingrc)
*/
SyntaxDocument::SyntaxDocument()
  : QDomDocument()
{
  currentFile = "";
  // Let's build the Mode List (katesyntaxhighlightingrc)
  setupModeList();
  myModeList.setAutoDelete( true );
}

/** Destructor
    Do nothing yet
*/
SyntaxDocument::~SyntaxDocument()
{
}

/** If the open hl file is different from the one needed, it opens
    the new one and assign some other things.
    identifier = File name and path of the new xml needed
*/
bool SyntaxDocument::setIdentifier(const QString& identifier)
{
  // if the current file is the same as the new one don't do anything.
  if(currentFile != identifier)
  {
    // let's open the new file
    QFile f( identifier );

    if ( f.open(IO_ReadOnly) )
    {
      // Let's parse the contets of the xml file
      /* The result of this function should be check for robustness,
         a false returned means a parse error */
      QString errorMsg;
      int line, col;
      bool success=setContent(&f,&errorMsg,&line,&col);

      // Ok, now the current file is the pretended one (identifier)
      currentFile = identifier;

      // Close the file, is not longer needed
      f.close();

      if (!success)
      {
        KMessageBox::error(0L,i18n("<qt>The error <b>%4</b><br> has been detected in the file %1 at %2/%3</qt>").arg(identifier)
            .arg(line).arg(col).arg(errorMsg));
        return false;
      }
    }
    else
    {
      // Oh o, we couldn't open the file.
      KMessageBox::error( 0L, i18n("Unable to open %1").arg(identifier) );
      return false;
    }
  }
  return true;
}

/** Get the complete syntax mode list
*/
SyntaxModeList SyntaxDocument::modeList()
{
  return myModeList;
}

/**
 * Jump to the next group, syntaxContextData::currentGroup will point to the next group
 */
bool SyntaxDocument::nextGroup( syntaxContextData* data)
{
  if(!data)
    return false;

  // No group yet so go to first child
  if (data->currentGroup.isNull())
  {
    // Skip over non-elements. So far non-elements are just comments
    QDomNode node = data->parent.firstChild();
    while (node.isComment())
      node = node.nextSibling();

    data->currentGroup = node.toElement();
  }
  else
  {
    // common case, iterate over siblings, skipping comments as we go
    QDomNode node = data->currentGroup.nextSibling();
    while (node.isComment())
      node = node.nextSibling();

    data->currentGroup = node.toElement();
  }

  return !data->currentGroup.isNull();
}

/**
 * Jump to the next item, syntaxContextData::item will point to the next item
 */
bool SyntaxDocument::nextItem( syntaxContextData* data)
{
  if(!data)
    return false;

  if (data->item.isNull())
  {
    QDomNode node = data->currentGroup.firstChild();
    while (node.isComment())
      node = node.nextSibling();

    data->item = node.toElement();
  }
  else
  {
    QDomNode node = data->item.nextSibling();
    while (node.isComment())
      node = node.nextSibling();

    data->item = node.toElement();
  }

  return !data->item.isNull();
}

/**
 * This function is used to fetch the atributes of the tags of the item in a syntaxContextData.
 */
QString SyntaxDocument::groupItemData( const syntaxContextData* data, const QString& name){
  if(!data)
    return QString::null;

  // If there's no name just return the tag name of data->item
  if ( (!data->item.isNull()) && (name.isEmpty()))
  {
    return data->item.tagName();
  }

  // if name is not empty return the value of the attribute name
  if (!data->item.isNull())
  {
    return data->item.attribute(name);
  }

  return QString::null;

}

QString SyntaxDocument::groupData( const syntaxContextData* data,const QString& name)
{
  if(!data)
    return QString::null;

  if (!data->currentGroup.isNull())
  {
    return data->currentGroup.attribute(name);
  }
  else
  {
    return QString::null;
  }
}

void SyntaxDocument::freeGroupInfo( syntaxContextData* data)
{
  if (data)
    delete data;
}

syntaxContextData* SyntaxDocument::getSubItems(syntaxContextData* data)
{
  syntaxContextData *retval = new syntaxContextData;

  if (data != 0)
  {
    retval->parent = data->currentGroup;
    retval->currentGroup = data->item;
  }

  return retval;
}

bool SyntaxDocument::getElement (QDomElement &element, const QString &mainGroupName, const QString &config)
{
  kdDebug(13010) << "Looking for \"" << mainGroupName << "\" -> \"" << config << "\"." << endl;

  QDomNodeList nodes = documentElement().childNodes();

  // Loop over all these child nodes looking for mainGroupName
  for (unsigned int i=0; i<nodes.count(); i++)
  {
    QDomElement elem = nodes.item(i).toElement();
    if (elem.tagName() == mainGroupName)
    {
      // Found mainGroupName ...
      QDomNodeList subNodes = elem.childNodes();

      // ... so now loop looking for config
      for (unsigned int j=0; j<subNodes.count(); j++)
      {
        QDomElement subElem = subNodes.item(j).toElement();
        if (subElem.tagName() == config)
        {
          // Found it!
          element = subElem;
          return true;
        }
      }

      kdDebug(13010) << "WARNING: \""<< config <<"\" wasn't found!" << endl;
      return false;
    }
  }

  kdDebug(13010) << "WARNING: \""<< mainGroupName <<"\" wasn't found!" << endl;
  return false;
}

/**
 * Get the syntaxContextData of the QDomElement Config inside mainGroupName
 * syntaxContextData::item will contain the QDomElement found
 */
syntaxContextData* SyntaxDocument::getConfig(const QString& mainGroupName, const QString &config)
{
  QDomElement element;
  if (getElement(element, mainGroupName, config))
  {
    syntaxContextData *data = new syntaxContextData;
    data->item = element;
    return data;
  }
  return 0;
}

/**
 * Get the syntaxContextData of the QDomElement Config inside mainGroupName
 * syntaxContextData::parent will contain the QDomElement found
 */
syntaxContextData* SyntaxDocument::getGroupInfo(const QString& mainGroupName, const QString &group)
{
  QDomElement element;
  if (getElement(element, mainGroupName, group+"s"))
  {
    syntaxContextData *data = new syntaxContextData;
    data->parent = element;
    return data;
  }
  return 0;
}

/**
 * Returns a list with all the keywords inside the list type
 */
QStringList& SyntaxDocument::finddata(const QString& mainGroup, const QString& type, bool clearList)
{
  kdDebug(13010)<<"Create a list of keywords \""<<type<<"\" from \""<<mainGroup<<"\"."<<endl;
  if (clearList)
    m_data.clear();

  for(QDomNode node = documentElement().firstChild(); !node.isNull(); node = node.nextSibling())
  {
    QDomElement elem = node.toElement();
    if (elem.tagName() == mainGroup)
    {
      kdDebug(13010)<<"\""<<mainGroup<<"\" found."<<endl;
      QDomNodeList nodelist1 = elem.elementsByTagName("list");

      for (uint l=0; l<nodelist1.count(); l++)
      {
        if (nodelist1.item(l).toElement().attribute("name") == type)
        {
          kdDebug(13010)<<"List with attribute name=\""<<type<<"\" found."<<endl;
          QDomNodeList childlist = nodelist1.item(l).toElement().childNodes();

          for (uint i=0; i<childlist.count(); i++)
          {
            QString element = childlist.item(i).toElement().text().stripWhiteSpace();
            if (element.isEmpty())
              continue;
#ifndef NDEBUG
            if (i<6)
            {
              kdDebug(13010)<<"\""<<element<<"\" added to the list \""<<type<<"\""<<endl;
            }
            else if(i==6)
            {
              kdDebug(13010)<<"... The list continues ..."<<endl;
            }
#endif
            m_data += element;
          }

          break;
        }
      }
      break;
    }
  }

  return m_data;
}

// Private
/** Generate the list of hl modes, store them in myModeList
    force: if true forces to rebuild the Mode List from the xml files (instead of katesyntax...rc)
*/
void SyntaxDocument::setupModeList (bool force)
{
  // If there's something in myModeList the Mode List was already built so, don't do it again
  if (!myModeList.isEmpty())
    return;

  // We'll store the ModeList in katesyntaxhighlightingrc
  KConfig config("katesyntaxhighlightingrc", false, false);

  // Let's get a list of all the xml files for hl
  QStringList list = KGlobal::dirs()->findAllResources("data","katepart/syntax/*.xml",false,true);

  // Let's iterate through the list and build the Mode List
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    // Each file has a group called:
    QString Group="Highlighting_Cache"+*it;

    // If the group exist and we're not forced to read the xml file, let's build myModeList for katesyntax..rc
    if ((config.hasGroup(Group)) && (!force))
    {
      // Let's go to this group
      config.setGroup(Group);

      // Let's make a new syntaxModeListItem to instert in myModeList from the information in katesyntax..rc
      syntaxModeListItem *mli=new syntaxModeListItem;
      mli->name       = config.readEntry("name");
      mli->section    = i18n(config.readEntry("section").utf8());
      mli->mimetype   = config.readEntry("mimetype");
      mli->extension  = config.readEntry("extension");
      mli->version    = config.readEntry("version");
      mli->priority   = config.readEntry("priority");
      mli->identifier = *it;

      // Apend the item to the list
      myModeList.append(mli);
    }
    else
    {
      // We're forced to read the xml files or the mode doesn't exist in the katesyntax...rc
      QFile f(*it);

      if (f.open(IO_ReadOnly))
      {
        // Ok we opened the file, let's read the contents and close the file
        /* the return of setContent should be checked because a false return shows a parsing error */
        QString errMsg;
        int line, col;

        bool success = setContent(&f,&errMsg,&line,&col);

        f.close();

        if (success)
        {
          QDomElement root = documentElement();

          if (!root.isNull())
          {
            // If the 'first' tag is language, go on
            if (root.tagName()=="language")
            {
              // let's make the mode list item.
              syntaxModeListItem *mli = new syntaxModeListItem;

              mli->name = root.attribute("name");
              // Is this safe for translators ? I mean, they must add by hand the transalation for each section.
              // This could be done by a switch or ifs with the allowed sections but a new
              // section will can't be added without recompiling and it's not a very versatil
              // way, adding a new section (from the xml files) would break the translations.
              // Why don't we store everything in english internaly and we translate it just when showing it.
              mli->section   = i18n(root.attribute("section").utf8());
              mli->mimetype  = root.attribute("mimetype");
              mli->extension = root.attribute("extensions");
              mli->version   = root.attribute("version");
              mli->priority  = root.attribute("priority");

              // I think this solves the problem, everything not in the .po is Other.
              if (mli->section.isEmpty())
                mli->section=i18n("Other");

              mli->identifier = *it;

              // Now let's write or overwrite (if force==true) the entry in katesyntax...rc
              config.setGroup(Group);
              config.writeEntry("name",mli->name);
              config.writeEntry("section",mli->section);
              config.writeEntry("mimetype",mli->mimetype);
              config.writeEntry("extension",mli->extension);
              config.writeEntry("version",mli->version);
              config.writeEntry("priority",mli->priority);

              // Append the new item to the list.
              myModeList.append(mli);
            }
          }
        }
        else
        {
          syntaxModeListItem *emli=new syntaxModeListItem;

          emli->section=i18n("Errors!");
          emli->mimetype="invalid_file/invalid_file";
          emli->extension="invalid_file.invalid_file";
          emli->version="1.";
          emli->name=i18n("Error: %1").arg(*it);
          emli->identifier=(*it);

          myModeList.append(emli);
        }
      }
    }
  }
  // Syncronize with the file katesyntax...rc
  config.sync();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
