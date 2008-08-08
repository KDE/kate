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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_VIEW_HELPERS_H__
#define __KATE_VIEW_HELPERS_H__

#include <kcodecaction.h>
#include <kencodingprober.h>
#include <klineedit.h>

#include <QtGui/QPixmap>
#include <QtGui/QColor>
#include <QtGui/QScrollBar>
#include <QtCore/QHash>
#include <QtGui/QStackedLayout>
#include <QtCore/QMap>

class KateDocument;
class KateView;
class KateViewInternal;

#define MAXFOLDINGCOLORS 16

class KateLineInfo;

namespace KTextEditor {
  class Command;
  class SmartRange;
  class AnnotationModel;
}

/**
 * This class is required because QScrollBar's sliderMoved() signal is
 * really supposed to be a sliderDragged() signal... so this way we can capture
 * MMB slider moves as well
 *
 * Also, it adds some useful indicators on the scrollbar.
 */
class KateScrollBar : public QScrollBar
{
  Q_OBJECT

  public:
    KateScrollBar(Qt::Orientation orientation, class KateViewInternal *parent);

    inline bool showMarks() { return m_showMarks; }
    inline void setShowMarks(bool b) { m_showMarks = b; update(); }

  Q_SIGNALS:
    void sliderMMBMoved(int value);

  protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent (QMouseEvent* e);
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void styleChange(QStyle &oldStyle);
    virtual void sliderChange ( SliderChange change );

  protected Q_SLOTS:
    void sliderMaybeMoved(int value);
    void marksChanged();

  private:
    void redrawMarks();
    void recomputeMarksPositions();

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

class KateIconBorder : public QWidget
{
  Q_OBJECT

  public:
    KateIconBorder( KateViewInternal* internalView, QWidget *parent );
    virtual ~KateIconBorder();
    // VERY IMPORTANT ;)
    virtual QSize sizeHint() const;

    void updateFont();
    int lineNumberWidth() const;

    void setIconBorderOn(       bool enable );
    void setLineNumbersOn(      bool enable );
    void setAnnotationBorderOn( bool enable );
    void setDynWrapIndicators(int state );
    int dynWrapIndicators()  const { return m_dynWrapIndicators; }
    bool dynWrapIndicatorsOn() const { return m_dynWrapIndicatorsOn; }
    void setFoldingMarkersOn( bool enable );
    void toggleIconBorder()     { setIconBorderOn(     !iconBorderOn() );     }
    void toggleLineNumbers()    { setLineNumbersOn(    !lineNumbersOn() );    }
    void toggleFoldingMarkers() { setFoldingMarkersOn( !foldingMarkersOn() ); }
    inline bool iconBorderOn()       const { return m_iconBorderOn;       }
    inline bool lineNumbersOn()      const { return m_lineNumbersOn;      }
    inline bool foldingMarkersOn()   const { return m_foldingMarkersOn;   }
    inline bool annotationBorderOn() const { return m_annotationBorderOn; }

    enum BorderArea { None, LineNumbers, IconBorder, FoldingMarkers, AnnotationBorder };
    BorderArea positionToArea( const QPoint& ) const;

  Q_SIGNALS:
    void toggleRegionVisibility( unsigned int );
  public Q_SLOTS:
    void updateAnnotationBorderWidth();
    void updateAnnotationLine( int line );
    void annotationModelChanged( KTextEditor::AnnotationModel* oldmodel, KTextEditor::AnnotationModel* newmodel );

  private:
    void paintEvent( QPaintEvent* );
    void paintBorder (int x, int y, int width, int height);

    void mousePressEvent( QMouseEvent* );
    void mouseMoveEvent( QMouseEvent* );
    void mouseReleaseEvent( QMouseEvent* );
    void mouseDoubleClickEvent( QMouseEvent* );
    void leaveEvent(QEvent *event);

    void showMarkMenu( uint line, const QPoint& pos );

    void showAnnotationTooltip( int line, const QPoint& pos );
    void hideAnnotationTooltip();
    void removeAnnotationHovering();
    void showAnnotationMenu( int line, const QPoint& pos);
    int annotationLineWidth( int line );

