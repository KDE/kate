/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#include "katelayoutcache.h"

#include "katerenderer.h"
#include "kateview.h"

#include <kdebug.h>

static bool enableLayoutCache = false;

KateLayoutCache::KateLayoutCache(KateRenderer* renderer)
  : m_renderer(renderer)
  , m_startPos(-1,-1)
  , m_viewWidth(0)
{
  Q_ASSERT(m_renderer);
}

void KateLayoutCache::updateViewCache(const KTextEditor::Cursor& startPos, int newViewLineCount, int viewLinesScrolled)
{
  //kdDebug() << k_funcinfo << startPos << " nvlc " << newViewLineCount << " vls " << viewLinesScrolled << endl;

  int oldViewLineCount = m_textLayouts.count();
  if (newViewLineCount == -1)
    newViewLineCount = oldViewLineCount;

  enableLayoutCache = true;

  m_startPos = startPos;
  int realLine = m_renderer->doc()->getRealLine(m_startPos.line());
  int _viewLine = 0;

  // TODO check these assumptions are ok... probably they don't give much speedup anyway?
  if (startPos == m_startPos && m_textLayouts.count()) {
    _viewLine = m_textLayouts.first().viewLine();

  } else if (viewLinesScrolled > 0 && m_textLayouts.count() > viewLinesScrolled) {
    _viewLine = m_textLayouts[viewLinesScrolled].viewLine();

  } else {
    KateLineLayoutPtr l = line(realLine);
    if (l) {
      Q_ASSERT(l->isValid());
      Q_ASSERT(l->length() >= m_startPos.column() || m_renderer->view()->wrapCursor());

      for (; _viewLine < l->viewLineCount(); ++_viewLine) {
        const KateTextLayout& t = l->viewLine(_viewLine);
        if (t.startCol() >= m_startPos.column() || _viewLine == l->viewLineCount() - 1)
          goto foundViewLine;
      }

      // FIXME FIXME need to calculate past-end-of-line position here...
      Q_ASSERT(false);

      foundViewLine:
      Q_ASSERT(true);
    }
  }

  // Move the text layouts if we've just scrolled...
  if (viewLinesScrolled != 0) {
    // loop backwards if we've just scrolled up...
    bool forwards = viewLinesScrolled >= 0 ? true : false;
    for (int z = forwards ? 0 : m_textLayouts.count() - 1; forwards ? (z < m_textLayouts.count()) : (z >= 0); forwards ? z++ : z--) {
      int oldZ = z + viewLinesScrolled;
      if (oldZ >= 0 && oldZ < m_textLayouts.count()) {
        m_textLayouts[z] = m_textLayouts[oldZ];

      } /*else {
        if (forwards) {
          if (m_textLayouts[z-1].wrap()) {
            m_textLayouts.append(m_textLayouts[z-1].kateLineLayout()->viewLine(m_textLayouts[z-1].viewLine() + 1));
          } else {
            KateLineLayoutPtr l = line(m_textLayouts[z-1].line() + 1);
            if (!l)
              m_textLayouts[z] = KateTextLayout::invalid();
            else
              m_textLayouts[z] = l->viewLine(0);
          }
        } else {
          if (m_textLayouts[z+1].viewLine()) {
            m_textLayouts[z] = m_textLayouts[z+1].kateLineLayout()->viewLine(m_textLayouts[z+1].viewLine() - 1);
          } else {
            KateLineLayoutPtr l = line(m_textLayouts[z+1].line() - 1);
            if (!l)
              m_textLayouts[z] = KateTextLayout::invalid();
            else
              m_textLayouts[z] = l->viewLine(-1);
          }
        }
      }*/
    }
  }

  // Resize functionality
  if (newViewLineCount > oldViewLineCount) {
    m_textLayouts.reserve(newViewLineCount);

  } else if (newViewLineCount < oldViewLineCount) {
    /* FIXME reintroduce... check we're not missing any
    int lastLine = m_textLayouts[newSize - 1].line();
    for (int i = oldSize; i < newSize; i++) {
      const KateTextLayout& layout = m_textLayouts[i];
      if (layout.line() > lastLine && !layout.viewLine())
        layout.kateLineLayout()->layout()->setCacheEnabled(false);
    }*/
    m_textLayouts.resize(newViewLineCount);
  }

  KateLineLayoutPtr l = line(realLine);
  for (int i = 0; i < newViewLineCount; ++i) {
    if (!l) {
      if (i < m_textLayouts.count())
        m_textLayouts[i] = KateTextLayout::invalid();
      else
        m_textLayouts.append(KateTextLayout::invalid());
      continue;
    }

    Q_ASSERT(l->isValid());
    Q_ASSERT(_viewLine < l->viewLineCount());

    if (i < m_textLayouts.count()) {
      if (m_textLayouts[i].line() != realLine || m_textLayouts[i].viewLine() != _viewLine || !m_textLayouts[i].isValid())
        m_textLayouts[i] = l->viewLine(_viewLine);

    } else {
      m_textLayouts.append(l->viewLine(_viewLine));
    }

    //kdDebug() << k_funcinfo << "Laid out line " << realLine << " (" << l << "), viewLine " << _viewLine << " (" << m_textLayouts[i].kateLineLayout().data() << ")" << endl;
    //m_textLayouts[i].debugOutput();

    _viewLine++;

    if (_viewLine > l->viewLineCount() - 1) {
      int virtualLine = l->virtualLine() + 1;
      realLine = m_renderer->doc()->getRealLine(virtualLine);
      _viewLine = 0;
      l = line(realLine, virtualLine);
    }
  }

  enableLayoutCache = false;
}

