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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_ATTRIBUTE_H
#define KDELIBS_KTEXTEDITOR_ATTRIBUTE_H

#include <QtGui/QTextFormat>

#include <ksharedptr.h>
#include <ktexteditor/ktexteditor_export.h>

class KAction;

namespace KTextEditor
{

class SmartRange;

/**
 * \brief A class which provides customized text decorations.
 *
 * The Attribute class extends QTextCharFormat, the class which Qt
 * uses internally to provide formatting information to characters
 * in a text document.
 *
 * In addition to its inherited properties, it provides support for:
 * \li several customized text formatting properties
 * \li dynamic highlighting of associated ranges of text
 * \li binding of actions with associated ranges of text (note: not currently implemented)
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
class KTEXTEDITOR_EXPORT Attribute : public QTextCharFormat, public KShared
{
  friend class SmartRange;

  public:
    typedef KSharedPtr<Attribute> Ptr;

    /**
     * Default constructor.  The resulting Attribute has no properties set to begin with.
     */
    Attribute();

    /**
     * Copy constructor.
     */
    Attribute(const Attribute& a);

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

    //BEGIN custom properties
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
      /// Defined to allow storage of dynamic effect information
      AttributeDynamicEffect = 0x10A00,
      /// Defined for internal usage of KTextEditor implementations
      AttributeInternalProperty = 0x10E00,
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
     * Find out if the font weight is set to QFont::Bold.
     *
     * \return \c true if the font weight is exactly QFont::Bold, otherwise \c false
     *
     * \see QTextCharFormat::fontWeight()
     */
    bool fontBold() const;

    /**
     * Set the font weight to QFont::Bold.  If \a bold is \p false, the weight will be set to 0 (normal).
     *
     * \param bold whether the font weight should be bold or not.
     *
     * \see QTextCharFormat::setFontWeight()
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
     * \param foreground brush to be used to draw selected text.
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
    //END

    //BEGIN Action association
    /**
     * \}
     * \name Action association
     *
     * The following functions allow for KAction%s to be associated with attributes,
     * and thus with ranges which use this attribute.
     * 
     * \note This feature is currently not implemented (ETA KDE 4.1).
     * \{
     */
    /**
     * Associate an action with this attribute.  When assigned to a range, this attribute
     * will enable the associated action(s) when the caret enters the range, and
     * disable them on exit.  The action is also added to the context menu when
     * the caret is within an associated range.
     *
     * \param action KAction to associate with this Attribute
     */
    void associateAction(KAction* action);

    /**
     * Remove the association with an action from this attribute; it will no
     * longer be managed by associated ranges.
     *
     * \param action KAction to dissociate from this Attribute
     */
    void dissociateAction(KAction* action);

    /**
     * Returns a list of currently associated KAction%s.
     */
    const QList<KAction*>& associatedActions() const;

    /**
     * Clears all associations between KAction%s and this attribute.
     */
    void clearAssociatedActions();
    //!\}
    //END

    //BEGIN Dynamic highlighting
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
     * Dynamic effects for display.
     * \todo Pulse and CycleGradient are unclear.
     */
    enum Effect {
      EffectNone          = 0x0 /**< No effect. Just display. */,
      EffectFadeIn        = 0x1 /**< Fade in and stay there. */,
      EffectFadeOut       = 0x2 /**< Fade out to vanish. */,
      EffectPulse         = 0x4 /**< Pulse (throb); change weight. */,
      EffectCycleGradient = 0x8 /**< Cycle colors. */
    };
    Q_DECLARE_FLAGS(Effects, Effect)
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
    Attribute::Ptr dynamicAttribute(ActivationType type) const;

    /**
     * Set the attribute to use when the event referred to by \a type occurs.
     *
     * \note Nested dynamic attributes are ignored.
     *
     * \param type the activation type to set the attribute for
     * \param attribute the attribute to assign. As attribute is refcounted, ownership is not an issue.
     */
    void setDynamicAttribute(ActivationType type, Attribute::Ptr attribute);

    Effects effects() const;
    void setEffects(Effects effects);
    //!\}
    //END

    /**
     * Addition assignment operator.  Use this to merge another Attribute with this Attribute.
     * Where both attributes have a particular property set, the property in \a a will
     * be used.
     *
     * \param a attribute to merge into this attribute.
     */
    Attribute& operator+=(const Attribute& a);

    /**
     * Replacement assignment operator.  Use this to overwrite this Attribute with another Attribute.
     *
     * \param a attribute to assign to this attribute.
     */
    Attribute& operator=(const Attribute& a);

  protected:
    /**
     * \internal
     * Add a range using this attribute.
     * \param range range using this attribute.
     *
    void addRange(SmartRange* range);
    **
     * \internal
     * Remove a range which is no longer using this attribute.
     * \param range range no longer using this attribute.
     *
    void removeRange(SmartRange* range);*/

  private:
    class AttributePrivate* const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Attribute::Effects)

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
