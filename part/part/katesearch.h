/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>  
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef _katesearch_h_
#define _katesearch_h_

#include <qstring.h>
#include <qregexp.h>
#include <qptrlist.h>
#include <qdialog.h>

#include "kateglobal.h"

class SConfig
{
public:
     // Search flags
    enum SearchFlags
    {
      sfCaseSensitive=1,
      sfWholeWords=2,
      sfFromBeginning=4,
      sfBackward=8,
      sfSelected=16,
      sfPrompt=32,
      sfReplace=64,
      sfAgain=128,
      sfWrapped=256,
      sfFinished=512,
      sfRegularExpression=1024
    };
   KateTextCursor cursor;
   int flags;

    // Set the pattern to be used for searching.
    void setPattern(QString &newPattern) {
  bool regExp = (flags & sfRegularExpression);

  m_pattern = newPattern;
  if (regExp) {
    m_regExp.setCaseSensitive(flags & sfCaseSensitive);
    m_regExp.setPattern(m_pattern);
  }
}

    // The length of the last match found using pattern or regExp.
    int matchedLength;

    QString m_pattern;

    // The regular expression corresponding to pattern. Only guaranteed valid if
    // flags has sfRegularExpression set.
    QRegExp m_regExp;
};

class KActionCollection;
namespace Kate {
	class View;
	class Document;
}

class KateSearch : public QObject
{
Q_OBJECT
public:
    enum Dialog_results {
      srYes = QDialog::Accepted,
      srNo = 10,
      srAll,
      srCancel = QDialog::Rejected
    };
public:
	KateSearch( Kate::View* );
	virtual ~KateSearch();
public:
	void createActions( KActionCollection* );
public slots:
	void find();
	void replace();
	void findAgain( bool back );
private:
    void doReplaceAction(int result, bool found = false);
    bool askReplaceEnd();
private slots:
	void replaceSlot();
	void slotFindNext() { findAgain( false ); }
	void slotFindPrev() { findAgain( true );  }
private:
	static void addToList( QStringList&, const QString& );
	static void addToSearchList( const QString& s )  { addToList( s_searchList, s ); }
	static void addToReplaceList( const QString& s ) { addToList( s_replaceList, s ); }
	static QStringList s_searchList;
	static QStringList s_replaceList;
	
	void initSearch( int flags );
	void continueSearch();
	void findAgain();
	void replaceAgain();
	void exposeFound( KateTextCursor &cursor, int slen );
	bool askContinue( bool forward, bool replace, int replacements );
	
	QString getSearchText();
	KateTextCursor getCursor();
	
	Kate::View* view()    { return m_view; }
	Kate::Document* doc() { return m_doc;  }
	
	Kate::View*     m_view;
	Kate::Document* m_doc;
	
	SConfig s;
	uint       _searchFlags;
	int        replaces;
	QDialog*   replacePrompt;
};

#endif
