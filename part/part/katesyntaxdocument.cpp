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

/** Constructor
    Sets the current file to nothing and build the ModeList (katesyntaxhighlightingrc)
Pupeno */
SyntaxDocument::SyntaxDocument() : QDomDocument(){
  // There's no current file
  currentFile="";           
  // Let's build the Mode List (katesyntaxhighlightingrc)
  setupModeList();
  myModeList.setAutoDelete( true );
}

/** If the open hl file is different from the one needed, it opens
    the new one and assign some other things.
    identifier = File name and path of the new xml needed
Pupeno */
void SyntaxDocument::setIdentifier(const QString& identifier){
  // if the current file is the same as the new one don't do anything.
  if(currentFile!=identifier){
    // let's open the new file
    QFile f( identifier );                                            
    
    if ( f.open(IO_ReadOnly) ){
      // Let's parse the contets of the xml file
      /* The result of this function should be check for robustness, 
         a false returned means a parse error */
      setContent(&f);                           
      // Ok, now the current file is the pretended one (identifier)
      currentFile=identifier;                                      
      // Close the file, is not longer needed
      f.close();
    }
    else {                                   
      // Oh o, we couldn't open the file.
      KMessageBox::error( 0L, i18n("Can't open %1").arg(identifier) );
    }
  }
}

/** Destructor
Pupeno */
SyntaxDocument::~SyntaxDocument(){
}

/** Generate the list of hl modes, store them in myModeList
    force: if true forces to rebuild the Mode List from the xml files (instead of katesyntax...rc)
Pupeno */
void SyntaxDocument::setupModeList(bool force){
  // If there's something in myModeList the Mode List was already built so, don't do it again
  if (myModeList.count() > 0) return;
  
  // We'll store the ModeList in katesyntaxhighlightingrc
  KConfig config("katesyntaxhighlightingrc");
                                   
  // Let's get a list of all the xml files for hl
  KStandardDirs *dirs = KGlobal::dirs();
  QStringList list=dirs->findAllResources("data","kate/syntax/*.xml",false,true);
  
  // Let's iterate trhu the list and build the Mode List
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )  {
    // Each file has a group called:
    QString Group="Highlighting_Cache"+*it;
    
    // If the group exist and we're not forced to read the xml file, let's build myModeList for katesyntax..rc                                       
    if ((config.hasGroup(Group)) && (!force)){
      // Let's go to this group
      config.setGroup(Group);  
      // Let's make a new syntaxModeListItem to instert in myModeList from the information in katesyntax..rc
      syntaxModeListItem *mli=new syntaxModeListItem;
      mli->name = config.readEntry("name","");
      mli->section = i18n(config.readEntry("section","").utf8());
      mli->mimetype = config.readEntry("mimetype","");
      mli->extension = config.readEntry("extension","");
      mli->version = config.readEntry("version","");
      mli->identifier = *it;
      // Apend the item to the list
      myModeList.append(mli);
    } 
    else {
      // We're forced to read the xml files or the mode doesn't exist in the katesyntax...rc
      QFile f(*it);

      if (f.open(IO_ReadOnly)) {
        // Ok we opened the file, let's read the contents and close the file
        /* the return of setContent should be checked because a false return shows a parsing error */
        setContent(&f);
        f.close();
        QDomElement n = documentElement();
        if (!n.isNull()){
          // What does this do ???? Pupeno
          QDomElement e=n.toElement();
          
          // If the 'first' tag is language, go on 
          if (e.tagName()=="language"){
            // let's make the mode list item.
            syntaxModeListItem *mli=new syntaxModeListItem;
            mli->name = e.attribute("name"); 
            // Is this safe for translators ? I mean, they must add by hand the transalation for each section.
            // This could be done by a switch or ifs with the allowed sections but a new
            // section will can't be added without recompiling and it's not a very versatil
            // way, adding a new section (from the xml files) would break the translations.
            // Why don't we store everything in english internaly and we translate it just when showing it.
            mli->section = i18n(e.attribute("section").utf8());
            mli->mimetype = e.attribute("mimetype");
            mli->extension = e.attribute("extensions");
            mli->version = e.attribute("version");	
            
            // I think this solves the proble, everything not in the .po is Other.
            if (mli->section.isEmpty()){
              mli->section=i18n("Other");
             }

            mli->identifier = *it;
            
            // Now let's write or overwrite (if force==true) the entry in katesyntax...rc
            config.setGroup(Group);
            config.writeEntry("name",mli->name);
            config.writeEntry("section",mli->section);
            config.writeEntry("mimetype",mli->mimetype);
            config.writeEntry("extension",mli->extension);
            config.writeEntry("version",mli->version);
            // Append the new item to the list.
            myModeList.append(mli);
          }
        }
      }
    }
  }
  // Syncronize with the file katesyntax...rc
  config.sync();
}