/*void KateLayoutCache::scrollViewCache( int viewLinesScrolled )
{
  // Move the text layouts if we've just scrolled...
  if (viewLinesScrolled == 0)
    return;

  enableLayoutCache = true;
  // loop backwards if we've just scrolled up...
  bool forwards = viewLinesScrolled >= 0 ? true : false;
  for (int z = forwards ? 0 : m_textLayouts.count() - 1; forwards ? (z < m_textLayouts.count()) : (z >= 0); forwards ? z++ : z--) {
    int oldZ = z + viewLinesScrolled;
    if (oldZ >= 0 && oldZ < m_textLayouts.count()) {
      m_textLayouts[z] = m_textLayouts[oldZ];

    } else {
      if (forwards) {
        if (m_textLayouts[z-1].wrap()) {
          m_textLayouts.append(m_textLayouts[z-1].kateLineLayout()->viewLine(m_textLayouts[z-1].viewLine() + 1));
        } else {
          KateLineLayoutPtr l = line(m_textLayouts[z-1].line() + 1);
          if (!l)
            m_textLayouts[z] = KateTextLayout::invalid();
          else
            m_textLayouts[z] = l->viewLine(0);
        }
      } else {
        if (m_textLayouts[z+1].viewLine()) {
          m_textLayouts[z] = m_textLayouts[z+1].kateLineLayout()->viewLine(m_textLayouts[z+1].viewLine() - 1);
        } else {
          KateLineLayoutPtr l = line(m_textLayouts[z+1].line() - 1);
          if (!l)
            m_textLayouts[z] = KateTextLayout::invalid();
          else
            m_textLayouts[z] = l->viewLine(-1);
        }
      }
    }
  }
  enableLayoutCache = false;

  m_startPos = m_textLayouts.first().start();
}

void KateLayoutCache::setViewCacheStartPos( const KTextEditor::Cursor & newStartPos )
{
  if (newStartPos == m_startPos)
    return;

  m_startPos = newStartPos;

  int _line = newStartPos.line();
  int _viewLine = 0;
  KateLineLayoutPtr l = line(_line);
  for (int i = 0; i < viewCacheLineCount(); ++i) {
    if (!l) {
      m_textLayouts[i] = KateTextLayout::invalid();
      continue;
    }

    Q_ASSERT(l->isValid());

    if (!i) {
      Q_ASSERT(l->length() >= m_startPos.column());

      for (; _viewLine < l->viewLineCount(); ++_viewLine) {
        const KateTextLayout& t = l->viewLine(_viewLine);
        if (t.startCol() >= m_startPos.column() || _viewLine == l->viewLineCount() - 1) {
          m_textLayouts[i] = t;
          break;
        }
      }
    } else {
      Q_ASSERT(_viewLine < l->viewLineCount());
      m_textLayouts[i] = l->viewLine(_viewLine);
    }

    //kdDebug() << k_funcinfo << "Laid out line " << _line << " (" << l << "), viewLine " << _viewLine << " (" << m_textLayouts[i].kateLineLayout().data() << ")" << endl;
    //m_textLayouts[i].debugOutput();

    _viewLine++;

    if (_viewLine > l->viewLineCount() - 1) {
      _line++;
      _viewLine = 0;
      l = line(_line);
    }
  }

  //kdDebug() << k_funcinfo << "Laid out lines from pos " << m_startPos.line() << "," << m_startPos.column() << " to " << _line << ", viewLine " << _viewLine << endl;
}*/

