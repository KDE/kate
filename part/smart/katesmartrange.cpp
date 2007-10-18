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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesmartrange.h"

#include "katedocument.h"
#include "kateview.h"
#include <ktexteditor/attribute.h>
#include "katesmartmanager.h"
#include "katedynamicanimation.h"

#include <kdebug.h>

KateSmartRange::KateSmartRange(const KTextEditor::Range& range, KateDocument* doc, KTextEditor::SmartRange* parent, KTextEditor::SmartRange::InsertBehaviors insertBehavior)
  : KTextEditor::SmartRange(new KateSmartCursor(range.start(), doc), new KateSmartCursor(range.end(), doc), parent, insertBehavior)
//  , m_feedbackLevel(NoFeedback)
  , m_dynamicDoc(0L)
  , m_mouseOver(false)
  , m_caretOver(false)
  , m_isInternal(false)
{
}

KateSmartRange::KateSmartRange(KateDocument* doc, KTextEditor::SmartRange* parent)
  : KTextEditor::SmartRange(new KateSmartCursor(doc), new KateSmartCursor(doc), parent)
//  , m_feedbackLevel(NoFeedback)
  , m_dynamicDoc(0L)
  , m_mouseOver(false)
  , m_caretOver(false)
  , m_isInternal(false)
{
}

KateSmartRange::KateSmartRange( KateSmartCursor * start, KateSmartCursor * end, KTextEditor::SmartRange * parent, KTextEditor::SmartRange::InsertBehaviors insertBehavior )
  : KTextEditor::SmartRange(start, end, parent, insertBehavior)
//  , m_feedbackLevel(NoFeedback)
  , m_dynamicDoc(0L)
  , m_mouseOver(false)
  , m_caretOver(false)
  , m_isInternal(false)
{
}

KateSmartRange::~KateSmartRange()
{
  foreach (KTextEditor::SmartRangeNotifier* n, notifiers()) {
    emit static_cast<KateSmartRangeNotifier*>(n)->rangeDeleted(this);
    // FIXME delete the notifier
  }

  foreach (KTextEditor::SmartRangeWatcher* w, watchers())
    w->rangeDeleted(this);

  if (m_start)
    kateDocument()->smartManager()->rangeDeleted(this);

  foreach (KateSmartRangePtr* ptr, m_pointers)
    ptr->deleted();
}

KateDocument * KateSmartRange::kateDocument( ) const
{
  return static_cast<KateDocument*>(document());
}

KateSmartRangeNotifier::KateSmartRangeNotifier(KateSmartRange* owner)
  : m_owner(owner)
{
}

void KateSmartRangeNotifier::connectNotify(const char* signal)
{
  if (receivers(signal) == 1)
    // which signal has been turned on?
    if (QMetaObject::normalizedSignature(SIGNAL(positionChanged(SmartRange*))) == signal)
      m_owner->checkFeedback();
}

void KateSmartRangeNotifier::disconnectNotify(const char* signal)
{
  if (receivers(signal) == 0)
    // which signal has been turned off?
    if (QMetaObject::normalizedSignature(SIGNAL(positionChanged(SmartRange*))) == signal)
      m_owner->checkFeedback();
}

KTextEditor::SmartRangeNotifier* KateSmartRange::createNotifier()
{
  return new KateSmartRangeNotifier(this);
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
    foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
      emit static_cast<KateSmartRangeNotifier*>(n)->rangePositionChanged(this);
    foreach (KTextEditor::SmartRangeWatcher* w, watchers())
      w->rangePositionChanged(this);
  }

  if (kStart().lastPosition() <= edit.oldRange().end()) {
    // contents changed
    foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
      emit static_cast<KateSmartRangeNotifier*>(n)->rangeContentsChanged(this);
    foreach (KTextEditor::SmartRangeWatcher* w, watchers())
      w->rangeContentsChanged(this);

    /*if (kStart().lastPosition() >= edit.start() && kStart().lastPosition() < edit.oldRange().end()) {
      // first character deleted
      foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
        emit static_cast<KateSmartRangeNotifier*>(n)->firstCharacterDeleted(this);
      foreach (KTextEditor::SmartRangeWatcher* w, watchers())
        w->firstCharacterDeleted(this);
    }

    if (kEnd().lastPosition() >= edit.start() && kEnd().lastPosition() <= edit.oldRange().end()) {
      // last character deleted
      foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
        emit static_cast<KateSmartRangeNotifier*>(n)->lastCharacterDeleted(this);
      foreach (KTextEditor::SmartRangeWatcher* w, watchers())
        w->lastCharacterDeleted(this);
    }*/
  }

  if (start() == end() && kStart().lastPosition() != kEnd().lastPosition()) {
    // range eliminated
    foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
      emit static_cast<KateSmartRangeNotifier*>(n)->rangeEliminated(this);
    foreach (KTextEditor::SmartRangeWatcher* w, watchers())
      w->rangeEliminated(this);
  }

  kStart().resetLastPosition();
  kEnd().resetLastPosition();
}

