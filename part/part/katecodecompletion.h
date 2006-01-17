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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/
/* Trolltech doesn't mind, if we license that piece of code as LGPL, because there isn't much
 * left from the desigener code */


#ifndef __KateCodeCompletion_H__
#define __KateCodeCompletion_H__

#include <ktexteditor/codecompletioninterface.h>

#include <QList>
#include <QLinkedList>
#include <qstringlist.h>
#include <qlabel.h>
#include <QFrame>
#include <qmap.h>
#include <QHash>

#include <kdebug.h>

class KateView;
class KateArgHint;
class KateCCListBox;

class KVBox;
class QLayout;

class KateCodeCompletionCommentLabel : public QLabel
{
  Q_OBJECT

  public:
    KateCodeCompletionCommentLabel( QWidget* parent, const QString& text) : QLabel( parent, "toolTipTip",
             Qt::WStyle_StaysOnTop | Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_Tool | Qt::WX11BypassWM )
    {
        setMargin(1);
        setIndent(0);
       // setAutoMask( false );
        setFrameStyle( QFrame::Plain | QFrame::Box );
        setLineWidth( 1 );
        setAlignment( Qt::AlignLeft | Qt::AlignTop );
        ensurePolished();
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

    void showCompletion(const KTextEditor::Cursor &position,const QLinkedList<KTextEditor::CompletionData> &data);

    void showArgHint(
        QStringList functionList, const QString& strWrapping, const QString& strDelimiter );
/*    void showCompletionBox(
        QList<KTextEditor::CompletionItem> entries, int offset = 0, bool casesensitive = true );*/
    bool eventFilter( QObject* o, QEvent* e );

    void handleKey (QKeyEvent *e);

  public Q_SLOTS:
    void slotCursorPosChanged();
    void showComment();
    void updateBox () { updateBox(false); }

  Q_SIGNALS:
    void argHintHidden();

  private:
    void doComplete();
    void abortCompletion();
    void complete( KTextEditor::CompletionItem );
    void updateBox( bool newCoordinate );

    KateArgHint*    m_pArgHint;
    KateView*       m_view;
    KVBox*          m_completionPopup;
    KateCCListBox*       m_completionListBox;
    //QList<KTextEditor::CompletionItem> m_complList;
    int            m_lineCursor;
    int            m_colCursor;
    int             m_offset;
    bool            m_caseSensitive;
    KateCodeCompletionCommentLabel* m_commentLabel;

    friend class KateCompletionItem;
    class CompletionItem {
    public:
      CompletionItem(const KTextEditor::CompletionData* _data,int _index):data(_data),index(_index) {}
      const KTextEditor::CompletionData* data;
      int index;
      int col;
      inline bool operator <(const CompletionItem& other) const {
        const KTextEditor::CompletionItem& oi(other.data->items().at(other.index));
        const KTextEditor::CompletionItem& ti(data->items().at(index));
        //longest match first, implement more accurately
        bool longer=(data->matchStart().column()<other.data->matchStart().column());
        bool equal=(data->matchStart().column()==other.data->matchStart().column());
        bool result= longer || (equal &&(oi.text()>ti.text()));
        return result;

      };
      inline CompletionItem& operator=(const CompletionItem& c) {data=c.data;index=c.index; return *this;} //FIXME
      inline const QString& text() const {
#if 0
        kdDebug(13035)<<"data="<<data<<endl;
        kdDebug(13035)<<"data->items().size()="<<data->items().size()<<endl;
#endif
        return data->items().at(index).text();
      }
      inline KTextEditor::CompletionItem item() const {
#if 0
        kdDebug(13035)<<"data="<<data<<endl;
        kdDebug(13035)<<"data->items().size()="<<data->items().size()<<endl;
#endif
        return data->items().at(index);
      }
    };
    QList<CompletionItem> m_items;
    QLinkedList<KTextEditor::CompletionData> m_data;
    void buildItemList();
    bool m_blockEvents;
};

class KateArgHint: public QFrame
{
  Q_OBJECT

  public:
      KateArgHint( KateView* =0 );
      virtual ~KateArgHint();

      virtual void setCurrentFunction( int );
      virtual int currentFunction() const { return m_currentFunction; }

      void setArgMarkInfos( const QString&, const QString& );

      virtual void addFunction( int, const QString& );
      QString functionAt( int id ) const { return m_functionMap[ id ]; }

      virtual void show();
      virtual void adjustSize();
      virtual bool eventFilter( QObject*, QEvent* );

  Q_SIGNALS:
      void argHintHidden();
      void argHintCompleted();
      void argHintAborted();

  public Q_SLOTS:
      virtual void reset( int, int );
      virtual void cursorPositionChanged( KateView*, int, int );

  private Q_SLOTS:
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
      QHash<int,QLabel*> labelDict;
      QLayout* layout;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
