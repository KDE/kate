/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

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

#include "katesuperrange.h"

#include "katedocument.h"
#include "kateview.h"
#include <ktexteditor/attribute.h>
#include "katerangetype.h"
#include "katesmartmanager.h"

#include <kdebug.h>

KateSmartRange::KateSmartRange(const KTextEditor::Range& range, KateDocument* doc, KTextEditor::SmartRange* parent, int insertBehaviour)
  : KTextEditor::SmartRange(new KateSmartCursor(range.start(), doc), new KateSmartCursor(range.end(), doc), parent, insertBehaviour)
  , m_notifier(0L)
  , m_watcher(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
//  , m_feedbackLevel(NoFeedback)
  , m_mouseOver(false)
  , m_caretOver(false)
{
}

KateSmartRange::KateSmartRange(KateDocument* doc, KTextEditor::SmartRange* parent)
  : KTextEditor::SmartRange(new KateSmartCursor(doc), new KateSmartCursor(doc), parent)
  , m_notifier(0L)
  , m_watcher(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
//  , m_feedbackLevel(NoFeedback)
  , m_mouseOver(false)
  , m_caretOver(false)
{
}

KateSmartRange::KateSmartRange( KateSmartCursor * start, KateSmartCursor * end, KTextEditor::SmartRange * parent, int insertBehaviour )
  : KTextEditor::SmartRange(start, end, parent, insertBehaviour)
  , m_notifier(0L)
  , m_watcher(0L)
  , m_attachedView(0L)
  , m_attachActions(TagLines)
//  , m_feedbackLevel(NoFeedback)
  , m_mouseOver(false)
  , m_caretOver(false)
{
}

KateSmartRange::~KateSmartRange()
{
}

void KateSmartRange::attachToView(KateView* view, int actions)
{
  m_attachedView = view;
  m_attachActions = actions;
}

bool KateSmartRange::isValid() const
{
  return start() <= end();
}

/*void KateSmartRange::slotEvaluateChanged()
{
  if (sender() == dynamic_cast<QObject*>(m_start)) {
    if (m_evaluate) {
      if (!m_endChanged) {
        // Only one was changed
        evaluateEliminated();

      } else {
        // Both were changed
        evaluatePositionChanged();
        m_endChanged = false;
      }

    } else {
      m_startChanged = true;
    }

  } else {
    if (m_evaluate) {
      if (!m_startChanged) {
        // Only one was changed
        evaluateEliminated();

      } else {
        // Both were changed
        evaluatePositionChanged();
        m_startChanged = false;
      }

    } else {
      m_endChanged = true;
    }
  }

  m_evaluate = !m_evaluate;
}

void KateSmartRange::slotEvaluateUnChanged()
{
  if (sender() == dynamic_cast<QObject*>(m_start)) {
    if (m_evaluate) {
      if (m_endChanged) {
        // Only one changed
        evaluateEliminated();
        m_endChanged = false;

      } else {
        // Neither changed
        emit positionUnChanged();
      }
    }

  } else {
    if (m_evaluate) {
      if (m_startChanged) {
        // Only one changed
        evaluateEliminated();
        m_startChanged = false;

      } else {
        // Neither changed
        emit positionUnChanged();
      }
    }
  }

  m_evaluate = !m_evaluate;
}

void KateSmartRange::slotTagRange()
{
  if (m_attachActions & TagLines)
    if (m_attachedView)
      m_attachedView->tagRange(*this);
    else
      doc()->tagLines(start(), end());

  if (m_attachActions & Redraw)
    if (m_attachedView)
      m_attachedView->repaintText(true);
    else
      doc()->repaintViews(true);
  //else
    //FIXME this method doesn't exist??
    //doc()->updateViews();

  emit tagRange(this);
}

void KateSmartRange::evaluateEliminated()
{
  if (start() == end()) {
    if (!m_allowZeroLength) emit eliminated();
  }
  else
    emit contentsChanged();
}

void KateSmartRange::evaluatePositionChanged()
{
  if (start() == end())
    emit eliminated();
  else
    emit positionChanged();
}*/

void KateSmartRange::slotMousePositionChanged(const KTextEditor::Cursor& newPosition)
{
  bool includesMouse = contains(newPosition);
  if (includesMouse != m_mouseOver) {
    m_mouseOver = includesMouse;

    //slotTagRange();
  }
}

void KateSmartRange::slotCaretPositionChanged(const KTextEditor::Cursor& newPosition)
{
  bool includesCaret = contains(newPosition);
  if (includesCaret != m_caretOver) {
    m_caretOver = includesCaret;

    //slotTagRange();
  }
}

void KateSmartRange::checkFeedback( )
{
  // FIXME don't set max feedback level if the feedback objects don't need it.
  /*int feedbackNeeded = (m_watcher || m_notifier) ? PositionChanged : NoFeedback;
  if (m_feedbackLevel != feedbackNeeded) {
    setFeedbackLevel(feedbackNeeded);
  }*/
}

KateDocument * KateSmartRange::kateDocument( ) const
{
  return static_cast<KateDocument*>(document());
}

/*void KateSmartRange::setFeedbackLevel( int feedbackLevel, bool request )
{
  int oldFeedbackLevel = m_feedbackLevel;
  m_feedbackLevel = feedbackLevel;
  if (request)
    kateDocument()->smartManager()->requestFeedback(this, oldFeedbackLevel);
}*/

/*KTextEditor::Attribute * KateSmartRange::attribute( ) const
{
  if (owningList() && owningList()->rangeType())
    if (KTextEditor::Attribute* a = owningList()->rangeType()->activatedAttribute(m_mouseOver, m_caretOver))
      return a;

  return 0L;
}*/

KateSmartRangeNotifier::KateSmartRangeNotifier(KateSmartRange* owner)
  : m_owner(owner)
{
}

void KateSmartRangeNotifier::connectNotify(const char* signal)
{
  if (receivers(signal) == 1)
    // which signal has been turned on?
    if (signal == SIGNAL(positionChanged(SmartRange*)))
      m_owner->checkFeedback();
}

void KateSmartRangeNotifier::disconnectNotify(const char* signal)
{
  if (receivers(signal) == 0)
    // which signal has been turned off?
    if (signal == SIGNAL(positionChanged(SmartRange*)))
      m_owner->checkFeedback();
}

KTextEditor::SmartRangeNotifier* KateSmartRange::notifier()
{
  if (!m_notifier) {
    m_notifier = new KateSmartRangeNotifier(this);
    checkFeedback();
  }
  return m_notifier;
}

void KateSmartRange::deleteNotifier()
{
  delete m_notifier;
  m_notifier = 0L;
  checkFeedback();
}

void KateSmartRange::setWatcher(KTextEditor::SmartRangeWatcher* watcher)
{
  m_watcher = watcher;
  checkFeedback();
}

void KateSmartRange::translated(const KateEditInfo& edit)
{
  if (end() < edit.start()) {
    kStart().resetLastPosition();
    kEnd().resetLastPosition();
    return;
  }

  if (kStart().lastPosition() != kStart()) {
    // position changed
    if (m_notifier)
      emit m_notifier->positionChanged(this);
    if (m_watcher)
      m_watcher->positionChanged(this);
  }

  if (kStart().lastPosition() <= edit.oldRange().end()) {
    // contents changed
    if (m_notifier)
      emit m_notifier->contentsChanged(this);
    if (m_watcher)
      m_watcher->contentsChanged(this);
  }

  if (start() == end() && kStart().lastPosition() != kEnd().lastPosition()) {
    // range eliminated
    if (m_notifier)
      emit m_notifier->eliminated(this);
    if (m_watcher)
      m_watcher->eliminated(this);
  }

  kStart().resetLastPosition();
  kEnd().resetLastPosition();
}

void KateSmartRange::shifted( )
{
  if (kStart().lastPosition() != kStart()) {
    // position changed
    if (m_notifier)
      emit m_notifier->positionChanged(this);
    if (m_watcher)
      m_watcher->positionChanged(this);
  }

  kStart().resetLastPosition();
  kEnd().resetLastPosition();
}

void KateSmartRange::setParentRange( SmartRange * r )
{
  bool gotParent = false;
  bool lostParent = false;
  if (!parentRange() && r)
    gotParent = true;
  else if (parentRange() && !r)
    lostParent = true;

  SmartRange::setParentRange(r);

  if (gotParent)
    kateDocument()->smartManager()->rangeGotParent(this);
  else if (lostParent)
    kateDocument()->smartManager()->rangeLostParent(this);
}

// kate: space-indent on; indent-width 2; replace-tabs on;

#include "katesuperrange.moc"
