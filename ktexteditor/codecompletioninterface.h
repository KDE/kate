/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger (jowenn@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ktexteditor_codecompletioninterface_h__
#define __ktexteditor_codecompletioninterface_h__
#include <qstring.h>
#include <qstringlist.h>

namespace KTextEditor
{
static int CodeCompletionInterfaceNumber;


  class CompletionEntry {
  public:
    QString type;
    QString text;
    QString prefix;
    QString postfix;
    QString comment;

    bool operator==( const CompletionEntry &c ) const {
      return ( c.type == type &&
	       c.text == text &&
	       c.postfix == postfix &&
	       c.prefix == prefix &&
	       c.comment == comment);
    }
  };


class PrivateCodeCompletionInterface;

/*
*  This is an interface for the KTextEditor::View class !!!
*/
class CodeCompletionInterface
{
  public:
	CodeCompletionInterface();
	virtual ~CodeCompletionInterface();

	//
	// slots !!!
	//
    /**
     * This shows an argument hint
     */
    virtual void showArgHint ( QStringList functionList, const QString& strWrapping, const QString& strDelimiter ) = 0;
    
    /**
     * This shows a completion list box
     */
    virtual void showCompletionBox (QValueList<CompletionEntry> complList,int offset=0)=0;

	//
	// signals !!!
	//
	public:
    virtual void completionAborted()=0;
    virtual void completionDone()=0;
    virtual void argHintHided()=0;

private:
		PrivateCodeCompletionInterface *d;
};


};

#endif
