/***************************************************************************
                          katecodecompletion.h  -  description
                             -------------------
  begin      : Sun Nov 18 20:00 CET 2001
  copyright  : (C) 2001 Joseph Wenninger <jowenn@kde.org>
               (C) 2002 John Firebaugh <jfirebaugh@kde.org>
               
  taken from KDEVELOP:
  begin   : Sam Jul 14 18:20:00 CEST 2001
  copyright : (C) 2001 by Victor Röder <Victor_Roeder@GMX.de>
 ***************************************************************************/

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
