/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_ATTRIBUTE_H
#define KDELIBS_KTEXTEDITOR_ATTRIBUTE_H

#include <kdelibs_export.h>

#include <QColor>
#include <QTextCharFormat>

class KAction;

namespace KTextEditor
{

class SmartRange;

/**
 * The Attribute class describes text decorations.
 *
 * TODO: store the actual font as well.
 * TODO: update changed mechanism - use separate bitfield
 */
class KTEXTEDITOR_EXPORT Attribute
{
  friend class SmartRange;

  public:
    enum items {
      Weight = 0x1,
      Bold = 0x2,
      Italic = 0x4,
      Underline = 0x8,
      StrikeOut = 0x10,
      Outline = 0x20,
      TextColor = 0x40,
      SelectedTextColor = 0x80,
      BGColor = 0x100,
      SelectedBGColor = 0x200,
      Overline = 0x400
    };

    Attribute();
    virtual ~Attribute();

    // BEGIN Action attachment
    /**
     * Attach an action to this attribute.  When assigned to a range, this attribute
     * will enable the attached action(s) when the caret enters the range, and
     * disable them on exit.  The action is also added to the context menu when
     * the caret is within an associated range.
     */
    void attachAction(KAction* action);

    /**
     * Remove an action from this attribute; it will no longer be managed by associated
     * ranges.
     */
    void detachAction(KAction* action);

    /**
     * Returns a list of currently associated KActions.
     */
    const QList<KAction*>& associatedActions() const { return m_associatedActions; }
    // END

    // BEGIN Dynamic highlighting
    /**
     * Several automatic activation mechanisms exist for associated attributes.
     * Using this you can conveniently have your ranges highlighted on mouse|cursor in|out.
     */
    enum ActivationFlag {
      ActivateMouseIn     = 0x1,
      ActivateMouseOut    = 0x2,
      ActivateCaretIn     = 0x4,
      ActivateCaretOut    = 0x8
    };
    Q_DECLARE_FLAGS(ActivationFlags, ActivationFlag);

    Attribute* dynamicAttribute(ActivationFlags activationFlags) const;
    void setDynamicAttribute(ActivationFlags activationFlags, Attribute* attribute);
    // END

    QFont font(const QFont& ref);
    const QTextCharFormat& toFormat() const;

    inline bool itemSet(int item) const
    { return item & m_itemsSet; };

    inline bool isSomethingSet() const
    { return m_itemsSet; };

    inline int itemsSet() const
    { return m_itemsSet; };

    inline void clearAttribute(int item)
    { m_itemsSet &= (~item); }

    inline int weight() const
    { return m_weight; };

    void setWeight(int weight);

    inline bool bold() const
    { return weight() >= QFont::Bold; };

    void setBold(bool enable = true);

    inline bool italic() const
    { return m_italic; };

    void setItalic(bool enable = true);

    inline bool overline() const
    { return m_overline; };

    void setOverline(bool enable = true);

    inline bool underline() const
    { return m_underline; };

    void setUnderline(bool enable = true);

    inline bool strikeOut() const
    { return m_strikeout; };

    void setStrikeOut(bool enable = true);

    inline const QColor& outline() const
    { return m_outline; };

    void setOutline(const QColor& color);

    inline const QColor& textColor() const
    { return m_textColor; };

    void setTextColor(const QColor& color);

    inline const QColor& selectedTextColor() const
    { return m_selectedTextColor; };

    void setSelectedTextColor(const QColor& color);

    inline const QColor& bgColor() const
    { return m_bgColor; };
    inline bool bgColorFillWhitespace() const
    { return m_bgColorFillWhitespace; };

    void setBGColor(const QColor& color, bool fillWhitespace = false);

    inline const QColor& selectedBGColor() const
    { return m_selectedBGColor; };

    void setSelectedBGColor(const QColor& color);

    Attribute& operator+=(const Attribute& a);

    friend bool operator ==(const Attribute& h1, const Attribute& h2)
    {
      if (h1.m_itemsSet != h2.m_itemsSet)
        return false;

      if (h1.itemSet(Attribute::Weight))
        if (h1.m_weight != h2.m_weight)
          return false;

      if (h1.itemSet(Attribute::Italic))
        if (h1.m_italic != h2.m_italic)
          return false;

      if (h1.itemSet(Attribute::Underline))
        if (h1.m_underline != h2.m_underline)
          return false;

      if (h1.itemSet(Attribute::StrikeOut))
        if (h1.m_strikeout != h2.m_strikeout)
          return false;

      if (h1.itemSet(Attribute::Outline))
        if (h1.m_outline != h2.m_outline)
          return false;

      if (h1.itemSet(Attribute::TextColor))
        if (h1.m_textColor != h2.m_textColor)
          return false;

      if (h1.itemSet(Attribute::SelectedTextColor))
        if (h1.m_selectedTextColor != h2.m_selectedTextColor)
          return false;

      if (h1.itemSet(Attribute::BGColor))
        if (h1.m_bgColor != h2.m_bgColor)
          return false;

      if (h1.itemSet(Attribute::SelectedBGColor))
        if (h1.m_selectedBGColor != h2.m_selectedBGColor)
          return false;

      return true;
    }

    friend bool operator !=(const Attribute& h1, const Attribute& h2);

    virtual void changed() { m_changed = true; };
    bool isChanged() { bool ret = m_changed; m_changed = false; return ret; };

    void clear();

  private:
    // Add / remove ranges to tag if an attribute changes.
    void addRange(SmartRange* range);
    void removeRange(SmartRange* range);

    QList<SmartRange*> m_usingRanges;
    QList<KAction*> m_associatedActions;
    Attribute* m_dynamicAttributes[4];

    mutable QTextCharFormat m_format;
    int m_weight;
    bool m_italic : 1,
      m_underline : 1,
      m_overline : 1,
      m_strikeout : 1,
      m_changed : 1,
      m_bgColorFillWhitespace : 1;
    QColor m_outline, m_textColor, m_selectedTextColor, m_bgColor, m_selectedBGColor;
    int m_itemsSet;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
