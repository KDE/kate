/* This file is part of the KDE libraries

  copyright  : (C) 2001 Joseph Wenninger <jowenn@kde.org>
               (C) 2002 John Firebaugh <jfirebaugh@kde.org>
 	       (C) 2001 by Victor Röder <Victor_Roeder@GMX.de>

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

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/
/* Trolltech doesn't mind, if we license that piece of code as LGPL, because there isn't much
 * left from the desigener code */


#ifndef __KateCodeCompletion_H__
#define __KateCodeCompletion_H__

#include <qvaluelist.h>
#include <qstringlist.h>
#include <qvbox.h>
#include <qlistbox.h>
#include <qlabel.h>

#include <ktexteditor/codecompletioninterface.h>

class KDevArgHint;
class KateView;

class KateCodeCompletionCommentLabel : public QLabel
{
    Q_OBJECT
public:
    KateCodeCompletionCommentLabel( QWidget* parent, const QString& text) : QLabel( parent, "toolTipTip",
             WStyle_StaysOnTop | WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WX11BypassWM )
    {
        setMargin(1);
        setIndent(0);
        setAutoMask( FALSE );
        setFrameStyle( QFrame::Plain | QFrame::Box );
        setLineWidth( 1 );
        setAlignment( AlignAuto | AlignTop );
        polish();
        setText(text);
        adjustSize();
    }
};


class KateCodeCompletion : public QObject
{
  Q_OBJECT

public:
  KateCodeCompletion(KateView *view);

  bool codeCompletionVisible ();

  void showArgHint(
      QStringList functionList, const QString& strWrapping, const QString& strDelimiter );
  void showCompletionBox( 
      QValueList<KTextEditor::CompletionEntry> entries, int offset = 0, bool casesensitive = true );
  bool eventFilter( QObject* o, QEvent* e );

public slots:
  void slotCursorPosChanged();
  void showComment();

signals:
  void completionAborted();
  void completionDone();
  void argHintHidden();
  void completionDone(KTextEditor::CompletionEntry);
  void filterInsertString(KTextEditor::CompletionEntry*,QString *);

private:
  void doComplete();
  void abortCompletion();
  void complete( KTextEditor::CompletionEntry );
  void updateBox( bool newCoordinate = false );

  KDevArgHint*    m_pArgHint;
  KateView*       m_view;
  QVBox*          m_completionPopup;
  QListBox*       m_completionListBox;
  QValueList<KTextEditor::CompletionEntry> m_complList;
  uint            m_lineCursor;
  uint            m_colCursor;
  int             m_offset;
  bool            m_caseSensitive;
  KateCodeCompletionCommentLabel* m_commentLabel;
};

#endif
