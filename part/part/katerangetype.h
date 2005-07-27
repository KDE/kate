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

#ifndef KATERANGETYPE_H
#define KATERANGETYPE_H

#include <QList>
#include <QMap>

class KateSuperRange;
class KAction;
class KateAttribute;

/**
 * Allows a group of KTextEditor::Ranges to be characterised and manipulated in a
 * convenient manner.
 */
class KateRangeType
{
  public:
    KateRangeType();

    void addRange(KateSuperRange* range);
    void removeRange(KateSuperRange* range);

    /**
     * Attach an action to all ranges of this type.  The action is enabled when
     * the range is entered by the cursor and disabled on exit.  The action is
     * also added to the context menu when the cursor is over this range.
     */
    void attachAction(KAction* action);
    void detachAction(KAction* action);

    /**
     * Several automatic activation mechanisms exist for associated attributes.
     * Using this you can conveniently have your ranges highlighted on mouse|cursor in|out.
     */
    enum activationFlags {
      activateNone        = 0x0,
      activateMouseIn   = 0x1,
      activateMouseOut    = 0x2,
      activateCaretIn   = 0x4,
      activateCaretOut    = 0x8
    };

    /**
     * Associate attributes with this range type.  Enables quick setting of
     * default attributes to a group of ranges.
     */
    void addAttribute(KateAttribute* attribute, int activationFlags = activateNone, bool activate = false);
    void removeAttribute(KateAttribute* attribute);
    void changeActivationFlags(KateAttribute* attribute, int flags);

    int attributeCount() const;
    KateAttribute* attribute(int index) const;

    /// Get / set the index of the currently active attribute.
    KateAttribute* activatedAttribute(bool mouseIn, bool caretIn) const;
    void activateAttribute(KateAttribute* attribute);

    /**
     * Quickly modify all ranges of this type in some way...
     */
    /// Tag all ranges for redraw
    void tagAll();
    /// Delete all ranges, leaving the text intact
    void clearAll();
    /// Delete all ranges and their associated text
    void removeAll();

  private:
    void activateMouse(bool activate = true);
    void connectMouse(KateSuperRange* range, bool doconnect);
    void activateCaret(bool activate = true);
    void connectCaret(KateSuperRange* range, bool doconnect);

    QList<KateSuperRange*> m_ranges;
    QMap<KateAttribute*, int> m_attributes;
    KateAttribute* m_activeAttribute;

    bool m_haveConnectedMouse :1,
         m_haveConnectedCaret :1;
};

#endif
