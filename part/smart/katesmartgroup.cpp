/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2005 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#include "katesmartgroup.h"

#include "katedocument.h"
#include "kateedit.h"
#include "katesmartcursor.h"
#include "katesmartrange.h"

#include <kdebug.h>

//#define DEBUG_TRANSLATION

using namespace KTextEditor;

KateSmartGroup::KateSmartGroup( int startLine, int endLine, KateSmartGroup * previous, KateSmartGroup * next )
  : m_startLine(startLine)
  , m_newStartLine(startLine)
  , m_endLine(endLine)
  , m_newEndLine(endLine)
  , m_next(next)
  , m_previous(previous)
{
  if (m_previous)
    m_previous->setNext(this);

  if (m_next)
    m_next->setPrevious(this);
}

void KateSmartGroup::translateChanged( const KateEditInfo& edit)
{
  //kDebug() << "Was " << edit.oldRange() << " now " << edit.newRange() << " numcursors feedback " << m_feedbackCursors.count() << " normal " << m_normalCursors.count();

  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->translate(edit);

  foreach (KateSmartCursor* cursor, m_normalCursors)
    cursor->translate(edit);
}

void KateSmartGroup::translateShifted(const KateEditInfo& edit)
{
  m_newStartLine = m_startLine + edit.translate().line();
  m_newEndLine = m_endLine + edit.translate().line();
  //Since shifting only does not change the overlap or order, we don't need to rebuild any child structures here
}

void KateSmartGroup::translatedChanged(const KateEditInfo& edit)
{
  m_startLine = m_newStartLine;
  m_endLine = m_newEndLine;

  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->translated(edit);
}

void KateSmartGroup::translatedChanged2(const KateEditInfo& edit)
{
  Q_UNUSED(edit)
  //Tell the affected parent smart-ranges to rebuild their child-structure, so they stay consistent
  QSet<KTextEditor::SmartRange*> rebuilt;

  foreach (KateSmartCursor* cursor, m_normalCursors + m_feedbackCursors) {
    KTextEditor::SmartRange* range = cursor->smartRange();
    if(range) {
        KTextEditor::SmartRange* parent = range->parentRange();
        if(parent && !rebuilt.contains(parent)) {
            rebuilt.insert(parent);
            KateSmartRange* kateSmart = dynamic_cast<KateSmartRange*>(parent);
            Q_ASSERT(kateSmart);
            kateSmart->rebuildChildStructure();
        }
    }
  }
}

void KateSmartGroup::translatedShifted(const KateEditInfo& edit)
{
  if (m_startLine != m_newStartLine) {
    m_startLine = m_newStartLine;
    m_endLine = m_newEndLine;
  }

  if (edit.translate().line() == 0)
    return;

  // Todo: don't need to provide positionChanged to all feedback cursors?
  foreach (KateSmartCursor* cursor, m_feedbackCursors)
    cursor->shifted();
}

void KateSmartGroup::merge( )
{
  Q_ASSERT(m_next);

  foreach (KateSmartCursor* cursor, next()->feedbackCursors())
    cursor->migrate(this);
  m_feedbackCursors += next()->feedbackCursors();

  foreach (KateSmartCursor* cursor, next()->normalCursors())
    cursor->migrate(this);
  m_normalCursors += next()->normalCursors();

  m_newEndLine = m_endLine = next()->endLine();
  KateSmartGroup* newNext = next()->next();
  delete m_next;
  m_next = newNext;
  if (m_next)
    m_next->setPrevious(this);
}

const QSet< KateSmartCursor * > & KateSmartGroup::feedbackCursors( ) const
{
  return m_feedbackCursors;
}

const QSet< KateSmartCursor * > & KateSmartGroup::normalCursors( ) const
{
  return m_normalCursors;
}

void KateSmartGroup::addCursor( KateSmartCursor * cursor)
{
  Q_ASSERT(!m_feedbackCursors.contains(cursor));
  Q_ASSERT(!m_normalCursors.contains(cursor));

  if (cursor->feedbackEnabled())
    m_feedbackCursors.insert(cursor);
  else
    m_normalCursors.insert(cursor);
}

void KateSmartGroup::changeCursorFeedback( KateSmartCursor * cursor )
{
  if (!cursor->feedbackEnabled()) {
    Q_ASSERT(!m_feedbackCursors.contains(cursor));
    Q_ASSERT(m_normalCursors.contains(cursor));
    m_normalCursors.remove(cursor);
    m_feedbackCursors.insert(cursor);

  } else {
    Q_ASSERT(m_feedbackCursors.contains(cursor));
    Q_ASSERT(!m_normalCursors.contains(cursor));
    m_feedbackCursors.remove(cursor);
    m_normalCursors.insert(cursor);
  }
}

void KateSmartGroup::removeCursor( KateSmartCursor * cursor)
{
  if (cursor->feedbackEnabled()) {
    Q_ASSERT(m_feedbackCursors.contains(cursor));
    Q_ASSERT(!m_normalCursors.contains(cursor));
    m_feedbackCursors.remove(cursor);

  } else {
    Q_ASSERT(!m_feedbackCursors.contains(cursor));
    Q_ASSERT(m_normalCursors.contains(cursor));
    m_normalCursors.remove(cursor);
  }
}

void KateSmartGroup::joined( KateSmartCursor * cursor )
{
  addCursor(cursor);
}

void KateSmartGroup::leaving( KateSmartCursor * cursor )
{
  removeCursor(cursor);
}

void KateSmartGroup::debugOutput( ) const
{
  kDebug() << " -> KateSmartGroup: from " << startLine() << " to " << endLine() << "; Cursors " << m_normalCursors.count() + m_feedbackCursors.count() << " (" << m_feedbackCursors.count() << " feedback)";
}


void KateSmartGroup::deleteCursors( bool includingInternal )
{
  if (includingInternal) {
    qDeleteAll(m_feedbackCursors);
    m_feedbackCursors.clear();

    qDeleteAll(m_normalCursors);
    m_normalCursors.clear();

  } else {
    deleteCursorsInternal(m_feedbackCursors);
    deleteCursorsInternal(m_normalCursors);
  }
}

void KateSmartGroup::deleteCursorsInternal( QSet< KateSmartCursor * > & set )
{
  foreach (KateSmartCursor* c, set.toList()) {
    if (!c->range() && !c->isInternal()) {
      set.remove(c);
      delete c;
    }
  }
}

#include "katesmartgroup.moc"