    KateView *m_view;
    KateDocument *m_doc;
    KateViewInternal *m_viewInternal;

    bool m_iconBorderOn:1;
    bool m_lineNumbersOn:1;
    bool m_foldingMarkersOn:1;
    bool m_dynWrapIndicatorsOn:1;
    bool m_annotationBorderOn:1;
    int m_dynWrapIndicators;

    int m_lastClickedLine;

    int m_cachedLNWidth;

    int m_maxCharWidth;
    int iconPaneWidth;
    int m_annotationBorderWidth;

    mutable QPixmap m_arrow;
    mutable QColor m_oldBackgroundColor;

    KTextEditor::SmartRange *m_foldingRange;
    int m_lastBlockLine;
    void showBlock(int line);
    void hideBlock();

    QBrush m_foldingColors[MAXFOLDINGCOLORS];
    QBrush m_foldingColorsSolid[MAXFOLDINGCOLORS];
    const QBrush &foldingColor(KateLineInfo *, int,bool solid);
    QString m_hoveredAnnotationText;
    
    void initializeFoldingColors();
};

class KateViewEncodingAction : public KCodecAction
{
  Q_OBJECT

  public:
    KateViewEncodingAction(KateDocument *_doc, KateView *_view, const QString& text, QObject *parent);

    ~KateViewEncodingAction(){}

  private:
    KateDocument* doc;
    KateView *view;

  private Q_SLOTS:
    void setEncoding (const QString &e);
    void setProberTypeForEncodingAutoDetection (KEncodingProber::ProberType);
    void slotAboutToShow();
};

class KateViewBar;

class KateViewBarWidget : public QWidget
{
  Q_OBJECT

  public:
    KateViewBarWidget (KateViewBar *viewBar);

    KateViewBar *viewBar () { return m_viewBar; }
    QWidget *centralWidget () { return m_centralWidget; }

  public Q_SLOTS:
    void showBar ();
    void hideBar ();

  protected:
    // allow subclass to avoid hiding...
    virtual bool hideIsTriggered () { return true; }

  private:
    KateViewBar *m_viewBar;
    QWidget *m_centralWidget;
};

// Helper layout class to always provide minimum size
class KateStackedLayout : public QStackedLayout
{
  Q_OBJECT
public:
  KateStackedLayout(QWidget* parent);
  virtual QSize sizeHint() const;
  virtual QSize minimumSize() const;
};

class KateViewBar : public QWidget
{
  Q_OBJECT

  friend class KateViewBarWidget;

  public:
    KateViewBar (QWidget *parent,KateView *view);

    KateView *view () { return m_view; }

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void hideEvent(QHideEvent* event);

  private:
    void addBarWidget (KateViewBarWidget *newBarWidget);
    void showBarWidget (KateViewBarWidget *barWidget);
  public:
    void hideBarWidget ();

  private:
    KateView *m_view;
    KateStackedLayout *m_stack;
};

class KateCmdLine : public KateViewBarWidget
{
  public:
    explicit KateCmdLine(KateView *view, KateViewBar *viewBar);
    ~KateCmdLine();

  private:
    class KateCmdLineEdit *m_lineEdit;
};

class KateCmdLineEdit : public KLineEdit
{
  Q_OBJECT

  public:
    KateCmdLineEdit (KateCmdLine *bar, KateView *view);
    virtual bool event(QEvent *e);
  private Q_SLOTS:
    void slotReturnPressed ( const QString& cmd );
    void hideBar ();

  protected:
    void focusInEvent ( QFocusEvent *ev );
    void keyPressEvent( QKeyEvent *ev );

  private:
    void fromHistory( bool up );
    QString helptext( const QPoint & ) const;

    KateView *m_view;
    KateCmdLine *m_bar;
    bool m_msgMode;
    QString m_oldText;
    uint m_histpos; ///< position in the history
    uint m_cmdend; ///< the point where a command ends in the text, if we have a valid one.
    KTextEditor::Command *m_command; ///< For completing flags/args and interactiveness
    class KCompletion *m_oldCompletionObject; ///< save while completing command args.
    class KateCmdLnWhatsThis *m_help;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
