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

#include "katecursor.h"

class SearchFlags
{
  public:
  bool caseSensitive     :1;
  bool wholeWords        :1;
  bool fromBeginning     :1;
  bool backward          :1;
  bool selected          :1;
  bool prompt            :1;
  bool replace           :1;
  bool finished          :1;
  bool regExp            :1;
};

class KActionCollection;

class KateView;
class KateDocument;

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
  KateSearch( KateView* );
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
  static SearchFlags s_searchFlags;
  
  void search( SearchFlags flags );
  void wrapSearch();
  bool askContinue();
  
  void findAgain();
  void promptReplace();
  void replaceAll();
  void replaceOne();
  void skipOne();
  
  QString getSearchText();
  KateTextCursor getCursor();
  bool doSearch( const QString& text );
  void exposeFound( KateTextCursor &cursor, int slen );
  
  inline KateView* view()    { return m_view; }
  inline KateDocument* doc() { return m_doc;  }
  
  KateView*     m_view;
  KateDocument* m_doc;
  
  struct SConfig {
    SearchFlags flags;
    KateTextCursor cursor;
    uint matchedLength;
    KateTextCursor selBegin;
    KateTextCursor selEnd;
  } s;
  long options;
  int           replaces;
  QDialog*      replacePrompt;
  QString m_replacement;
  QRegExp m_re;
};

#endif