void KateSmartRange::feedbackMostSpecific( KateSmartRange * mostSpecific )
{
  // most specific range feedback
  foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
    emit static_cast<KateSmartRangeNotifier*>(n)->rangeContentsChanged(this, mostSpecific);
  foreach (KTextEditor::SmartRangeWatcher* w, watchers())
    w->rangeContentsChanged(this, mostSpecific);
}

void KateSmartRange::feedbackMouseCaretChange(KTextEditor::View* view, bool mouse, bool entered)
{
  if (mouse) {
    if (entered) {
      foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
        emit static_cast<KateSmartRangeNotifier*>(n)->mouseEnteredRange(this, view);
      foreach (KTextEditor::SmartRangeWatcher* w, watchers())
        w->mouseEnteredRange(this, view);

    } else {
      foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
        emit static_cast<KateSmartRangeNotifier*>(n)->mouseExitedRange(this, view);
      foreach (KTextEditor::SmartRangeWatcher* w, watchers())
        w->mouseExitedRange(this, view);
    }

  } else {
    if (entered) {
      foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
        emit static_cast<KateSmartRangeNotifier*>(n)->caretEnteredRange(this, view);
      foreach (KTextEditor::SmartRangeWatcher* w, watchers())
        w->caretEnteredRange(this, view);

    } else {
      foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
        emit static_cast<KateSmartRangeNotifier*>(n)->caretExitedRange(this, view);
      foreach (KTextEditor::SmartRangeWatcher* w, watchers())
        w->caretExitedRange(this, view);
    }
  }
}

void KateSmartRange::shifted( )
{
  if (kStart().lastPosition() != kStart()) {
    // position changed
    foreach (KTextEditor::SmartRangeNotifier* n, notifiers())
      emit static_cast<KateSmartRangeNotifier*>(n)->rangePositionChanged(this);
    foreach (KTextEditor::SmartRangeWatcher* w, watchers())
      w->rangePositionChanged(this);
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

void KateSmartRange::unbindAndDelete( )
{
  kateDocument()->smartManager()->rangeDeleted(this);
  kStart().unbindFromRange();
  kEnd().unbindFromRange();
  m_start = 0L;
  m_end = 0L;
  delete this;
}

void KateSmartRange::setInternal( )
{
  m_isInternal = true;
  kStart().setInternal();
  kEnd().setInternal();
}

/*KateDynamicAnimation * KateSmartRange::dynamicForView( const KateView * const view) const
{
  foreach (KateDynamicAnimation* anim, m_dynamic)
    if (anim->view() == view)
      return anim;

  return 0L;
}*/

void KateSmartRange::addDynamic( KateDynamicAnimation * anim )
{
  m_dynamic.append(anim);
}

void KateSmartRange::removeDynamic( KateDynamicAnimation * anim )
{
  m_dynamic.removeAll(anim);
}

const QList<KateDynamicAnimation*> & KateSmartRange::dynamicAnimations( ) const
{
  return m_dynamic;
}

void KateSmartRange::registerPointer( KateSmartRangePtr * ptr )
{
  m_pointers.append(ptr);
}

void KateSmartRange::deregisterPointer( KateSmartRangePtr * ptr )
{
  m_pointers.removeAll(ptr);
}

// kate: space-indent on; indent-width 2; replace-tabs on;

#include "katesmartrange.moc"
