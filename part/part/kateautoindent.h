/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>
   Copyright (C) 2004 >Anders Lund <anders@alweb.dk> (KateVarIndent class)

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

#ifndef __KATE_AUTO_INDENT_H__
#define __KATE_AUTO_INDENT_H__

#include <qobject.h>

#include "katecursor.h"
#include "kateconfig.h"

class KateDocument;

/**
 * Provides Auto-Indent functionality for katepart.
 */
class KateAutoIndent
{
  /**
   * Static methods to create and list indention modes
   */
  public:
    /**
     * Create an indenter
     * @param doc document for the indenter
     * @param mode indention mode wanted
     * @return created autoindention object
     */
    static KateAutoIndent *createIndenter (KateDocument *doc, uint mode);

    /**
     * List all possible modes by name
     * @return list of modes
     */
    static QStringList listModes ();

    /**
     * Return the mode name given the mode
     * @param mode mode index
     * @return name for this mode index
     */
    static QString modeName (uint mode);

    /**
     * Return the mode description
     * @param mode mode index
     * @return mode index
     */
    static QString modeDescription (uint mode);

    /**
     * Maps name -> index
     * @param name mode name
     * @return mode index
     */
    static uint modeNumber (const QString &name);

  public:
    /**
     * Constructor
     * @param doc parent document
     */
    KateAutoIndent (KateDocument *doc);

    /**
     * Virtual Destructor for the baseclass
     */
    virtual ~KateAutoIndent ();

    /**
     * Update indenter's configuration (indention width, attributes etc.)
     */
    void updateConfig ();

    /**
     * Called every time a newline character is inserted in the document.
     *
     * @param cur The position to start processing. Contains the new cursor position after the indention.
     * @param needContinue Used to determine whether to calculate a continue indent or not.
     */
    virtual void processNewline (KateDocCursor &cur, bool needContinue);

    /**
     * Called every time a character is inserted into the document.
     * @param c character inserted
     */
    virtual void processChar (QChar /*c*/) { }

    /**
     * Aligns/indents the given line to the proper indent position.
     */
    virtual void processLine (KateDocCursor &/*line*/) { }

    /**
     * Processes a section of text, indenting each line in between.
     */
    virtual void processSection (KateDocCursor &/*begin*/, KateDocCursor &/*end*/) { }

    /**
     * Set to true if an actual implementation of 'processLine' is present.
     * This is used to prevent a needless Undo action from being created.
     */
    virtual bool canProcessLine() { return false; }

    /**
     * Mode index of this mode
     * @return modeNumber
     */
    virtual uint modeNumber () const { return KateDocumentConfig::imNormal; };

  protected:

    /**
     * Determines if the characters open and close are balanced between @p begin and @p end
     * Fills in @p pos with the column position of first opened character if found.
     *
     * @param begin Beginning cursor position.
     * @param end Ending cursor position where the processing will stop.
     * @param open The open character.
     * @param close The closing character which should be matched against @p open.
     * @param pos Contains the position of the first @p open character in the line.
     * @return True if @p open and @p close have an equal number of occurances between @p begin and @p end. False otherwise.
     */
    bool isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close, uint &pos) const;

    /**
     * Skip all whitespace starting at @p cur and ending at @p max. Spans lines if @p newline is set.
     * @p cur is set to the current position afterwards.
     *
     * @param cur The current cursor position to start from.
     * @param max The furthest cursor position that will be used for processing
     * @param newline Whether we are allowed to span multiple lines when skipping blanks
     * @return True if @p cur < @p max after processing.  False otherwise.
     */
    bool skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const;

    /**
     * Measures the indention of the current textline marked by cur
     * @param cur The cursor position to measure the indent to.
     * @return The length of the indention in characters.
     */
    uint measureIndent (KateDocCursor &cur) const;

    /**
     * Produces a string with the proper indentation characters for its length.
     *
     * @param length The length of the indention in characters.
     * @return A QString representing @p length characters (factoring in tabs and spaces)
     */
    QString tabString(uint length) const;

    KateDocument *doc;

    uint  tabWidth;     //!< The number of characters simulated for a tab
    uint  indentWidth;  //!< The number of characters used when tabs are replaced by spaces

    // Attributes that we should skip over or otherwise know about
    uchar commentAttrib;
    uchar doxyCommentAttrib;
    uchar regionAttrib;
    uchar symbolAttrib;
    uchar alertAttrib;
    uchar tagAttrib;
    uchar wordAttrib;
    uchar keywordAttrib;
    uchar normalAttrib;
    uchar extensionAttrib;

    bool  useSpaces;    //!< Should we use spaces or tabs to indent
    bool  keepProfile;  //!< Always try to honor the leading whitespace of lines already in the file
};

class KateCSmartIndent : public KateAutoIndent
{
  public:
    KateCSmartIndent (KateDocument *doc);
    ~KateCSmartIndent ();

    virtual void processNewline (KateDocCursor &begin, bool needContinue);
    virtual void processChar (QChar c);