KateLineLayoutPtr KateLayoutCache::line( int realLine, int virtualLine ) const
{
  if (m_lineLayouts.contains(realLine)) {
    KateLineLayoutPtr l = m_lineLayouts[realLine];
    if (virtualLine != -1)
      l->setVirtualLine(virtualLine);
    if (!l->isValid())
      m_renderer->layoutLine(l, m_viewWidth, enableLayoutCache);
    Q_ASSERT(l->isValid());
    return l;
  }

  if (realLine < 0 || realLine >= m_renderer->doc()->lines())
    return 0L;

  KateLineLayoutPtr l = new KateLineLayout(m_renderer->doc());
  l->setLine(realLine, virtualLine);
  m_renderer->layoutLine(l, m_viewWidth, enableLayoutCache);
  Q_ASSERT(l->isValid());

  m_lineLayouts.insert(realLine, l);
  return l;
}

KateLineLayoutPtr KateLayoutCache::line( const KTextEditor::Cursor & realCursor ) const
{
  return line(realCursor.line());
}

KateTextLayout KateLayoutCache::textLayout( const KTextEditor::Cursor & realCursor ) const
{
  /*if (realCursor >= viewCacheStart() && (realCursor < viewCacheEnd() || realCursor == viewCacheEnd() && !m_textLayouts.last().wrap()))
    foreach (const KateTextLayout& l, m_textLayouts)
      if (l.line() == realCursor.line() && (l.endCol() < realCursor.column() || !l.wrap()))
        return l;*/

  return line(realCursor.line())->viewLine(viewLine(realCursor));
}

KateTextLayout KateLayoutCache::textLayout( uint realLine, int _viewLine ) const
{
  /*if (m_textLayouts.count() && (realLine >= m_textLayouts.first().line() && _viewLine >= m_textLayouts.first().viewLine()) &&
      (realLine <= m_textLayouts.last().line() && _viewLine <= m_textLayouts.first().viewLine()))
    foreach (const KateTextLayout& l, m_textLayouts)
      if (l.line() == realLine && l.viewLine() == _viewLine)
        return const_cast<KateTextLayout&>(l);*/

  return line(realLine)->viewLine(_viewLine);
}

KateTextLayout & KateLayoutCache::viewLine( int _viewLine ) const
{
  Q_ASSERT(_viewLine >= 0 && _viewLine < m_textLayouts.count());
  return m_textLayouts[_viewLine];
}

int KateLayoutCache::viewCacheLineCount( ) const
{
  return m_textLayouts.count();
}

