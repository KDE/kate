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

struct SearchFlags {
	bool caseSensitive     :1;
	bool wholeWords        :1;
	bool fromBeginning     :1;
	bool backward          :1;
	bool selected          :1;
	bool prompt            :1;
	bool replace           :1;
	bool again             :1;
	bool wrapped           :1;
	bool finished          :1;
	bool regExp            :1;
};

class SConfig
{
public:
	SearchFlags flags;
	KateTextCursor cursor;
	uint matchedLength;
	QString m_pattern;
	QRegExp m_regExp;
	
	void setPattern( QString &newPattern )
	{
		m_pattern = newPattern;
		if( flags.regExp ) {
			m_regExp.setCaseSensitive( flags.caseSensitive );
			m_regExp.setPattern( m_pattern );
		}
	}
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
	
	void createActions( KActionCollection* );
	
public slots:
	void find();
	void replace();
	void findAgain( bool back );
	
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
	
	void initSearch( SearchFlags flags );
	void continueSearch();
	void findAgain();
	void replaceAgain();
	void exposeFound( KateTextCursor &cursor, int slen );
	bool askContinue( bool forward, bool replace, int replacements );
	void doReplaceAction(int result, bool found = false);
	bool askReplaceEnd();
	
	QString getSearchText();
	KateTextCursor getCursor();
	
	Kate::View* view()    { return m_view; }
	Kate::Document* doc() { return m_doc;  }
	
	Kate::View*     m_view;
	Kate::Document* m_doc;
	
	SConfig       s;
	SearchFlags   m_searchFlags;
	int           replaces;
	QDialog*      replacePrompt;
};

#endif
