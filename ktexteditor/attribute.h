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
class KTEXTEDITOR_EXPORT Attribute : public QTextCharFormat
{
  friend class SmartRange;

  public:
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

    /**
     * Clears all associations between KActions and this attribute.
     */
    void clearAssociatedActions();
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
    Q_DECLARE_FLAGS(ActivationFlags, ActivationFlag)

    Attribute* dynamicAttribute(ActivationFlags activationFlags) const;
    void setDynamicAttribute(ActivationFlags activationFlags, Attribute* attribute);
    // END

    // BEGIN custom properties
    enum CustomProperties {
      Outline = QTextFormat::UserProperty,
      SelectedForeground,
      SelectedBackground,
      BackgroundFillWhitespace
    };

    QBrush outline() const;
    void setOutline(const QBrush& brush);

    QBrush selectedForeground() const;
    void setSelectedForeground(const QBrush& foreground);

    bool backgroundFillWhitespace() const;
    void setBackgroundFillWhitespace(bool fillWhitespace);

    QBrush selectedBackground() const;
    void setSelectedBackground(const QBrush& brush);
    // END

    // Fix deficiencies in QText{Char}Format
    void clear();
    bool hasAnyProperty() const;
    bool fontBold() const;
    void setFontBold(bool bold = true);

    Attribute& operator+=(const Attribute& a);

  private:
    // Add / remove ranges to tag if an attribute changes.
    void addRange(SmartRange* range);
    void removeRange(SmartRange* range);

    QList<SmartRange*> m_usingRanges;
    QList<KAction*> m_associatedActions;
    Attribute* m_dynamicAttributes[4];
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