KTextEditor::Cursor KateLayoutCache::viewCacheStart( ) const
{
  return m_textLayouts.count() ? m_textLayouts.first().start() : KTextEditor::Cursor();
}

KTextEditor::Cursor KateLayoutCache::viewCacheEnd( ) const
{
  return m_textLayouts.count() ? m_textLayouts.last().end() : KTextEditor::Cursor();
}

int KateLayoutCache::viewWidth( ) const
{
  return m_viewWidth;
}

/**
 * This returns the view line upon which realCursor is situated.
 * The view line is the number of lines in the view from the first line
 * The supplied cursor should be in real lines.
 */
int KateLayoutCache::viewLine(const KTextEditor::Cursor& realCursor) const
{
  if (realCursor.column() == 0) return 0;

  KateLineLayoutPtr thisLine = line(realCursor.line());

  for (int i = 0; i < thisLine->viewLineCount(); ++i) {
    const KateTextLayout& l = thisLine->viewLine(i);
    if (realCursor.column() >= l.startCol() && realCursor.column() < l.endCol())
      return i;
  }

  return thisLine->viewLineCount() - 1;
}

int KateLayoutCache::displayViewLine(const KTextEditor::Cursor& virtualCursor, bool limitToVisible) const
{
  KTextEditor::Cursor work = viewCacheStart();

  int limit = m_textLayouts.count();

  // Efficient non-word-wrapped path
  if (!m_renderer->view()->dynWordWrap()) {
    int ret = virtualCursor.line() - work.line();
    if (limitToVisible && (ret < 0 || ret > limit))
      return -1;
    else
      return ret;
  }

  if (work == virtualCursor) {
    return 0;
  }

  int ret = -(int)viewLine(work);
  bool forwards = (work < virtualCursor) ? true : false;

  // FIXME switch to using ranges? faster?
  if (forwards) {
    while (work.line() != virtualCursor.line()) {
      ret += viewLineCount(m_renderer->doc()->getRealLine(work.line()));
      work.setLine(work.line() + 1);
      if (limitToVisible && ret > limit)
        return -1;
    }
  } else {
    while (work.line() != virtualCursor.line()) {
      work.setLine(work.line() - 1);
      ret -= viewLineCount(m_renderer->doc()->getRealLine(work.line()));
      if (limitToVisible && ret < 0)
        return -1;
    }
  }

  // final difference
  KTextEditor::Cursor realCursor = virtualCursor;
  realCursor.setLine(m_renderer->doc()->getRealLine(realCursor.line()));
  if (realCursor.column() == -1) realCursor.setColumn(m_renderer->doc()->lineLength(realCursor.line()));
  ret += viewLine(realCursor);

  if (limitToVisible && (ret < 0 || ret > limit))
    return -1;

  return ret;
}

int KateLayoutCache::lastViewLine(int realLine) const
{
  if (!m_renderer->view()->dynWordWrap()) return 0;

  KateLineLayoutPtr l = line(realLine);
  Q_ASSERT(l);
  return l->viewLineCount() - 1;
}

int KateLayoutCache::viewLineCount(int realLine) const
{
  return lastViewLine(realLine) + 1;
}

void KateLayoutCache::viewCacheDebugOutput( ) const
{
  kdDebug() << k_funcinfo << "Printing values for " << m_textLayouts.count() << " lines:" << endl;
  if (m_textLayouts.count())
  foreach (const KateTextLayout& t, m_textLayouts)
    if (t.isValid())
      t.debugOutput();
    else
      kdDebug() << "Line Invalid." << endl;
}

