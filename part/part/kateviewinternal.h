#ifndef _KATE_VIEW_INTERNAL_
#define _KATE_VIEW_INTERNAL_

#include "kateview.h"
#include <qintdict.h>
class KateViewInternal : public QWidget
{
    Q_OBJECT
    friend class KateDocument;
    friend class KateView;
    friend class KateIconBorder;
    friend class CodeCompletion_Impl;

  private:
    //uint iconBorderWidth;
    uint iconBorderHeight;

  public:
    KateViewInternal(KateView *view, KateDocument *doc);
    ~KateViewInternal();

    void doCursorCommand(VConfig &, int cmdNum);
    void doEditCommand(VConfig &, int cmdNum);

    void cursorLeft(VConfig &);
    void cursorRight(VConfig &);
    void wordLeft(VConfig &);
    void wordRight(VConfig &);
    void home(VConfig &);
    void end(VConfig &);
    void cursorUp(VConfig &);
    void cursorDown(VConfig &);
    void scrollUp(VConfig &);
    void scrollDown(VConfig &);
    void topOfView(VConfig &);
    void bottomOfView(VConfig &);
    void pageUp(VConfig &);
    void pageDown(VConfig &);
    void cursorPageUp(VConfig &);
    void cursorPageDown(VConfig &);
    void top(VConfig &);
    void bottom(VConfig &);
    void top_home(VConfig &c);
    void bottom_end(VConfig &c);

  private slots:
    void changeXPos(int);
    void changeYPos(int);
    void tripleClickTimeout();

  private:
    void getVConfig(VConfig &);
    void changeState(VConfig &);
    void insLine(int line);
    void delLine(int line);
    void updateCursor();
    void updateCursor(VConfig &c,bool keepSel=false);//KateTextCursor &newCursor);
    void updateCursor(KateTextCursor &newCursor, bool keepSel=false);
    void clearDirtyCache(int height);
    void tagLines(int start, int end, int x1, int x2);
    void tagAll();
    void setPos(int x, int y);
    void center();

    void updateView(int flags);

    void paintTextLines(int xPos, int yPos);
    void paintCursor();
    void paintBracketMark();

    void placeCursor(int x, int y, int flags = 0);
    bool isTargetSelected(int x, int y);

    void doDrag();

    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent( QWheelEvent *e );
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void timerEvent(QTimerEvent *);

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

    KateView *myView;
    KateDocument *myDoc;
    QScrollBar *xScroll;
    QScrollBar *yScroll;
    KateIconBorder *leftBorder;


public:
    void clear();
    KateTextCursor& getCursor(){return cursor;}
    void setCursor (KateTextCursor c){cursor=c;}
    void resizeDrawBuffer(int w, int h){drawBuffer->resize(w,h);}
    QPoint cursorCoordinates(){return QPoint(xCoord,yCoord);}
    QPoint originCoordinates(){return QPoint(xPos,yPos);}
private:
    void calculateDisplayPositions(KateTextCursor &, KateTextCursor, bool, bool);

    QIntDict <int> m_lineMapping;

    // cursor position in pixels:
    int xCoord;
    int yCoord;

    int xPos;
    int yPos;

    int mouseX;
    int mouseY;
    int scrollX;
    int scrollY;
    int scrollTimer;

    KateTextCursor cursor;
    KateTextCursor displayCursor;
    bool cursorOn;
    int cursorTimer;
    int cXPos;
    int cOldXPos;

    int startLine;
    int endLine;
    uint maxLen;

    bool possibleTripleClick;
    bool exposeCursor;
    int updateState;
    uint numLines;
    class KateLineRange *lineRanges;
    uint lineRangesLen;
    int newXPos;
    int newYPos;

    QPixmap *drawBuffer;

    BracketMark bm;

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState       state;
      KateTextCursor      start;
      QTextDrag       *dragObject;
    } dragInfo;

  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);

   protected slots:
   void singleShotUpdateView();


};
#endif
