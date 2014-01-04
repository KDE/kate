/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katetextfolding.h"
#include "katetextbuffer.h"
#include "katetextrange.h"

namespace Kate {
  
TextFolding::FoldingRange::FoldingRange (TextBuffer &buffer, const KTextEditor::Range &range, FoldingRangeFlags _flags)
  : start (new TextCursor (buffer, range.start(), KTextEditor::MovingCursor::MoveOnInsert))
  , end (new TextCursor (buffer, range.end(), KTextEditor::MovingCursor::MoveOnInsert))
  , parent (0)
  , flags (_flags)
  , id (-1)
{
}
  
TextFolding::FoldingRange::~FoldingRange ()
{
  /**
   * kill all our data!
   * this will recurse all sub-structures!
   */
  delete start;
  delete end;
  qDeleteAll (nestedRanges);
}
  
TextFolding::TextFolding (TextBuffer &buffer)
  : QObject ()
  , m_buffer (buffer)
  , m_idCounter (-1)
{
  /**
   * connect needed signals from buffer
   */
  connect (&m_buffer, SIGNAL(cleared()), SLOT(clear()));
}

TextFolding::~TextFolding ()
{
  /**
   * only delete the folding ranges, the folded ranges and mapped ranges are the same objects
   */
  qDeleteAll (m_foldingRanges);
}

void TextFolding::clear ()
{
  /**
   * reset counter
   */
  m_idCounter = -1;
  
  /**
   * no ranges, no work
   */
  if (m_foldingRanges.isEmpty()) {
    /**
     * assert all stuff is consistent and return!
     */
    Q_ASSERT (m_idToFoldingRange.isEmpty());
    Q_ASSERT (m_foldedFoldingRanges.isEmpty());
    return;
  }
  
  /**
   * cleanup
   */
  m_idToFoldingRange.clear();
  m_foldedFoldingRanges.clear();
  qDeleteAll (m_foldingRanges);
  m_foldingRanges.clear ();
  
  /**
   * folding changed!
   */
  emit foldingRangesChanged ();
}

qint64 TextFolding::newFoldingRange (const KTextEditor::Range &range, FoldingRangeFlags flags)
{
  /**
   * sort out invalid and empty ranges
   * that makes no sense, they will never grow again!
   */
  if (!range.isValid() || range.isEmpty())
    return -1;
  
  /**
   * create new folding region that we want to insert
   * this will internally create moving cursors!
   */
  FoldingRange *newRange = new FoldingRange (m_buffer, range, flags);
  
  /**
   * the construction of the text cursors might have invalidated this
   * check and bail out if that happens
   * bail out, too, if it can't be inserted!
   */
  if (    !newRange->start->isValid()
       || !newRange->end->isValid()
       || !insertNewFoldingRange (0 /* no parent here */, m_foldingRanges, newRange)) {
    /**
     * cleanup and be done
     */
    delete newRange;
    return -1;
  }

  /**
   * set id, catch overflows, even if they shall not happen
   */
  newRange->id = ++m_idCounter;
  if (newRange->id < 0)
    newRange->id = m_idCounter = 0;
  
  /**
   * remember the range
   */
  m_idToFoldingRange.insert (newRange->id, newRange);
  
  /**
   * update our folded ranges vector!
   */
  bool updated = updateFoldedRangesForNewRange (newRange);
  
  /**
   * emit that something may have changed
   * do that only, if updateFoldedRangesForNewRange did not already do the job!
   */
  if (!updated)
    emit foldingRangesChanged ();
  
  /**
   * all went fine, newRange is now registered internally!
   */
  return newRange->id;
}

KTextEditor::Range TextFolding::foldingRange(qint64 id) const
{
  FoldingRange* range = m_idToFoldingRange.value (id);
  if (!range)
    return KTextEditor::Range::invalid();

  return KTextEditor::Range(range->start->toCursor(), range->end->toCursor());
}

bool TextFolding::foldRange (qint64 id)
{
  /**
   * try to find the range, else bail out
   */
  FoldingRange *range = m_idToFoldingRange.value (id);
  if (!range)
    return false;
  
  /**
   * already folded? nothing to do
   */
  if (range->flags & Folded)
    return true;
  
  /**
   * fold and be done
   */
  range->flags |= Folded;
  updateFoldedRangesForNewRange (range);
  return true;
}
                      
bool TextFolding::unfoldRange (qint64 id, bool remove)
{
  /**
   * try to find the range, else bail out
   */
  FoldingRange *range = m_idToFoldingRange.value (id);
  if (!range)
    return false;
  
  /**
   * nothing to do?
   * range is already unfolded and we need not to remove it!
   */
  if (!remove && !(range->flags & Folded))
    return true;
  
  /**
   * do we need to delete the range?
   */
  const bool deleteRange = remove || !(range->flags & Persistent);
  
  /**
   * first: remove the range, if forced or non-persistent!
   */
  if (deleteRange) {
    /**
     * remove from outside visible mapping!
     */
    m_idToFoldingRange.remove (id);
    
    /**
     * remove from folding vectors!
     * FIXME: OPTIMIZE
     */
    FoldingRange::Vector &parentVector = range->parent ? range->parent->nestedRanges : m_foldingRanges;
    FoldingRange::Vector newParentVector;
    Q_FOREACH (FoldingRange *curRange, parentVector) {
      /**
       * insert our nested ranges and reparent them
       */
      if (curRange == range) {
        Q_FOREACH (FoldingRange *newRange, range->nestedRanges) {
          newRange->parent = range->parent;
          newParentVector.push_back (newRange);
        }
      
        continue;
      }
      
      /**
       * else just transfer elements
       */
      newParentVector.push_back (curRange);
    }
    parentVector = newParentVector;
  }
  
  /**
   * second: unfold the range, if needed!
   */
  bool updated = false;
  if (range->flags & Folded) {
    range->flags &= ~Folded;
    updated = updateFoldedRangesForRemovedRange (range);
  }
  
  /**
   * emit that something may have changed
   * do that only, if updateFoldedRangesForRemoveRange did not already do the job!
   */
  if (!updated)
    emit foldingRangesChanged ();
  
  /**
   * really delete the range, if needed!
   */
  if (deleteRange) {
    /**
     * clear ranges first, they got moved!
     */
    range->nestedRanges.clear ();
    delete range;
  }
  
  /**
   * be done ;)
   */
  return true;
}
    
bool TextFolding::isLineVisible (int line, qint64 *foldedRangeId) const
{
  /**
   * skip if nothing folded
   */
  if (m_foldedFoldingRanges.isEmpty())
    return true;
  
  /**
   * search upper bound, index to item with start line higher than our one
   */
  FoldingRange::Vector::const_iterator upperBound = qUpperBound (m_foldedFoldingRanges.begin(), m_foldedFoldingRanges.end(), line, compareRangeByStartWithLine);
  if (upperBound != m_foldedFoldingRanges.begin())
    --upperBound;
  
  /**
   * check if we overlap with the range in front of us
   */
  const bool hidden = (((*upperBound)->end->line() >= line) && (line > (*upperBound)->start->line()));
  
  /**
   * fill in folded range id, if needed
   */
  if (foldedRangeId)
    (*foldedRangeId) = hidden ? (*upperBound)->id : -1;
  
  /**
   * visible == !hidden
   */
  return !hidden;
}

void TextFolding::ensureLineIsVisible (int line)
{
  /**
   * skip if nothing folded
   */
  if (m_foldedFoldingRanges.isEmpty())
    return;
  
  /**
   * while not visible, unfold
   */
  qint64 foldedRangeId = -1;
  while (!isLineVisible (line, &foldedRangeId)) {
    /**
     * id should be valid!
     */
    Q_ASSERT (foldedRangeId >= 0);
    
    /**
     * unfold shall work!
     */
    const bool unfolded = unfoldRange (foldedRangeId);
    (void) unfolded;
    Q_ASSERT (unfolded);
  }
}

int TextFolding::visibleLines () const
{
  /**
   * start with all lines we have
   */
  int visibleLines = m_buffer.lines();
  
  /**
   * skip if nothing folded
   */
  if (m_foldedFoldingRanges.isEmpty())
    return visibleLines;
  
  /**
   * count all folded lines and subtract them from visible lines
   */
  Q_FOREACH (FoldingRange *range, m_foldedFoldingRanges)
    visibleLines -= (range->end->line() - range->start->line());
    
  /**
   * be done, assert we did no trash
   */
  Q_ASSERT (visibleLines > 0);
  return visibleLines;
}

int TextFolding::lineToVisibleLine (int line) const
{
  /**
   * valid input needed!
   */
  Q_ASSERT (line >= 0);
  
  /**
   * start with identity
   */
  int visibleLine = line;
  
  /**
   * skip if nothing folded or first line
   */
  if (m_foldedFoldingRanges.isEmpty() || (line == 0))
    return visibleLine;
  
  /**
   * walk over all folded ranges until we reach the line
   * keep track of seen visible lines, for the case we want to convert a hidden line!
   */
  int seenVisibleLines = 0;
  int lastLine = 0;
  Q_FOREACH (FoldingRange *range, m_foldedFoldingRanges) {
    /**
     * abort if we reach our line!
     */
    if (range->start->line() >= line)
      break;
    
    /**
     * count visible lines
     */
    seenVisibleLines += (range->start->line() - lastLine);
    lastLine = range->end->line();
    
    /**
     * we might be contained in the region, then we return last visible line
     */
    if (line <= range->end->line())
      return seenVisibleLines;
    
    /**
     * subtrace folded lines
     */
    visibleLine -= (range->end->line() - range->start->line());
  }
  
  /**
   * be done, assert we did no trash
   */
  Q_ASSERT (visibleLine >= 0);
  return visibleLine;
}
    
int TextFolding::visibleLineToLine (int visibleLine) const
{
  /**
   * valid input needed!
   */
  Q_ASSERT (visibleLine >= 0);
  
  /**
   * start with identity
   */
  int line = visibleLine;
  
  /**
   * skip if nothing folded or first line
   */
  if (m_foldedFoldingRanges.isEmpty() || (visibleLine == 0))
    return line;
  
  /**
   * last visible line seen, as line in buffer
   */
  int seenVisibleLines = 0;
  int lastLine = 0;
  int lastLineVisibleLines = 0;
  Q_FOREACH (FoldingRange *range, m_foldedFoldingRanges) {
    /**
     * else compute visible lines and move last seen
     */
    lastLineVisibleLines = seenVisibleLines;
    seenVisibleLines += (range->start->line() - lastLine);
    
    /**
     * bail out if enough seen
     */
    if (seenVisibleLines >= visibleLine)
      break;
    
    lastLine = range->end->line();
  }
  
  /**
   * check if still no enough visible!
   */
  if (seenVisibleLines < visibleLine)
    lastLineVisibleLines = seenVisibleLines;
  
  /**
   * compute visible line
   */
  line = (lastLine + (visibleLine - lastLineVisibleLines));
  Q_ASSERT (line >= 0);
  return line;
}

QVector<QPair<qint64, TextFolding::FoldingRangeFlags> > TextFolding::foldingRangesStartingOnLine (int line) const
{
  /**
   * results vector
   */
  QVector<QPair<qint64, TextFolding::FoldingRangeFlags> > results;
  
  /**
   * recursively do binary search
   */
  foldingRangesStartingOnLine (results, m_foldingRanges, line);
  
  /**
   * return found results
   */
  return results;
}

void TextFolding::foldingRangesStartingOnLine (QVector<QPair<qint64, FoldingRangeFlags> > &results, const TextFolding::FoldingRange::Vector &ranges, int line) const
{
  /**
   * early out for no folds
   */
  if (ranges.isEmpty())
    return;
  
  /**
   * first: lower bound of start
   */
  FoldingRange::Vector::const_iterator lowerBound = qLowerBound (ranges.begin(), ranges.end(), line, compareRangeByLineWithStart);
  
  /**
   * second: upper bound of end
   */
  FoldingRange::Vector::const_iterator upperBound = qUpperBound (ranges.begin(), ranges.end(), line, compareRangeByStartWithLine);

  /**
   * we may need to go one to the left, if not already at the begin, as we might overlap with the one in front of us!
   */
  if ((lowerBound != ranges.begin()) && ((*(lowerBound-1))->end->line() >= line))
    --lowerBound;
  
  /**
   * for all of them, check if we start at right line and recurse
   */
  for (FoldingRange::Vector::const_iterator it = lowerBound; it != upperBound; ++it) {
    /**
     * this range already ok? add it to results
     */
    if ((*it)->start->line() == line)
      results.push_back (qMakePair((*it)->id, (*it)->flags));
    
    /**
     * recurse anyway
     */
    foldingRangesStartingOnLine (results, (*it)->nestedRanges, line);
  }
}

QString TextFolding::debugDump () const
{
  /**
   * dump toplevel ranges recursively
   */
  return QString ("tree %1 - folded %2").arg (debugDump (m_foldingRanges, true)).arg(debugDump (m_foldedFoldingRanges, false));
}

void TextFolding::debugPrint (const QString &title) const
{
  // print title + content
  printf ("%s\n    %s\n", qPrintable (title), qPrintable(debugDump()));
}

QString TextFolding::debugDump (const TextFolding::FoldingRange::Vector &ranges, bool recurse)
{
  /**
   * dump all ranges recursively
   */
  QString dump;
  Q_FOREACH (FoldingRange *range, ranges) {
    if (!dump.isEmpty())
      dump += " ";
    
    dump += QString ("[%1:%2 %3%4 ").arg (range->start->line()).arg(range->start->column()).arg((range->flags & Persistent) ? "p" : "").arg((range->flags & Folded) ? "f" : "");
    
    /**
     * recurse
     */
    if (recurse) {
      QString inner = debugDump (range->nestedRanges, recurse);
      if (!inner.isEmpty())
        dump += inner + " ";
    }
    
    dump += QString ("%1:%2]").arg (range->end->line()).arg(range->end->column());
  }
  return dump;
}

bool TextFolding::insertNewFoldingRange (FoldingRange *parent, FoldingRange::Vector &existingRanges, FoldingRange *newRange)
{
  /**
   * existing ranges are non-overlapping and sorted
   * that means, we can search for lower bound of start of range and upper bound of end of range to find all "overlapping" ranges.
   */
  
  /**
   * first: lower bound of start
   */
  FoldingRange::Vector::iterator lowerBound = qLowerBound (existingRanges.begin(), existingRanges.end(), newRange, compareRangeByStart);
  
  /**
   * second: upper bound of end
   */
  FoldingRange::Vector::iterator upperBound = qUpperBound (existingRanges.begin(), existingRanges.end(), newRange, compareRangeByEnd);

  /**
   * we may need to go one to the left, if not already at the begin, as we might overlap with the one in front of us!
   */
  if ((lowerBound != existingRanges.begin()) && ((*(lowerBound-1))->end->toCursor() > newRange->start->toCursor()))
    --lowerBound;
  
  /**
   * now: first case, we overlap with nothing or hit exactly one range!
   */
  if (lowerBound == upperBound) {
    /**
     * nothing we overlap with?
     * then just insert and be done!
     */
    if ((lowerBound == existingRanges.end()) || (newRange->start->toCursor() >= (*lowerBound)->end->toCursor()) || (newRange->end->toCursor() <= (*lowerBound)->start->toCursor())) {
      /**
       * insert + fix parent
       */
      existingRanges.insert (lowerBound, newRange);
      newRange->parent = parent;
      
      /**
       * all done
       */
      return true;
    }
    
    /**
     * we are contained in this one range?
     * then recurse!
     */
    if ((newRange->start->toCursor() >= (*lowerBound)->start->toCursor()) && (newRange->end->toCursor() <= (*lowerBound)->end->toCursor()))
      return insertNewFoldingRange ((*lowerBound), (*lowerBound)->nestedRanges, newRange);
    
    /**
     * else: we might contain at least this fold, or many more, if this if block is not taken at all
     * use the general code that checks for "we contain stuff" below!
     */
  }
  
  /**
   * check if we contain other folds!
   */
  FoldingRange::Vector::iterator it = lowerBound;
  bool includeUpperBound = false;
  FoldingRange::Vector nestedRanges;
  while (it != existingRanges.end()) {
    /**
     * do we need to take look at upper bound, too?
     * if not break
     */
    if (it == upperBound) {
      if (newRange->end->toCursor() <= (*upperBound)->start->toCursor())
        break;
      else
        includeUpperBound = true;
    }
    
    /**
     * if one region is not contained in the new one, abort!
     * then this is not well nested!
     */
    if (!((newRange->start->toCursor() <= (*it)->start->toCursor()) && (newRange->end->toCursor() >= (*it)->end->toCursor())))
      return false;
    
    /**
     * include into new nested ranges
     */
    nestedRanges.push_back ((*it));
      
    /**
     * end reached
     */
    if (it == upperBound)
      break;
    
    /**
     * else increment
     */
    ++it;
  }
  
  /**
   * if we arrive here, all is nicely nested into our new range
   * remove the contained ones here, insert new range with new nested ranges we already constructed
   */
  it = existingRanges.erase (lowerBound, includeUpperBound ? (upperBound+1) : upperBound);
  existingRanges.insert (it, newRange);
  newRange->nestedRanges = nestedRanges;
  
  /**
   * correct parent mapping!
   */
  newRange->parent = parent;
  Q_FOREACH (FoldingRange *range, newRange->nestedRanges)
    range->parent = newRange;
  
  /**
   * all nice
   */
  return true;
}

bool TextFolding::compareRangeByStart (FoldingRange *a, FoldingRange *b)
{
  return a->start->toCursor() < b->start->toCursor();
}

bool TextFolding::compareRangeByEnd (FoldingRange *a, FoldingRange *b)
{
  return a->end->toCursor() < b->end->toCursor();
}

bool TextFolding::compareRangeByStartWithLine (int line, FoldingRange *range)
{
  return (line < range->start->line());
}

bool TextFolding::compareRangeByLineWithStart (FoldingRange *range, int line)
{
  return (range->start->line() < line);
}

bool TextFolding::updateFoldedRangesForNewRange (TextFolding::FoldingRange *newRange)
{
  /**
   * not folded? not interesting! we don't need to touch our m_foldedFoldingRanges vector
   */
  if (!(newRange->flags & Folded))
    return false;
  
  /**
   * any of the parents folded? not interesting, too!
   */
  TextFolding::FoldingRange *parent = newRange->parent;
  while (parent) {
    /**
     * parent folded => be done
     */
    if (parent->flags & Folded)
      return false;
    
    /**
     * walk up
     */
    parent = parent->parent;
  }
  
  /**
   * ok, if we arrive here, we are a folded range and we have no folded parent
   * we now want to add this range to the m_foldedFoldingRanges vector, just removing any ranges that is included in it!
   * TODO: OPTIMIZE
   */
  FoldingRange::Vector newFoldedFoldingRanges;
  bool newRangeInserted = false;
  Q_FOREACH (FoldingRange *range, m_foldedFoldingRanges) {
    /**
     * contained? kill
     */
    if ((newRange->start->toCursor() <= range->start->toCursor()) && (newRange->end->toCursor() >= range->end->toCursor()))
      continue;
    
    /**
     * range is behind newRange?
     * insert newRange if not already done
     */
    if (!newRangeInserted && (range->start->toCursor() >= newRange->end->toCursor())) {
      newFoldedFoldingRanges.push_back (newRange);
      newRangeInserted = true;
    }
    
    /**
     * just transfer range
     */
    newFoldedFoldingRanges.push_back (range);
  }
  
  /**
   * last: insert new range, if not done
   */
  if (!newRangeInserted)
    newFoldedFoldingRanges.push_back (newRange);
  
  /**
   * fixup folded ranges
   */
  m_foldedFoldingRanges = newFoldedFoldingRanges;
  
  /**
   * folding changed!
   */
  emit foldingRangesChanged ();
  
  /**
   * all fine, stuff done, signal emitted
   */
  return true;
}

bool TextFolding::updateFoldedRangesForRemovedRange (TextFolding::FoldingRange *oldRange)
{
  /**
   * folded? not interesting! we don't need to touch our m_foldedFoldingRanges vector
   */
  if (oldRange->flags & Folded)
    return false;
  
  /**
   * any of the parents folded? not interesting, too!
   */
  TextFolding::FoldingRange *parent = oldRange->parent;
  while (parent) {
    /**
     * parent folded => be done
     */
    if (parent->flags & Folded)
      return false;
    
    /**
     * walk up
     */
    parent = parent->parent;
  }
  
    /**
   * ok, if we arrive here, we are a unfolded range and we have no folded parent
   * we now want to remove this range from the m_foldedFoldingRanges vector and include our nested folded ranges!
   * TODO: OPTIMIZE
   */
  FoldingRange::Vector newFoldedFoldingRanges;
  Q_FOREACH (FoldingRange *range, m_foldedFoldingRanges) {
    /**
     * right range? insert folded nested ranges
     */
    if (range == oldRange) {
      appendFoldedRanges (newFoldedFoldingRanges, oldRange->nestedRanges);
      continue;
    }
    
    /**
     * just transfer range
     */
    newFoldedFoldingRanges.push_back (range);
  }
  
  /**
   * fixup folded ranges
   */
  m_foldedFoldingRanges = newFoldedFoldingRanges;
  
  /**
   * folding changed!
   */
  emit foldingRangesChanged ();
  
  /**
   * all fine, stuff done, signal emitted
   */
  return true;
}

void TextFolding::appendFoldedRanges (TextFolding::FoldingRange::Vector &newFoldedFoldingRanges, const TextFolding::FoldingRange::Vector &ranges) const
{
  /**
   * search for folded ranges and append them
   */
  Q_FOREACH (FoldingRange *range, ranges) {
    /**
     * itself folded? append
     */
    if (range->flags & Folded) {
      newFoldedFoldingRanges.push_back (range);
      continue;
    }
    
    /**
     * else: recurse!
     */
    appendFoldedRanges (newFoldedFoldingRanges, range->nestedRanges);
  }
}

QVariantList TextFolding::exportFoldingRanges () const
{
  QVariantList folds;
  exportFoldingRanges (m_foldingRanges, folds);
  return folds;
}

void TextFolding::exportFoldingRanges (const TextFolding::FoldingRange::Vector &ranges, QVariantList &folds)
{
  /**
   * dump all ranges recursively
   */
  Q_FOREACH (FoldingRange *range, ranges) {
    /**
     * construct one range and dump to folds
     */
    QVariantMap rangeMap;
    rangeMap["startLine"] = range->start->line();
    rangeMap["startColumn"] = range->start->column();
    rangeMap["endLine"] = range->end->line();
    rangeMap["endColumn"] = range->end->column();
    rangeMap["flags"] = (int)range->flags;
    folds.append (rangeMap);
    
    /**
     * recurse
     */
    exportFoldingRanges (range->nestedRanges, folds);
  }
}

void TextFolding::importFoldingRanges (const QVariantList &folds)
{
  /**
   * try to create all folding ranges
   */
  Q_FOREACH (QVariant rangeVariant, folds) {
    /**
     * get map
     */
    QVariantMap rangeMap = rangeVariant.toMap ();
    
    /**
     * construct range start/end
     */
    KTextEditor::Cursor start (rangeMap["startLine"].toInt(), rangeMap["startColumn"].toInt());
    KTextEditor::Cursor end (rangeMap["endLine"].toInt(), rangeMap["endColumn"].toInt());
    
    /**
     * get flags
     */
    int rawFlags = rangeMap["flags"].toInt();
    FoldingRangeFlags flags;
    if (rawFlags & Persistent)
      flags = Persistent;
    if (rawFlags & Folded)
      flags = Folded;
    
    /**
     * create folding range
     */
    newFoldingRange (KTextEditor::Range (start, end), flags);
  }
}

}
