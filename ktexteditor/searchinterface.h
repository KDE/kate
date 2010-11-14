/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

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

#ifndef KDELIBS_KTEXTEDITOR_SEARCHINTERFACE_H
#define KDELIBS_KTEXTEDITOR_SEARCHINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/range.h>

class QString;

namespace KTextEditor
{

class Document;

/**
 * \brief Search interface options.
 */
namespace Search
{
  /**
   * \brief Search flags for use with searchText.
   *
   * Modifies the behavior of searchText.
   * By default it is searched for a case-sensitive plaintext pattern,
   * without processing of escape sequences, with "whole words" off,
   * in forward direction, within a non-block-mode text range.
   *
   * \author Sebastian Pipping \<webmaster@hartwork.org\>
   */
  enum SearchOptionsEnum
  {
    Default             = 0,      ///< Default settings

    // modes
    Regex               = 1 << 1, ///< Treats the pattern as a regular expression

    // options for all modes
    CaseInsensitive     = 1 << 4, ///< Ignores cases, e.g. "a" matches "A"
    Backwards           = 1 << 5, ///< Searches in backward direction
    BlockInputRange     = 1 << 6, ///< Treats the input range as a recantgle (block-selection mode)

    // options for plaintext
    EscapeSequences     = 1 << 10, ///< Plaintext mode: Processes escape sequences
    WholeWords          = 1 << 11, ///< Plaintext mode: Whole words only, e.g. @em not &quot;amp&quot; in &quot;example&quot;

    // options for regex
    DotMatchesNewline   = 1 << 15  ///< Regex mode: Makes "." match newlines
  };

  Q_DECLARE_FLAGS(SearchOptions, SearchOptionsEnum)
  Q_DECLARE_OPERATORS_FOR_FLAGS(SearchOptions)
}

/**
 * \brief Search interface extension for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section searchiface_intro Introduction
 *
 * The SearchInterface provides methods to search for a given text pattern in
 * a Document. You can either search for a simple text or for a regular
 * expression, see searchText.
 *
 * \section searchiface_access Accessing the SearchInterface
 *
 * The SearchInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::SearchInterface *iface =
 *     qobject_cast<KTextEditor::SearchInterface*>( doc );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT SearchInterface
{
  public:
    /**
     * Constructor.
     */
    SearchInterface();

    /**
     * Virtual destructor.
     */
    virtual ~SearchInterface();

  public:
    /**
     * \brief Searches the given input range for a text pattern.
     *
     * Searches for a text pattern within the given input range.
     * The kind of search performed depends on the <code>options</code>
     * used. Use this function for plaintext searches as well as
     * regular expression searches. Query supportedSearchOptions
     * to find out, which options the current implementation does
     * support. If no match is found the first (and only) element
     * in the vector return is the invalid range. When searching
     * for regular expressions, the first element holds the
     * range of the full match, the subsequent elements hold
     * the ranges of the capturing parentheses.
     *
     * \param range    Input range to search in
     * \param pattern  Text pattern to search for
     * \param options  Combination of search flags
     * \return         List of ranges (length >=1)
     *
     * \see Search::SearchOptionsEnum
     * \author Sebastian Pipping \<webmaster@hartwork.org\>
     */
    virtual QVector<KTextEditor::Range> searchText(
        const KTextEditor::Range & range,
        const QString & pattern,
        const Search::SearchOptions options = Search::Default) = 0;

    /**
     * \brief Specifies all options supported by searchText.
     *
     * \return  Combination of all flags supported by searchText
     *
     * \see Search::SearchOptionsEnum
     * \author Sebastian Pipping \<webmaster@hartwork.org\>
     */
    virtual Search::SearchOptions supportedSearchOptions() const = 0;

  private:
    class SearchInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SearchInterface, "org.kde.KTextEditor.SearchInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
