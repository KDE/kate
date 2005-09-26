/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_VIEW_HELPERS_H__
#define __KATE_VIEW_HELPERS_H__

#include <kaction.h>
#include <klineedit.h>

#include <QPixmap>
#include <QColor>
#include <QScrollBar>
#include <QHash>

class KateDocument;
class KateView;
class KateViewInternal;
class QActionGroup;

namespace KTextEditor {
  class Command;
}

/**
 * This class is required because QScrollBar's sliderMoved() signal is
 * really supposed to be a sliderDragged() signal... so this way we can capture
 * MMB slider moves as well
 *
 * Also, it adds some usefull indicators on the scrollbar.
 */
class KateScrollBar : public QScrollBar
{
  Q_OBJECT

  public:
    KateScrollBar(Qt::Orientation orientation, class KateViewInternal *parent, const char* name = 0L);

    inline bool showMarks() { return m_showMarks; };
    inline void setShowMarks(bool b) { m_showMarks = b; update(); };

  signals:
    void sliderMMBMoved(int value);

  protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent (QMouseEvent* e);
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void styleChange(QStyle &oldStyle);
    virtual void sliderChange ( SliderChange change );

  protected slots:
    void sliderMaybeMoved(int value);
    void marksChanged();

  private:
    void redrawMarks();
    void recomputeMarksPositions(bool forceFullUpdate = false);
    void watchScrollBarSize();

  bool m_middleMouseDown;

    KateView *m_view;
    KateDocument *m_doc;
    class KateViewInternal *m_viewInternal;

    int m_topMargin;
    int m_bottomMargin;
    int m_savVisibleLines;

    QHash<int, QColor> m_lines;

    bool m_showMarks;
};

class KateCmdLine : public KLineEdit
{
  Q_OBJECT

  public:
    KateCmdLine (KateView *view);
    virtual bool event(QEvent *e);
  private slots:
    void slotReturnPressed ( const QString& cmd );
    void hideMe ();

  protected:
    void focusInEvent ( QFocusEvent *ev );
    void keyPressEvent( QKeyEvent *ev );

  private:
    void fromHistory( bool up );
    QString helptext( const QPoint & ) const;

    KateView *m_view;
    bool m_msgMode;
    QString m_oldText;
    uint m_histpos; ///< position in the history
    uint m_cmdend; ///< the point where a command ends in the text, if we have a valid one.
    KTextEditor::Command *m_command; ///< For completing flags/args and interactiveness
    class KCompletion *m_oldCompletionObject; ///< save while completing command args.
    class KateCmdLnWhatsThis *m_help;
};

class KateIconBorder : public QWidget
{
  Q_OBJECT

  public:
    KateIconBorder( KateViewInternal* internalView, QWidget *parent );

    // VERY IMPORTANT ;)
    virtual QSize sizeHint() const;

    void updateFont();
    int lineNumberWidth() const;

    void setIconBorderOn(     bool enable );
    void setLineNumbersOn(    bool enable );
    void setDynWrapIndicators(int state );
    int dynWrapIndicators()  const { return m_dynWrapIndicators; }
    bool dynWrapIndicatorsOn() const { return m_dynWrapIndicatorsOn; }
    void setFoldingMarkersOn( bool enable );
    void toggleIconBorder()     { setIconBorderOn(     !iconBorderOn() );     }
    void toggleLineNumbers()    { setLineNumbersOn(    !lineNumbersOn() );    }
    void toggleFoldingMarkers() { setFoldingMarkersOn( !foldingMarkersOn() ); }
    bool iconBorderOn()       const { return m_iconBorderOn;     }
    bool lineNumbersOn()      const { return m_lineNumbersOn;    }
    bool foldingMarkersOn()   const { return m_foldingMarkersOn; }

    enum BorderArea { None, LineNumbers, IconBorder, FoldingMarkers };
    BorderArea positionToArea( const QPoint& ) const;

  signals:
    void toggleRegionVisibility( unsigned int );

  private:
    void paintEvent( QPaintEvent* );
    void paintBorder (int x, int y, int width, int height);

    void mousePressEvent( QMouseEvent* );
    void mouseMoveEvent( QMouseEvent* );
    void mouseReleaseEvent( QMouseEvent* );
    void mouseDoubleClickEvent( QMouseEvent* );

    void showMarkMenu( uint line, const QPoint& pos );

    KateView *m_view;
    KateDocument *m_doc;
    KateViewInternal *m_viewInternal;

    bool m_iconBorderOn:1;
    bool m_lineNumbersOn:1;
    bool m_foldingMarkersOn:1;
    bool m_dynWrapIndicatorsOn:1;
    int m_dynWrapIndicators;

    int m_lastClickedLine;

    int m_cachedLNWidth;

    int m_maxCharWidth;

    mutable QPixmap m_arrow;
    mutable QColor m_oldBackgroundColor;
    
    QPixmap minus_px;
    QPixmap plus_px;
};

class KateViewEncodingAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewEncodingAction(KateDocument *_doc, KateView *_view, const QString& text, KActionCollection* parent = 0, const char* name = 0);

    ~KateViewEncodingAction(){;};

  private:
    KateDocument* doc;
    KateView *view;
    QActionGroup *m_actions;

  private slots:
    void setMode (QAction*);
    void slotAboutToShow();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
