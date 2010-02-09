/* This file is part of the KDE project
   Copyright (C) 2009 Milian Wolff <mail@milianw.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_HIGHLIGHTINTERFACE_H
#define KDELIBS_KTEXTEDITOR_HIGHLIGHTINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>

#include <ktexteditor/attribute.h>
#include <ktexteditor/cursor.h>

namespace KTextEditor
{

class Document;

/**
 * \brief Highlighting information interface for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section highlightiface_intro Introduction
 *
 * The HighlightInterface provides methods to access the Attributes
 * used for highlighting the Document.
 *
 * \section searchiface_access Accessing the HighlightInterface
 *
 * The HighlightInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::HighlightInterface *iface =
 *     qobject_cast<KTextEditor::HighlightInterface*>( doc );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Milian Wolff \<mail@milianw.de\>
 */
class KTEXTEDITOR_EXPORT HighlightInterface
{
  public:
    ///TODO: Documentation
    enum DefaultStyle {
      dsNormal,
      dsKeyword,
      dsDataType,
      dsDecVal,
      dsBaseN,
      dsFloat,
      dsChar,
      dsString,
      dsComment,
      dsOthers,
      dsAlert,
      dsFunction,
      dsRegionMarker,
      dsError
    };
    
    /**
     * Constructor.
     */
    HighlightInterface();

    /**
     * Virtual destructor.
     */
    virtual ~HighlightInterface();

    /**
     * Returns the attribute used for the style \p ds.
     */
    virtual Attribute::Ptr defaultStyle(const DefaultStyle ds) const = 0;

    /// An AttributeBlock represents an Attribute with its
    /// dimension in a given line.
    ///
    /// \see lineAttributes()
    struct AttributeBlock {
      AttributeBlock(const int _start, const int _length, const Attribute::Ptr & _attribute)
        : start(_start), length(_length), attribute(_attribute)
      {
      }
      /// The column this attribute starts at.
      int start;
      /// The number of columns this attribute spans.
      int length;
      /// The attribute for the current range.
      Attribute::Ptr attribute;
    };

    /**
     * Get the list of AttributeBlocks for a given \p line in the document.
     *
     * \return List of AttributeBlocks for given \p line.
     *
     * TODO: I intended to make this const but Kate's implementation needs to
     * call kateTextline which is non-const. Solution?
     * TODO: Cannot be QVector since we have a CTor. Should it be removed?
     */
    virtual QList<AttributeBlock> lineAttributes(const unsigned int line) = 0;

    /**
     * \brief Get all available highlighting modes for the current document.
     *
     * Each document can be highlighted using an arbitrary number of highlighting
     * contexts. This method will return the names for each of the used modes.
     * 
     * Example: The "PHP (HTML)" mode includes the highlighting for PHP, HTML, CSS and JavaScript.
     *
     * \return Returns a list of embedded highlighting modes for the current Document.
     *
     * \see KTextEditor::Document::highlightingMode()
     */
    virtual QStringList embeddedHighlightingModes() const = 0;

    /**
     * \brief Get the highlight mode used at a given position in the document.
     *
     * Retrieve the name of the applied highlight mode at a given \p position
     * in the current document.
     *
     * \see highlightingModes()
     *
     * TODO: I intended to make this const but Kate's implementation needs to
     * call kateTextline which is non-const. Solution?
     */
    virtual QString highlightingModeAt(const Cursor &position) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::HighlightInterface, "org.kde.KTextEditor.HighlightInterface")

#endif // KDELIBS_KTEXTEDITOR_HIGHLIGHTINTERFACE_H

// kate: space-indent on; indent-width 2; replace-tabs on;
