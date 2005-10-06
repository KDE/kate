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

#include "katerangetype.h"

#include "katedocument.h"
#include "katesmartrange.h"
#include <ktexteditor/attribute.h>

KateRangeType::KateRangeType()
  : m_activeAttribute(0L)
  , m_haveConnectedMouse(false)
  , m_haveConnectedCaret(false)
{
}

void KateRangeType::addAttribute( KTextEditor::Attribute * attribute, int activationFlags, bool activate )
{
  Q_ASSERT(!m_attributes.contains(attribute));
  Q_ASSERT(!(activateMouseIn & activationFlags && activateMouseOut & activationFlags));
  Q_ASSERT(!(activateCaretIn & activationFlags && activateCaretOut & activationFlags));

  m_attributes.insert(attribute, activationFlags);
  if (activate) {
    m_activeAttribute = attribute;
    tagAll();
  }

  if (!m_haveConnectedMouse && (activateMouseIn & activationFlags || activateMouseOut & activationFlags))
    activateMouse();

  if (!m_haveConnectedCaret && (activateCaretIn & activationFlags || activateCaretOut & activationFlags))
    activateCaret();
}

void KateRangeType::removeAttribute( KTextEditor::Attribute * attribute )
{
  Q_ASSERT(m_attributes.contains(attribute));
  m_attributes.remove(attribute);
  if (m_activeAttribute == attribute) {
    m_activeAttribute = 0L;
    tagAll();
  }
}

void KateRangeType::changeActivationFlags( KTextEditor::Attribute * attribute, int flags )
{
  Q_ASSERT(m_attributes.contains(attribute));
  m_attributes[attribute] = flags;

  bool needMouse, needCaret;
  for (QMap<KTextEditor::Attribute*, int>::ConstIterator it = m_attributes.constBegin(); it != m_attributes.constEnd(); ++it) {
    needMouse |= (activateMouseIn | activateMouseOut) & it.value();
    needCaret |= (activateCaretIn | activateCaretOut) & it.value();
  }

  if (needMouse != m_haveConnectedMouse)
    activateMouse(needMouse);

  if (needCaret != m_haveConnectedCaret)
    activateCaret(needCaret);
}

void KateRangeType::activateMouse( bool activate )
{
  if (m_haveConnectedMouse == activate)
    return;

  for (QList<KateSmartRange*>::ConstIterator it = m_ranges.begin(); it != m_ranges.end(); ++it)
    connectMouse(*it, activate);

  m_haveConnectedMouse = activate;
}

void KateRangeType::connectMouse( KateSmartRange * range, bool doconnect )
{
    /*KateDocument* doc = m_ranges.first()->kateDocument();

  if (doconnect)
    QObject::connect(doc, SIGNAL(activeViewMousePositionChanged(const KTextEditor::Cursor&)), range, SLOT(slotMousePositionChanged(const KTextEditor::Cursor&)));
  else
    QObject::disconnect(doc, SIGNAL(activeViewMousePositionChanged(const KTextEditor::Cursor&)), range, SLOT(slotMousePositionChanged(const KTextEditor::Cursor&)));*/
}

void KateRangeType::activateCaret( bool activate )
{
    /*if (m_haveConnectedCaret == activate)
    return;

  for (QList<KateSmartRange*>::ConstIterator it = m_ranges.begin(); it != m_ranges.end(); ++it)
    connectCaret(*it, activate);

  m_haveConnectedCaret = activate;*/
}

void KateRangeType::connectCaret( KateSmartRange * range, bool doconnect )
{
    /*KateDocument* doc = m_ranges.first()->doc();

  if (doconnect)
    QObject::connect(doc, SIGNAL(activeViewCaretPositionChanged(const KTextEditor::Cursor&)), range, SLOT(slotCaretPositionChanged(const KTextEditor::Cursor&)));
  else
    QObject::disconnect(doc, SIGNAL(activeViewCaretPositionChanged(const KTextEditor::Cursor&)), range, SLOT(slotCaretPositionChanged(const KTextEditor::Cursor&)));*/
}

void KateRangeType::tagAll( )
{
    /*for (QList<KateSmartRange*>::ConstIterator it = m_ranges.begin(); it != m_ranges.end(); ++it)
    (*it)->slotTagRange();*/
}

KTextEditor::Attribute * KateRangeType::activatedAttribute(bool mouseIn, bool caretIn) const
{
  int matchingFlag = (mouseIn ? activateMouseIn : activateMouseOut) | (caretIn ? activateCaretIn : activateCaretOut);

  static KTextEditor::Attribute* ret = new KTextEditor::Attribute();
  *ret = *m_activeAttribute;
  if (matchingFlag)
    for (QMap<KTextEditor::Attribute*, int>::ConstIterator it = m_attributes.constBegin(); it != m_attributes.constEnd(); ++it)
      if (it.value() & matchingFlag)
        *ret += *it.key();

  return ret;
}

void KateRangeType::addRange( KateSmartRange* range )
{
  m_ranges.append(range);
  if (m_haveConnectedMouse)
    connectMouse(range, true);
  if (m_haveConnectedCaret)
    connectCaret(range, true);
}

void KateRangeType::removeRange( KateSmartRange* range )
{
  m_ranges.removeAll(range);
  if (m_haveConnectedMouse)
    connectMouse(range, false);
  if (m_haveConnectedCaret)
    connectCaret(range, false);
}
