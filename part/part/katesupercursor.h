/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>

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

#ifndef KATESUPERCURSOR_H
#define KATESUPERCURSOR_H

#include "katecursor.h"

class KateDocument;
class KateView;

/**
 * Possible additional features:
 * - Notification when a cursor enters or exits a view
 * - suggest something :)
 *
 * Unresolved issues:
 * - testing, testing, testing
 *   - ie. everything which hasn't already been tested, you can see that which has inside the #ifdefs
 * - api niceness
 */

/**
 * A cursor which updates and gives off various interesting signals.
 *
 * This aims to be a working version of what KateCursor was originally intended to be.
 *
 * @author Hamish Rodda
 **/
class KateSuperCursor : public QObject, public KateDocCursor, public Kate::Cursor
{
  Q_OBJECT

public:
    /**
     * bool privateC says: if private, than don't show to apps using the cursorinterface in the list,
     * all internally only used SuperCursors should be private or people could modify them from the
     * outside breaking kate's internals
     */
    KateSuperCursor(KateDocument* doc, bool privateC, const KateTextCursor& cursor, QObject* parent = 0L, const char* name = 0L);
    KateSuperCursor(KateDocument* doc, bool privateC, int lineNum = 0, int col = 0, QObject* parent = 0L, const char* name = 0L);

    ~KateSuperCursor ();

  public:
    // KTextEditor::Cursor interface
    void position(uint *line, uint *col) const;
    bool setPosition(uint line, uint col);
    bool insertText(const QString& text);
    bool removeText(uint numberOfCharacters);
    QChar currentChar() const;

    /**
    * @returns true if the cursor is situated at the start of the line, false if it isn't.
    */
    bool atStartOfLine() const;

    /**
    * @returns true if the cursor is situated at the end of the line, false if it isn't.
    */
    bool atEndOfLine() const;

    /**
    * Returns how this cursor behaves when text is inserted at the cursor.
    * Defaults to not moving on insert.
    */
    bool moveOnInsert() const;

    /**
    * Change the behavior of the cursor when text is inserted at the cursor.
    *
    * If @p moveOnInsert is true, the cursor will end up at the end of the insert.
    */
    void setMoveOnInsert(bool moveOnInsert);

    /**
    * Debug: output the position.
    */
    operator QString();

    // Reimplementations;
    virtual void setLine(int lineNum);
    virtual void setCol(int colNum);
    virtual void setPos(const KateTextCursor& pos);
    virtual void setPos(int lineNum, int colNum);

  signals:
    /**
    * The cursor's position was directly changed by the program.
    */
    void positionDirectlyChanged();

    /**
    * The cursor's position was changed.
    */
    void positionChanged();

    /**
    * Athough an edit took place, the cursor's position was unchanged.
    */
    void positionUnChanged();

    /**
    * The cursor's surrounding characters were both deleted simultaneously.
    * The cursor is automatically placed at the start of the deleted region.
    */
    void positionDeleted();

    /**
    * A character was inserted immediately before the cursor.
    *
    * Whether the char was inserted before or after this cursor depends on
    * moveOnInsert():
    * @li true -> the char was inserted before
    * @li false -> the char was inserted after
    */
    void charInsertedAt();

    /**
    * The character immediately before the cursor was deleted.
    */
    void charDeletedBefore();

    /**
    * The character immediately after the cursor was deleted.
    */
    void charDeletedAfter();

  //BEGIN METHODES TO CALL FROM KATE DOCUMENT TO KEEP CURSOR UP TO DATE
  public:
    void editTextInserted ( uint line, uint col, uint len);
    void editTextRemoved ( uint line, uint col, uint len);

    void editLineWrapped ( uint line, uint col, bool newLine = true );
    void editLineUnWrapped ( uint line, uint col, bool removeLine = true, uint length = 0 );

    void editLineInserted ( uint line );
    void editLineRemoved ( uint line );
  //END

