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

#ifndef __KATE_SYNTAXDOCUMENT_H__
#define __KATE_SYNTAXDOCUMENT_H__

//BEGING INCLUDES
#include <qdom.h>
#include <qptrlist.h>
#include <qstringlist.h>
//END INCLUDES

/**
 * Information about each syntax hl Mode
 */
class KateSyntaxModeListItem
{
  public:
    QString name;
    QString section;
    QString mimetype;
    QString extension;
    QString identifier;
    QString version;
    QString priority;
    QString author;
    QString license;
};

/**
 * List of the KateSyntaxModeListItems holding all the syntax mode list items
 */
typedef QPtrList<KateSyntaxModeListItem> KateSyntaxModeList;

/**
 * Class holding the data around the current QDomElement
 */
class KateSyntaxContextData
{
  public:
    QDomElement parent;
    QDomElement currentGroup;
    QDomElement item;
};

/** Store and manage the information about Syntax Highlighting.
*/
class KateSyntaxDocument : public QDomDocument
{
  public:
    /**
     * Constructor:
     * Sets the current file to nothing and build the ModeList (katesyntaxhighlightingrc)
     */
    KateSyntaxDocument(bool force = false);

    /**
     * Desctructor
     */
    ~KateSyntaxDocument();

    /** If the open hl file is different from the one needed, it opens
     * the new one and assign some other things.
     * identifier = File name and path of the new xml needed
     */
    bool setIdentifier(const QString& identifier);

    /**
     * Get the mode list
     */
    KateSyntaxModeList modeList();

    /**
     * Jump to the next group, KateSyntaxContextData::currentGroup will point to the next group
     */
    bool nextGroup(KateSyntaxContextData* data);

    /**
     * Jump to the next item, KateSyntaxContextData::item will point to the next item
     */
    bool nextItem(KateSyntaxContextData* data);

    /**
     * This function is used to fetch the atributes of the tags.
     */
    QString groupItemData(const KateSyntaxContextData* data,const QString& name);
    QString groupData(const KateSyntaxContextData* data,const QString& name);

    void freeGroupInfo(KateSyntaxContextData* data);
    KateSyntaxContextData* getSubItems(KateSyntaxContextData* data);

    /**
     * Get the KateSyntaxContextData of the DomElement Config inside mainGroupName
     * It just fills KateSyntaxContextData::item
     */
    KateSyntaxContextData* getConfig(const QString& mainGroupName, const QString &config);

    /**
     * Get the KateSyntaxContextData of the QDomElement Config inside mainGroupName
     * KateSyntaxContextData::parent will contain the QDomElement found
     */
    KateSyntaxContextData* getGroupInfo(const QString& mainGroupName, const QString &group);

    /**
     * Returns a list with all the keywords inside the list type
     */
    QStringList& finddata(const QString& mainGroup,const QString& type,bool clearList=true);


  private:
    /**
     * Generate the list of hl modes, store them in myModeList
     * force: if true forces to rebuild the Mode List from the xml files (instead of katesyntax...rc)
    */
    void setupModeList(bool force);

    /**
     * Used by getConfig and getGroupInfo to traverse the xml nodes and
     * evenually return the found element
    */
    bool getElement (QDomElement &element, const QString &mainGroupName, const QString &config);

    /**
     * List of mode items
     */
    KateSyntaxModeList myModeList;

    QString currentFile;
    QStringList m_data;
};

#endif