/** Get the mode list
Pupeno */
SyntaxModeList SyntaxDocument::modeList(){
  return myModeList;
}

bool SyntaxDocument::nextGroup( syntaxContextData* data){
  if(!data){
    return false;
  }

  if (data->currentGroup.isNull()){
    data->currentGroup=data->parent.firstChild().toElement();
  }
  else {
    data->currentGroup=data->currentGroup.nextSibling().toElement();
  }

  data->item=QDomElement();

  if (data->currentGroup.isNull()){
    return false;
  }
  else {
    return true;
  }
}

bool SyntaxDocument::nextItem( syntaxContextData* data){
  if(!data) return false;

  if (data->item.isNull()){
    data->item=data->currentGroup.firstChild().toElement();
  }
  else {
    data->item=data->item.nextSibling().toElement();
  }

  if (data->item.isNull()){
    return false;
  }
  else {
    return true;
  }
}

QString SyntaxDocument::groupItemData( const syntaxContextData* data,const QString& name){
  if(!data){
    return QString::null;
  }

  if ( (!data->item.isNull()) && (name.isEmpty())){
    return data->item.tagName();                   
  }

  if (!data->item.isNull()){
    return data->item.attribute(name);
  }
  else {
    return QString();
  }
}

QString SyntaxDocument::groupData( const syntaxContextData* data,const QString& name){
  if(!data){
    return QString::null;
  }

  if (!data->currentGroup.isNull()){
    return data->currentGroup.attribute(name);
  }
  else {
    return QString::null;
  }
}

void SyntaxDocument::freeGroupInfo( syntaxContextData* data){
  if (data){
    delete data;
  }
}

syntaxContextData* SyntaxDocument::getSubItems(syntaxContextData* data){
  syntaxContextData *retval=new syntaxContextData;

  if (data != 0){
    retval->parent=data->currentGroup;
    retval->currentGroup=data->item;
    retval->item=QDomElement();
  }

  return retval;
}

syntaxContextData* SyntaxDocument::getConfig(const QString& mainGroupName, const QString &Config){
  QDomElement docElem = documentElement();
  QDomNode n = docElem.firstChild();

  while(!n.isNull()){
    kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (outer loop) " <<endl;
    QDomElement e=n.toElement();

    if (e.tagName().compare(mainGroupName)==0 ){
      QDomNode n1=e.firstChild();

      while (!n1.isNull()){
        kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (inner loop) " <<endl;
        QDomElement e1=n1.toElement();

        if (e1.tagName()==Config){
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



syntaxContextData* SyntaxDocument::getGroupInfo(const QString& mainGroupName, const QString &group){
  QDomElement docElem = documentElement();
  QDomNode n = docElem.firstChild();

  while (!n.isNull()){
    kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (outer loop) " <<endl;
    QDomElement e=n.toElement();

    if (e.tagName().compare(mainGroupName)==0 ){
      QDomNode n1=e.firstChild();

      while (!n1.isNull()){
        kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (inner loop) " <<endl;
        QDomElement e1=n1.toElement();

        if (e1.tagName()==group+"s"){
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


QStringList& SyntaxDocument::finddata(const QString& mainGroup,const QString& type,bool clearList){
  QDomElement e  = documentElement();
  if (clearList){
    m_data.clear();
  }

  for(QDomNode n=e.firstChild(); !n.isNull(); n=n.nextSibling()){
    if (n.toElement().tagName()==mainGroup){
      QDomNodeList nodelist1=n.toElement().elementsByTagName("list");

      for (uint l=0; l<nodelist1.count();l++){
        if (nodelist1.item(l).toElement().attribute("name")==type){
          n=nodelist1.item(l).toElement();
          QDomNodeList childlist=n.childNodes();

          for (uint i=0; i<childlist.count();i++){
            m_data+=childlist.item(i).toElement().text().stripWhiteSpace();
          }

          break;
        }
      }
      break;
    }
  }

  return m_data;
}