    virtual void processLine (KateDocCursor &line);
    virtual void processSection (KateDocCursor &begin, KateDocCursor &end);

    virtual bool canProcessLine() { return true; }

    virtual uint modeNumber () const { return KateDocumentConfig::imCStyle; };

  private:
    uint calcIndent (KateDocCursor &begin, bool needContinue);
    uint calcContinue (KateDocCursor &begin, KateDocCursor &end);
    uint findOpeningBrace (KateDocCursor &start);
    uint findOpeningParen (KateDocCursor &start);
    uint findOpeningComment (KateDocCursor &start);
    bool firstOpeningBrace (KateDocCursor &start);
    bool handleDoxygen (KateDocCursor &begin);

    bool allowSemi;
    bool processingBlock;
};

class KatePythonIndent : public KateAutoIndent
{
  public:
    KatePythonIndent (KateDocument *doc);
    ~KatePythonIndent ();

    virtual void processNewline (KateDocCursor &begin, bool needContinue);

    virtual uint modeNumber () const { return KateDocumentConfig::imPythonStyle; };

  private:
    int calcExtra (int &prevBlock, int &pos, KateDocCursor &end);

    static QRegExp endWithColon;
    static QRegExp stopStmt;
    static QRegExp blockBegin;
};

class KateXmlIndent : public KateAutoIndent
{
  public:
    KateXmlIndent (KateDocument *doc);
    ~KateXmlIndent ();

    virtual uint modeNumber () const { return KateDocumentConfig::imXmlStyle; }
    virtual void processNewline (KateDocCursor &begin, bool needContinue);
    virtual void processChar (QChar c);
    virtual void processLine (KateDocCursor &line);
    virtual bool canProcessLine() { return true; }
    virtual void processSection (KateDocCursor &begin, KateDocCursor &end);

  private:
    // sets the indentation of a single line based on previous line
    //  (returns indentation width)
    uint processLine (uint line);

    // gets information about a line
    void getLineInfo (uint line, uint &prevIndent, int &numTags,
      uint &attrCol, bool &unclosedTag);

    // useful regular expressions
    static const QRegExp startsWithCloseTag;
    static const QRegExp unclosedDoctype;
};

class KateCSAndSIndent : public KateAutoIndent
{
  public:
    KateCSAndSIndent (KateDocument *doc);
    ~KateCSAndSIndent ();

    virtual void processNewline (KateDocCursor &begin, bool needContinue);
    virtual void processChar (QChar c);

    virtual void processLine (KateDocCursor &line);
    virtual void processSection (KateDocCursor &begin, KateDocCursor &end);

    virtual bool canProcessLine() { return true; }

    virtual uint modeNumber () const { return KateDocumentConfig::imCSAndS; };

  private:
    void updateIndentString();

    bool inForStatement( int line );
    int lastNonCommentChar( const KateDocCursor &line );
    bool startsWithLabel( int line );
    bool inStatement( const KateDocCursor &begin );
    QString continuationIndent( const KateDocCursor &begin );

    QString calcIndent (const KateDocCursor &begin);
    QString calcIndentAfterKeyword(const KateDocCursor &indentCursor, const KateDocCursor &keywordCursor, int keywordPos, bool blockKeyword);
    QString calcIndentInBracket(const KateDocCursor &indentCursor, const KateDocCursor &bracketCursor, int bracketPos);
    QString calcIndentInBrace(const KateDocCursor &indentCursor, const KateDocCursor &braceCursor, int bracePos);

    bool handleDoxygen (KateDocCursor &begin);
    QString findOpeningCommentIndentation (const KateDocCursor &start);

    QString indentString;
};

/**
 * This indenter uses document variables to determine when to add/remove indents.
 *
 * It attempts to get the following variables from the document:
 * - var-indent-indent-after - A rerular expression which will cause a line to
 *   be indented by one unit, if the first non-whitespace-only line above matches.
 * - var-indent-indent - A regular expression, which will cause a matching line
 *   to be indented by one unit.
 * - var-indent-unindent - A regular expression which will cause the line to be
 *   unindented by one unit if matching.
 * - var-indent-triggerchars - a list of characters that should cause the
 *   indentiou to be recalculated immediately when typed.
 *
 * The idea is to provide a somewhat intelligent indentation for perl, php,
 * bash, scheme and in general formats with humble indentation needs.
 */
class KateVarIndent :  public QObject, public KateAutoIndent
{
  Q_OBJECT
  public:
    KateVarIndent( KateDocument *doc );
    virtual ~KateVarIndent();

    virtual void processNewline (KateDocCursor &begin, bool needContinue);
    virtual void processChar (QChar c);

    virtual void processLine (KateDocCursor &line);
    virtual void processSection (KateDocCursor &begin, KateDocCursor &end);

    virtual bool canProcessLine() { return true; }

    virtual uint modeNumber () const { return KateDocumentConfig::imVarIndent; };

  private slots:
    void slotVariableChanged(const QString&, const QString&);
  private:
    class KateVarIndentPrivate *d;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
