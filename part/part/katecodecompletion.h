/* This file is part of the KDE libraries

   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 by Victor Röder <Victor_Roeder@GMX.de>
   Copyright (C) 2002 by Roberto Raggi <roberto@kdevelop.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/
/* Trolltech doesn't mind, if we license that piece of code as LGPL, because there isn't much
 * left from the desigener code */


#ifndef __KateCodeCompletion_H__
#define __KateCodeCompletion_H__

#include <ktexteditor/codecompletioninterface.h>

#include <qvaluelist.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <qframe.h>
#include <qmap.h>
#include <qintdict.h>

class KateView;
class KateArgHint;
class KateCCListBox;

class QLayout;
class QVBox;

class KateCodeCompletionCommentLabel : public QLabel
{
  Q_OBJECT

  public:
    KateCodeCompletionCommentLabel( QWidget* parent, const QString& text) : QLabel( parent, "toolTipTip",
             WStyle_StaysOnTop | WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WX11BypassWM )
    {
        setMargin(1);
        setIndent(0);
        setAutoMask( false );
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

  friend class KateViewInternal;

  public:
    KateCodeCompletion(KateView *view);

    bool codeCompletionVisible ();

    void showArgHint(
        QStringList functionList, const QString& strWrapping, const QString& strDelimiter );
    void showCompletionBox(
        QValueList<KTextEditor::CompletionEntry> entries, int offset = 0, bool casesensitive = true );
    bool eventFilter( QObject* o, QEvent* e );

    void handleKey (QKeyEvent *e);

  public slots:
    void slotCursorPosChanged();
    void showComment();
    void updateBox () { updateBox(false); }

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
    void updateBox( bool newCoordinate );

    KateArgHint*    m_pArgHint;
    KateView*       m_view;
    QVBox*          m_completionPopup;
    KateCCListBox*       m_completionListBox;
    QValueList<KTextEditor::CompletionEntry> m_complList;
    uint            m_lineCursor;
    uint            m_colCursor;
    int             m_offset;
    bool            m_caseSensitive;
    KateCodeCompletionCommentLabel* m_commentLabel;
};

class KateArgHint: public QFrame
{
  Q_OBJECT

  public:
      KateArgHint( KateView* =0, const char* =0 );
      virtual ~KateArgHint();

      virtual void setCurrentFunction( int );
      virtual int currentFunction() const { return m_currentFunction; }

      void setArgMarkInfos( const QString&, const QString& );

      virtual void addFunction( int, const QString& );
      QString functionAt( int id ) const { return m_functionMap[ id ]; }

      virtual void show();
      virtual void adjustSize();
      virtual bool eventFilter( QObject*, QEvent* );

  signals:
      void argHintHidden();
      void argHintCompleted();
      void argHintAborted();

  public slots:
      virtual void reset( int, int );
      virtual void cursorPositionChanged( KateView*, int, int );

  private slots:
      void slotDone(bool completed);

  private:
      QMap<int, QString> m_functionMap;
      int m_currentFunction;
      QString m_wrapping;
      QString m_delimiter;
      bool m_markCurrentFunction;
      int m_currentLine;
      int m_currentCol;
      KateView* editorView;
      QIntDict<QLabel> labelDict;
      QLayout* layout;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