void KateLayoutCache::slotTextInserted( KTextEditor::Document *, const KTextEditor::Range & range )
{
  // OPTIMISE: check to see if it's worth the effort

  int deleteFromLine = range.start().line(); // eg 2,0 = 2 and 2,3 = 2
  int shiftFromLine = range.end().line() + (range.end().column() ? 1 : 0); // eg 3,6 = 4 and 3,0 = 3
  int shiftAmount = shiftFromLine - deleteFromLine; // eg 4 - 2 = 2 and 3 - 2 = 1

  updateCache(deleteFromLine, shiftFromLine, shiftAmount);
}

void KateLayoutCache::slotTextRemoved( KTextEditor::Document *, const KTextEditor::Range & range )
{
  // OPTIMISE: check to see if it's worth the effort

  int deleteFromLine = range.start().line(); // eg 2,0 = 2 and 2,3 = 2
  int shiftFromLine = range.end().line() + (range.end().column() ? 1 : 0); // eg 3,6 = 4 and 3,0 = 3
  int shiftAmount = deleteFromLine - shiftFromLine; // eg 2 - 4 = -2 and 2 - 3 = -1

  updateCache(deleteFromLine, shiftFromLine, shiftAmount);
}

void KateLayoutCache::slotTextChanged(KTextEditor::Document *, const KTextEditor::Range& oldRange, const KTextEditor::Range& newRange)
{
  // OPTIMISE: check to see if it's worth the effort

  int deleteFromLine = oldRange.start().line(); // eg 2,0 = 2 and 2,3 = 2
  int shiftFromLine = oldRange.end().line() + (oldRange.end().column() ? 1 : 0); // eg 3,6 = 4 and 3,0 = 3
  int shiftToLine = newRange.end().line() + (newRange.end().column() ? 1 : 0); // eg 5,6 = 6 and 5,0 = 5
  int shiftAmount = shiftToLine - shiftFromLine; // eg 6 - 4 = 2 and 5 - 4 = 1

  updateCache(deleteFromLine, shiftFromLine, shiftAmount);
}

void KateLayoutCache::updateCache( int fromLine, int toLine, int shiftAmount )
{
  if (shiftAmount) {
    QMap<int, KateLineLayoutPtr> oldMap = m_lineLayouts;
    m_lineLayouts.clear();

    QMapIterator<int, KateLineLayoutPtr> it = oldMap;
    while (it.hasNext()) {
      it.next();

      if (it.key() >= toLine)
        m_lineLayouts.insert(it.key() + shiftAmount, it.value());
      else if (it.key() < fromLine)
        m_lineLayouts.insert(it.key(), it.value());
      else
        //gets deleted when oldMap goes but invalidate it just in case
        const_cast<KateLineLayout*>(it.value().data())->invalidateLayout();
    }

  } else {
    for (int i = fromLine; i < toLine; --i)
      if (m_lineLayouts.contains(i)) {
        const_cast<KateLineLayout*>(m_lineLayouts[i].data())->invalidateLayout();
        m_lineLayouts.remove(i);
      }
  }
}

void KateLayoutCache::clear( )
{
  m_textLayouts.clear();
  m_lineLayouts.clear();
  m_startPos = KTextEditor::Cursor(-1,-1);
}

void KateLayoutCache::setViewWidth( int width )
{
  bool wider = width > m_viewWidth;

  m_viewWidth = width;

  m_lineLayouts.clear();
  m_startPos = KTextEditor::Cursor(-1,-1);

  // Only get rid of layouts that we have to
  if (wider) {
    QMutableMapIterator<int, KateLineLayoutPtr> it = m_lineLayouts;
    while (it.hasNext()) {
      it.next();

      if (it.value()->viewLineCount() > 1)
        const_cast<KateLineLayout*>(it.value().data())->invalidateLayout();
    }

  } else {
    QMutableMapIterator<int, KateLineLayoutPtr> it = m_lineLayouts;
    while (it.hasNext()) {
      it.next();

      if (it.value()->viewLineCount() > 1 || it.value()->width() > m_viewWidth)
        const_cast<KateLineLayout*>(it.value().data())->invalidateLayout();
    }
  }
}