  private:
    KateDocument *m_doc;
    bool m_moveOnInsert : 1;
    bool m_lineRemoved : 1;
    bool m_privateCursor : 1;
};

/**
 * Represents a range of text, from the start() to the end().
 *
 * Also tracks its position and emits useful signals.
 */
class KateSuperRange : public QObject, public KateRange
{
			  friend class KateSuperRangeList;

  Q_OBJECT

public:
  /// Determine how the range reacts to characters inserted immediately outside the range.
  enum InsertBehaviour {
    /// Don't expand to encapsulate new characters in either direction. This is the default.
    DoNotExpand = 0,
    /// Expand to encapsulate new characters to the left of the range.
    ExpandLeft = 0x1,
    /// Expand to encapsulate new characters to the right of the range.
    ExpandRight = 0x2
  };

  /**
   * Constructor.  Takes posession of @p start and @p end.
   */
  KateSuperRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent = 0L, const char* name = 0L);
  KateSuperRange(KateDocument* doc, const KateRange& range, QObject* parent = 0L, const char* name = 0L);
  KateSuperRange(KateDocument* doc, const KateTextCursor& start, const KateTextCursor& end, QObject* parent = 0L, const char* name = 0L);

  virtual ~KateSuperRange();

  // fulfill KateRange requirements
  virtual KateTextCursor& start();
  virtual KateTextCursor& end();
  virtual const KateTextCursor& start() const;
  virtual const KateTextCursor& end() const;

  void allowZeroLength(bool yes=true){m_allowZeroLength=yes;}
  /**
   * Returns the super start cursor.
   */
  KateSuperCursor& superStart();
  const KateSuperCursor& superStart() const;

  /**
   * Returns the super end cursor.
   */
  KateSuperCursor& superEnd();
  const KateSuperCursor& superEnd() const;

  /**
   * Returns how this range reacts to characters inserted immediately outside the range.
   */
  int behaviour() const;

  /**
   * Determine how the range should react to characters inserted immediately outside the range.
   *
   * TODO does this need a custom function to enable determining of the behavior based on the
   * text that is inserted / deleted?
   *
   * @sa InsertBehaviour
   */
  void setBehaviour(int behaviour);

  /**
   * Start and end must be valid and start <= end.
   */
  virtual bool isValid() const;

  /**
   * This is for use where the ranges are used in a heirachy,
   * ie. their parents are KateSuperRanges which completely
   * encapsulate them.
   *
   * @todo constrain children when their position changes deliberately;
   *       eliminate() children when they are equivalent to their parents
   *
   * @returns true if the range contains the cursor and no children
   *          also contain it; false otherwise.
   */
  bool owns(const KateTextCursor& cursor) const;

  /**
   * Returns true if the range includes @p cursor 's character.
   * Returns false if @p cursor == end().
   */
  bool includes(const KateTextCursor& cursor) const;

  /**
   * Returns true if the range includes @p line
   */
  bool includes(uint lineNum) const;

  /**
   * Returns true if the range totally encompasses @p line
   */
  bool includesWholeLine(uint lineNum) const;

  /**
   * Returns whether @p cursor is the site of a boundary of this range.
   */
  bool boundaryAt(const KateTextCursor& cursor) const;

  /**
   * Returns whether there is a boundary of this range on @p line.
   */
  bool boundaryOn(uint lineNum) const;

signals:
  /**
   * More interesting signals that aren't worth implementing here:
   *  firstCharDeleted: start()::charDeleted()
   *  lastCharDeleted: end()::previousCharDeleted()
   */

  /**
   * The range's position changed.
   */
  void positionChanged();

  /**
   * The range's position was unchanged.
   */
  void positionUnChanged();

  /**
   * The contents of the range changed.
   */
  void contentsChanged();

  /**
   * Either cursor's surrounding characters were both deleted.
   */
  void boundaryDeleted();

  /**
   * The range now contains no characters (ie. the start and end cursors are the same).
   *
   * To eliminate this range under different conditions, connect the other signal directly
   * to this signal.
   */
  void eliminated();

  /**
   * Indicates the region needs re-drawing.
   */
  void tagRange(KateSuperRange* range);

