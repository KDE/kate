/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

#ifndef __ktexteditor_codecompletioninterface_h__
#define __ktexteditor_codecompletioninterface_h__

#include <qstring.h>
#include <qstringlist.h>

namespace KTextEditor
{

class CompletionEntry
{
  public:
    QString type;
    QString text;
    QString prefix;
    QString postfix;
    QString comment;

    QString userdata;    // data, which the user of this interface can use in filterInsertString. free formed


    bool operator==( const CompletionEntry &c ) const {
      return ( c.type == type &&
	       c.text == text &&
	       c.postfix == postfix &&
	       c.prefix == prefix &&
	       c.comment == comment &&
               c.userdata == userdata);
    }
};

/*
*  This is an interface for the KTextEditor::View class !!!
*/
class CodeCompletionInterface
{
  friend class PrivateCodeCompletionInterface;

  public:
	CodeCompletionInterface();
	virtual ~CodeCompletionInterface();
  
  unsigned int codeCompletionInterfaceNumber () const;

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
    virtual void showCompletionBox (QValueList<CompletionEntry> complList,int offset=0, bool casesensitive=true)=0;

	//
	// signals !!!
	//
	public:
    virtual void completionAborted()=0;
    virtual void completionDone()=0;
    virtual void argHintHidden()=0;

    virtual void completionDone(CompletionEntry*)=0;
    virtual void filterInsertString(CompletionEntry*,QString *)=0;

  private:
    class PrivateCodeCompletionInterface *d;
    static unsigned int globalCodeCompletionInterfaceNumber;
    unsigned int myCodeCompletionInterfaceNumber;
};


};

#endif
