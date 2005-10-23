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

#include <QTextCharFormat>

class KAction;

namespace KTextEditor
{

class SmartRange;

/**
 * \brief A class which provides customised text decorations.
 *
 * The Attribute class extends QTextCharFormat, the class which Qt
 * uses internally to provide formatting information to characters
 * in a text document.
 *
 * In addition to its inherited properties, it provides support for:
 * \li several customised text formatting properties
 * \li dynamic highlighting of associated ranges of text
 * \li binding of actions with associated ranges of text
 *
 * Implementations are not required to support all properties.
 * In particular, several properties are not supported for dynamic
 * highlighting (notably: font() and fontBold()).
 *
 * Unfortunately, as QTextFormat's setProperty() is not virtual,
 * changes that are made to this attribute cannot automatically be
 * redrawn.  Once you have finished changing properties, you should
 * call changed() to force redrawing of affected ranges of text.
 *
 * \sa SmartInterface
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT Attribute : public QTextCharFormat
{
  friend class SmartRange;

  public:
    /**
     * Default constructor.  The resulting Attribute has no properties set to begin with.
     */
    Attribute();

    /**
     * Virtual destructor.
     */
    virtual ~Attribute();

    /**
     * Notify the editor implementation that a property of this attribute
     * has been changed.
     *
     * This is used to re-render any text which has this attribute assigned
     * to it.
     */
    void changed() const;

    // BEGIN custom properties
    /**
     * Custom property types, which may or may not be supported by implementations.
     */
    enum CustomProperties {
      /// Draws an outline around the text
      Outline = QTextFormat::UserProperty,
      /// Changes the brush used to paint the text when it is selected
      SelectedForeground,
      /// Changes the brush used to paint the background when it is selected
      SelectedBackground,
      /// Determines whether background color is drawn over whitespace. Defaults to true.
      BackgroundFillWhitespace,
      /// Defined to allow 3rd party code to create their own custom attributes - you may use values at or above this property.
      AttributeUserProperty = 0x110000
    };

    /**
     * \name Custom properties
     *
     * The following functions provide custom properties which can be set for
     * rendering by editor implementations.
     * \{
     */
    /**
     * \overload fontWeight()
     *
     * Find out if the font weight is set to QFont::Bold.
     *
     * \return \e true if the font weight is exactly QFont::Bold, otherwise \e false
     */
    bool fontBold() const;

    /**
     * \overload setFontWeight()
     *
     * Set the font weight to QFont::Bold.  If \a bold is \p false, the weight will be set to 0 (normal).
     *
     * \param bold whether the font weight should be bold or not.
     */
    void setFontBold(bool bold = true);

    /**
     * Get the brush used to draw an outline around text, if any.
     *
     * \return brush to be used to draw an outline, or Qt::NoBrush if no outline is set.
     */
    QBrush outline() const;

    /**
     * Set a brush to be used to draw an outline around text.
     *
     * Use \p clearProperty(Outline) to clear.
     *
     * \param brush brush to be used to draw an outline.
     */
    void setOutline(const QBrush& brush);

    /**
     * Get the brush used to draw text when it is selected, if any.
     *
     * \return brush to be used to draw selected text, or Qt::NoBrush if not set
     */
    QBrush selectedForeground() const;

    /**
     * Set a brush to be used to draw selected text.
     *
     * Use \p clearProperty(SelectedForeground) to clear.
     *
     * \param brush brush to be used to draw selected text.
     */
    void setSelectedForeground(const QBrush& foreground);

    /**
     * Get the brush used to draw the background of selected text, if any.
     *
     * \return brush to be used to draw the background of selected text, or Qt::NoBrush if not set
     */
    QBrush selectedBackground() const;

    /**
     * Set a brush to be used to draw the background of selected text, if any.
     *
     * Use \p clearProperty(SelectedBackground) to clear.
     *
     * \param brush brush to be used to draw the background of selected text
     */
    void setSelectedBackground(const QBrush& brush);

    /**
     * Determine whether background color is drawn over whitespace. Defaults to true if not set.
     *
     * \return whether the background color should be drawn over whitespace
     */
    bool backgroundFillWhitespace() const;

    /**
     * Set whether background color is drawn over whitespace. Defaults to true if not set.
     *
     * Use \p clearProperty(BackgroundFillWhitespace) to clear.
     *
     * \param fillWhitespace whether the background should be drawn over whitespace.
     */
    void setBackgroundFillWhitespace(bool fillWhitespace);

    // Fix deficiencies in QText{Char}Format
    /**
     * Clear all set properties.
     */
    void clear();

    /**
     * Determine if any properties are set.
     *
     * \return \e true if any properties are set, otherwise \e false
     */
    bool hasAnyProperty() const;
    // END

    // BEGIN Action attachment
    /**
     * \}
     * \name Action attachment
     *
     * The following functions allow for KActions to be associated with attributes,
     * and thus with ranges which use this attribute.
     * \{
     */
    /**
     * Attach an action to this attribute.  When assigned to a range, this attribute
     * will enable the attached action(s) when the caret enters the range, and
     * disable them on exit.  The action is also added to the context menu when
     * the caret is within an associated range.
     *
     * \param action KAction to attach to this Attribute
     */
    void attachAction(KAction* action);

    /**
     * Remove an action from this attribute; it will no longer be managed by associated
     * ranges.
     *
     * \param action KAction to detach from this Attribute
     */
    void detachAction(KAction* action);

    /**
     * Returns a list of currently associated KActions.
     */
    const QList<KAction*>& associatedActions() const;

    /**
     * Clears all associations between KActions and this attribute.
     */
    void clearAssociatedActions();
    //!\}
    // END

    // BEGIN Dynamic highlighting
    /**
     * Several automatic activation mechanisms exist for associated attributes.
     * Using this you can conveniently have your ranges highlighted when either
     * the mouse or cursor enter the range.
     */
    enum ActivationType {
      /// Activate attribute on mouse in
      ActivateMouseIn = 0,
      /// Activate attribute on caret in
      ActivateCaretIn
    };
    /**
     * \name Dynamic highlighting
     *
     * The following functions allow for text to be highlighted dynamically based on
     * several events.
     * \{
     */

    /**
     * Return the attribute to use when the event referred to by \a type occurs.
     *
     * \param type the activation type for which to return the Attribute.
     *
     * \returns the attribute to be used for events specified by \a type, or null if none is set.
     */
    Attribute* dynamicAttribute(ActivationType type) const;

    /**
     * Set the attribute to use when the event referred to by \a type occurs.
     *
     * \note Nested dynamic attributes are ignored.
     *
     * \param type the activation type to set the attribute for
     * \param attribute the attribute to assign
     * \param takeOwnership Set to true when this object should take ownership
     *                      of \p attribute. If \e true, \p attribute will be
     *                      deleted when this attribute is deleted.
     */
    void setDynamicAttribute(ActivationType type, Attribute* attribute, bool takeOwnership = false);
    //!\}
    // END

    /**
     * Addition assignment operator.  Use this to merge another Attribute with this Attribute.
     * Where both attributes have a particular property set, the property in \a a will
     * be used.
     *
     * \param a attribute to merge into this attribute.
     */
    Attribute& operator+=(const Attribute& a);

  protected:
    /**
     * \internal
     * Add a range using this attribute.
     * \param range range using this attribute.
     */
    void addRange(SmartRange* range);
    /**
     * \internal
     * Remove a range which is no longer using this attribute.
     * \param range range no longer using this attribute.
     */
    void removeRange(SmartRange* range);

  private:
    class AttributePrivate* d;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