public slots:
  void slotTagRange();

private slots:
  void slotEvaluateChanged();
  void slotEvaluateUnChanged();

private:
  void init();
  void evaluateEliminated();
  void evaluatePositionChanged();

  KateSuperCursor* m_start;
  KateSuperCursor* m_end;
  bool m_evaluate;
  bool m_startChanged;
  bool m_endChanged;
  bool m_deleteCursors;
  bool m_allowZeroLength;
};

class KateSuperCursorList : public QPtrList<KateSuperCursor>
{
protected:
  virtual int compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2);
};

class KateSuperRangeList : public QObject, public QPtrList<KateSuperRange>
{
  Q_OBJECT

public:
  /**
   * @sa autoManage()
   */
  KateSuperRangeList(bool autoManage = true, QObject* parent = 0L, const char* name = 0L);

  /**
   * Semi-copy constructor.
   *
   * Does not copy auto-manage value, as that would make it too easy to perform
   * double-deletions.
   *
   * Also, does not connect signals and slots to save time, as this is mainly
   * used by the document itself while drawing (call connectAll() to re-constitute)
   */
  KateSuperRangeList(const QPtrList<KateSuperRange>& rangeList, QObject* parent = 0L, const char* name = 0L);

  virtual ~KateSuperRangeList() {}
  /**
   * Append another list.
   * If this object was created by the semi-copy constructor, it may not connect items
   * (unless connectAll() has already been called), call connectAll().
   */
  void appendList(const QPtrList<KateSuperRange>& rangeList);

  /**
   * Connect items that are not connected. This only needs to be called once,
   * and only if this was created with the semi-copy constructor.
   */
  void connectAll();

  /**
   * Override to emit rangeEliminated() signals.
   */
  virtual void clear();

  /**
   * Automanage is a combination of autodeleting of the objects and
   * removing of any eliminated() ranges.
   */
  bool autoManage() const;

  /**
   * @sa autoManage()
   */
  void setAutoManage(bool autoManage);

  /**
   * This is just a straight-forward list so that there is no confusion about whether
   * this list should be auto-managed (ie. it shouldn't, to avoid double deletions).
   */
  QPtrList<KateSuperRange> rangesIncluding(const KateTextCursor& cursor);
  QPtrList<KateSuperRange> rangesIncluding(uint line);

  /**
   * @retval true if one of the ranges in the list includes @p cursor
   * @retval false otherwise
   */
  bool rangesInclude(const KateTextCursor& cursor);

  /**
   * Construct a list of boundaries, and return the first, or 0L if there are none.
   * If @p start is defined, the first boundary returned will be at or after @p start.
   *
   * @returns the first boundary location
   */
  KateSuperCursor* firstBoundary(const KateTextCursor* start = 0L);

  /**
   * @returns the next boundary, or 0L if there are no more.
   */
  KateSuperCursor* nextBoundary();

  /**
   * @returns the current boundary
   */
  KateSuperCursor* currentBoundary();

signals:
  /**
   * The range now contains no characters (ie. the start and end cursors are the same).
   * If autoManage() is true, the range will be deleted after the signal has processed.
   */
  void rangeEliminated(KateSuperRange* range);

  /**
   * There are no ranges left.
   */
  void listEmpty();

  /**
   * Connected to all ranges if connect()ed.
   */
  void tagRange(KateSuperRange* range);

protected:
  /**
   * internal reimplementation
   */
  virtual int compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2);

  /**
   * internal reimplementation
   */
  virtual QPtrCollection::Item newItem(QPtrCollection::Item d);

private slots:
  void slotEliminated();
  void slotDeleted(QObject* range);

private:
  bool m_autoManage;
  bool m_connect;

  KateSuperCursorList m_columnBoundaries;
  bool m_trackingBoundaries;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
