/* kate-script
 * name: C++/boost Style
 * license: LGPL
 * author: Alex Turbov <i.zaufi@gmail.com>
 * revision: 3
 * kate-version: 3.4
 * priority: 10
 * indent-languages: C++11, C++11/Qt4
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * \warning This indenter designed to be used with my C++ style! It consists
 * of mix of boost and STL styles + some my, unfortunately (still)
 * undocumented additions I've found useful after ~15 years of C++ coding.
 * So \b LOT of things here are \b HARDCODED and I \b DON'T care about other
 * styles!!!
 *
 * Ok, you've been warned :-)
 *
 * Some settings it assumes being in effect:
 * indent-width 4;
 * space-indent true;
 * auto-brackets true;
 * replace-tabs true;
 * replace-tabs-save true;
 *
 * \todo Better to check (assert) some of that modelines...
 */

// required katepart js libraries
require ("range.js");
require ("string.js");

// specifies the characters which should trigger indent, beside the default '\n'
// ':' is for `case'/`default' and class access specifiers: public, protected, private
// '/' is for single line comments
// ',' for parameter list
// '<' and '>' is for templates
// '#' is for preprocessor directives
// ')' is for align dangling close bracket
// ';' is for align `for' parts
// TBD <others>
triggerCharacters = "{}()<>/:;,#\\?|/%.";

var debugMode = true;

/// \todo Move to a separate library?
function dbg()
{
    if (debugMode)
    {
        debug.apply(this, arguments);
    }
}

//BEGIN global variables and functions
var gIndentWidth = 4;
var gSameLineCommentStartAt = 60;                           ///< Position for same-line-comments (inline comments)
var gMode = "C++11";
var gAttr = "Normal Text";
var gBraceMap = {
    '(': ')', ')': '('
  , '<': '>', '>': '<'
  , '{': '}', '}': '{'
  , '[': ']', ']': '['
  };
//END global variables and functions

/// Check if given line/column located withing a braces
function isInsideBraces(line, column, ch)
{
    var cursor = document.anchor(line, column, ch);
    return cursor.isValid();
}

/**
 * Split a given text line by comment into parts \e before and \e after the comment
 * \return an object w/ the following fields:
 *   \li \c hasComment -- boolean: \c true if comment present on the line, \c false otherwise
 *   \li \c before -- text before the comment
 *   \li \c after -- text of the comment
 */
function splitByComment(text)
{
    var commentStartPos = text.indexOf("//");
    var before = "";
    var after = "";
    var found = commentStartPos != -1;
    if (found)
    {
        before = text.substring(0, commentStartPos);
        after = text.substring(commentStartPos + 2, text.length);
    }
    else before = text;
    return {hasComment: found, before: before, after: after};
}

function stripComment(text)
{
    return splitByComment(text).before.rstrip();
}

/// Return \c true if attribute at given position is not a \e String or \e Comment
function isNotStringOrComment(line, column)
{
    // Check if we are not withning a string or a comment
    var c = new Cursor(line, column);
    var mode = document.attributeName(c);
    dbg("isNotStringOrComment: Check mode @ " + c + ": " + mode);
    return !(document.isString(c) || document.isComment(c));
}

/// Try to (re)align (to 60th position) inline comment if present
function alignInlineComment(line)
{
    // Check is there any comment on the current line
    var currentLineText = document.line(line);
    var sc = splitByComment(currentLineText);
    // Did we found smth and if so, make sure it is not a string or comment...
    if (sc.hasComment && isNotStringOrComment(line, sc.before.length - 1))
    {
        var rbefore = sc.before.rtrim();
        /// \attention Kate has a BUG: even if everything is Ok and no realign
        /// required, document gets modified anyway! So condition below
        /// designed to prevent document modification w/o real actions won't
        /// help anyway :-( Need to fix Kate before!
        if (rbefore.length < gSameLineCommentStartAt && sc.before.length != gSameLineCommentStartAt)
        {
            // Ok, test on the line is shorter than needed.
            // But what about current padding?
            document.editBegin();
            if (sc.before.length < gSameLineCommentStartAt)
                // Need to add some padding
                document.insertText(
                    line
                  , sc.before.length
                  , String().fill(' ', gSameLineCommentStartAt - sc.before.length)
                  );
            else
                // Need to remove a redundant padding
                document.removeText(line, gSameLineCommentStartAt, line, sc.before.length);
            document.editEnd();
        }
        else if (gSameLineCommentStartAt < rbefore.length)
        {
            // Move inline comment before the current line
            var startPos = document.firstColumn(line);
            currentLineText = String().fill(' ', startPos) + "//" + sc.after.rtrim() + "\n";
            document.editBegin();
            document.removeText(line, rbefore.length, line, document.lineLength(line));
            document.insertText(line, 0, currentLineText);
            document.editEnd();
        }
    }
}

/**
 * Try to keep same-line comment.
 * I.e. if \c ENTER was hit on a line w/ inline comment and before it,
 * try to keep it on a previous line...
 */
function tryToKeepInlineComment(line)
{
    // Make sure that there is some text still present on a prev line
    // i.e. it was jsut splitted and same-line-comment must be moved back to it
    if (document.line(line - 1).trim().length == 0)
        return;

    // Check is there any comment on the current line
    var currentLineText = document.line(line);
    var sc = splitByComment(currentLineText);
    if (sc.hasComment && isNotStringOrComment(line, sc.before.length - 1) && sc.after.length > 0)
    {
        // Ok, here is few cases possible when ENTER pressed in different positions
        // |  |smth|was here; |        |// comment
        //
        // If sc.before has some text, it means that cursor was in the middle of some
        // non-commented text, and part of it left on a prev line, so we have to move
        // the comment back to that line...
        if (sc.before.trim().length > 0)                    // Is there some text before comment?
        {
            var lastPos = document.lastColumn(line - 1);    // Get last position of non space char @ prev line
            // Put the comment text to the prev line w/ padding
            document.insertText(
                line - 1
              , lastPos + 1
              , String().fill(' ', gSameLineCommentStartAt - lastPos - 1)
                  + "//"
                  + sc.after.rtrim()
              );
            // Remove it from current line starting from current position
            // 'till the line end
            document.removeText(line, sc.before.rtrim().length, line, currentLineText.length);
        }
        else
        {
            // No text before comment. Need to remove possible spaces from prev line...
            var prevLine = line - 1;
            document.removeText(
                prevLine
              , document.lastColumn(prevLine) + 1
              , prevLine
              , document.lineLength(prevLine)
              );
        }
    }
}

/**
 * Return a current preprocessor indentation level
 * \note <em>preprocessor indentation</em> means how deep the current line
 * inside of \c #if directives.
 * \warning Negative result means that smth wrong w/ a source code
 */
function getPreprocessorLevelAt(line)
{
    // Just follow towards start and count #if/#endif directives
    var currentLine = line;
    var result = 0;
    while (currentLine >= 0)
    {
        currentLine--;
        var currentLineText = document.line(currentLine);
        if (currentLineText.search(/^\s*#\s*(if|ifdef|ifndef)\s+.*$/) != -1)
            result++;
        else if (currentLineText.search(/^\s*#\s*endif.*$/) != -1)
            result--;
    }
    return result;
}

/**
 * Check if \c ENTER was hit between ()/{}/[]/<>
 * \todo Match closing brace forward, put content between
 * braces on a separate line and align a closing brace.
 */
function tryBraceSplit_ch(line)
{
    var result = -1;
    // Get last char from previous line (opener) and a first from the current (closer)
    var firstCharPos = document.lastColumn(line - 1);
    var firstChar = document.charAt(line - 1, firstCharPos);
    var lastCharPos = document.firstColumn(line);
    var lastChar = document.charAt(line, lastCharPos);

    var isCurveBracketsMatched = (firstChar == '{' && lastChar == '}');
    var isBracketsMatched = isCurveBracketsMatched
        || (firstChar == '[' && lastChar == ']')
        || (firstChar == '(' && lastChar == ')')
        || (firstChar == '<' && lastChar == '>')
        ;
    if (isBracketsMatched)
    {
        var currentIndentation = document.firstVirtualColumn(line - 1);
        result = currentIndentation + gIndentWidth;
        document.editBegin();
        document.insertText(line, document.firstColumn(line), "\n");
        document.indent(new Range(line + 1, 0, line + 1, 1), currentIndentation / gIndentWidth);
        // Add half-tab (2 spaces) if matched not a curve bracket or
        // open character isn't the only one on the line
        var isOpenCharTheOnlyOnLine = (document.firstColumn(line - 1) == firstCharPos);
        if (!(isCurveBracketsMatched || isOpenCharTheOnlyOnLine))
            document.insertText(line + 1, document.firstColumn(line + 1), "  ");
        document.editEnd();
        view.setCursorPosition(line, result);
    }
    if (result != -1)
    {
        dbg("tryBraceSplit_ch result="+result);
        tryToKeepInlineComment(line);
    }
    return result;
}

/**
 * Even if counterpart brace not found (\sa \c tryBraceSplit_ch), align the current line
 * to one level deeper if last char on a previous line is one of open braces.
 * \code
 *     foo(|blah);
 *     // or
 *     {|
 *     // or
 *     smth<|blah, blah>
 *     // or
 *     array[|idx] = blah;
 * \endcode
 */
function tryToAlignAfterOpenBrace_ch(line)
{
    var result = -1;
    var pos = document.lastColumn(line - 1);
    var ch = document.charAt(line - 1, pos);

    if (ch == '{' || ch == '(' || ch == '[')
        result = document.firstColumn(line - 1) + gIndentWidth;
    else if (ch == '<')
    {
        // Does it looks like 'operator<<'?
        if (document.charAt(line - 1, pos - 1) != '<')
            result = document.firstColumn(line - 1) + gIndentWidth;
        else
            result = document.firstColumn(line - 1) + 2;
    }

    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryToAlignOpenBrace_ch result="+result);
    }
    return result;
}

function tryToAlignBeforeCloseBrace_ch(line)
{
    var result = -1;
    var pos = document.firstColumn(line);
    var ch = document.charAt(line, pos);

    if (ch == '}' || ch == ')' || ch == ']')
    {
        var openBracePos = document.anchor(line, pos, ch);
        dbg("Found open brace @ "+openBracePos)
        if (openBracePos.isValid())
            result = document.firstColumn(openBracePos.line) + (ch == '}' ? 0 : 2);
    }
    else if (ch == '>')
    {
        // TBD
    }

    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryToAlignBeforeCloseBrace_ch result="+result);
    }
    return result;
}

function tryToAlignBeforeComma_ch(line)
{
    var result = -1;
    var pos = document.firstColumn(line);
    var ch = document.charAt(line, pos);

    if (line > 0 && (ch == ',' || ch == ';'))
    {
        var openBracePos = document.anchor(line, pos, '(');
        if (!openBracePos.isValid())
            openBracePos = document.anchor(line, pos, '[');

        if (openBracePos.isValid())
            result = document.firstColumn(openBracePos.line) + 2;
    }

    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryToAlignBeforeComma_ch result="+result);
    }
    return result;
}

/// Check if a multiline comment introduced on a previous line
function tryMultilineCommentStart_ch(line)
{
    var result = -1;
    // Check if multiline comment was started on the line
    if (document.startsWith(line - 1, "/*", true))
    {
        var filler = String().fill(' ', document.firstVirtualColumn(line - 1) + 1);
        var padding = filler + "* ";
        // If next line (if present) doesn't looks like a continue of the current comment,
        // then append a comment closer also...
        if ((line + 1) < document.lines())
        {
            if (!document.startsWith(line + 1, "*", true))
                padding += "\n" + filler + "*/";
        }
        else padding += "\n" + filler + "*/\n";             // There is no a next line...

        document.insertText(line, 0, padding);
        view.setCursorPosition(line, padding.length + 4);
        result = filler.length;
    }
    if (result != -1)
        dbg("tryMultilineCommentStart_ch result="+result);
    return result;
}

/// Check if \c ENTER was hit inside or at last line of a multiline comment
function tryMultilineCommentCont_ch(line)
{
    var result = -1;
    // Check if multiline comment continued on the line:
    // 0) it starts w/ a start
    // 1) and followed by a space (i.e. it doesn't looks like a dereference) or nothing
    var firstCharPos = document.firstColumn(line - 1);
    var prevLineFirstChar = document.charAt(line - 1, firstCharPos);
    var prevLineSecondChar = document.charAt(line - 1, firstCharPos + 1);
    if (prevLineFirstChar == '*' && (prevLineSecondChar == ' ' || prevLineSecondChar == -1))
    {
        if (document.charAt(line - 1, firstCharPos + 1) == '/')
            // ENTER pressed after multiline comment: unindent 1 space!
            result = firstCharPos - 1;
        else
        {
            // Ok, ENTER pressed inside of the multiline comment:
            // just append one more line...
            var filler = String().fill(' ', document.firstColumn(line - 1));
            // Try to continue a C-style comment
            document.insertText(line, 0, filler + "* ");
            result = filler.length;
        }
    }
    if (result != -1)
        dbg("tryMultilineCommentCont_ch result="+result);
    return result;
}

/**
 * Check if a current line has a text after cursor position
 * and a previous one has a comment, then append a <em>"// "</em>
 * before cursor and realign if latter was inline comment...
 */
function trySplitComment_ch(line)
{
    var result = -1;
    if (document.lastColumn(line) != -1)
    {
        // Ok, current line has some text after...
        // NOTE There is should be at least one space between
        // the text and the comment
        var match = /^(.*\s)(\/\/)(.*)$/.exec(document.line(line - 1));
        if (match != null && 0 < match[3].trim().length)    // If matched and there is some text in a comment
        {
            if (0 < match[1].trim().length)                 // Is there some text before the comment?
            {
                // Align comment to gSameLineCommentStartAt
                result = gSameLineCommentStartAt;
            }
            else
            {
                result = match[1].length;
            }
            var leadMatch = /^([^\s]*\s+).*$/.exec(match[3]);
            var lead = "";
            if (leadMatch != null)
                lead = leadMatch[1];
            else
                lead = " ";
            document.insertText(line, 0, "//" + lead);
        }
    }
    if (result != -1)
        dbg("trySplitComment_ch result="+result);
    return result;
}

/// Indent a next line after some keywords
function tryIndentAfterSomeKeywords_ch(line)
{
    var result = -1;
    // Check if ENTER was pressed after some keywords...
    var prevString = document.line(line - 1);
    var r = /^(\s*)((if|for|while)\s*\(|\bdo\b|\belse\b|(public|protected|private|default|case\s+.*)\s*:).*$/
      .exec(prevString);
    if (r != null)
    {
        dbg("r=",r);
        result = r[1].length + gIndentWidth;
    }
    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryIndentAfterSomeKeywords_ch result="+result);
    }
    return result;
}

/**
 * Try to indent a line right after a dangling semicolon
 * (possible w/ leading close braces and comment after)
 * \code
 *     foo(
 *         blah
 *     );|
 * \endcode
 */
function tryAfterDanglingSemicolon_ch(line)
{
    var result = -1;
    var prevString = document.line(line - 1);
    var r = /^(\s*)(([\)\]}]?\s*)*([\)\]]\s*))?;(\s*\/\/.*)?$/.exec(prevString);
    if (r != null)
        result = r[1].length - 2;
    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryDanglingSemicolon_ch result="+result);
    }
    return result;
}

/**
 * Check if \c ENTER pressed after equal sign
 * \code
 *     blah =
 *         |blah
 * \endcode
 */
function tryAfterEqualChar_ch(line)
{
    var result = -1;
    var pos = document.lastColumn(line - 1);
    if (document.charAt(line - 1, pos) == '=')
        result = document.firstColumn(line - 1) + gIndentWidth;
    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryAfterEqualChar_ch result="+result);
    }
    return result;
}

/// Check if \c ENTER hits after \c #define w/ a backslash
function tryMacroDefinition_ch(line)
{
    var result = -1;
    var prevString = document.line(line - 1);
    if (prevString.search(/^\s*#\s*define\s+.*\\$/) != -1)
        result = gIndentWidth;
    if (result != -1)
        dbg("tryMacroDefinition_ch result="+result);
    return result;
}

/**
 * Try to align a line w/ a leading (word) delimiter symbol
 * (i.e. not an identifier and a brace)
 */
function tryBeforeDanglingDelimiter_ch(line)
{
    var result = -1;
    dbg("text='"+document.line(line)+"'");
    var halfTabNeeded =
        // current line do not starts w/ a comment
        !document.line(line).ltrim().startsWith("//")
        // if a previous line starts w/ an identifier
      && (document.line(line - 1).search(/^\s*[A-Za-z_][A-Za-z0-9_]*/) != -1)
        // but the current one starts w/ a delimiter (which is looks like operator)
      && (document.line(line).search(/^\s*[,%&<=:\|\-\?\/\+\*\.]/) != -1)
      ;
    // check if we r at function call or array index
    var insideBraces = document.anchor(line, document.firstColumn(line), '(').isValid()
      || document.anchor(line, document.firstColumn(line), '[').isValid()
      ;
    if (halfTabNeeded)
        result = document.firstVirtualColumn(line - 1) + (insideBraces ? -2 : 2);
    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryBeforeDanglingDelimiter_ch result="+result);
    }
    return result;
}

function tryPreprocessor_ch(line)
{
    var result = -1;
    if (document.firstChar(line) == '#')
    {
        result = 0;
        var text = document.line(line);
        // Get current depth level
        var currentLevel = getPreprocessorLevelAt(line);
        if (currentLevel > 0)
        {
            // How much spaces we have after hash?
            var spacesCnt = 0;
            var column = document.firstColumn(line) + 1;
            var i = column;
            for (; i < text.length; i++)
            {
                if (text[i] != ' ')
                    break;
                spacesCnt++;
            }
            var wordAfterHash = document.wordAt(line, i);
            dbg("wordAfterHash='"+wordAfterHash+"'");
            if (wordAfterHash[0] == '#')
                wordAfterHash = wordAfterHash.substring(1, wordAfterHash.length);
            if (wordAfterHash == "else" || wordAfterHash == "elif" || wordAfterHash == "endif")
                currentLevel--;
            var paddingLen = (currentLevel == 0) ? 0 : (currentLevel - 1) * 2 + 1;
            if (spacesCnt < paddingLen)
            {
                var padding = String().fill(' ', paddingLen - spacesCnt);
                document.insertText(line, column, padding);
            }
            else if (paddingLen < spacesCnt)
            {
                document.removeText(line, column, line, column + spacesCnt - paddingLen);
            }
        }
    }
    if (result != -1)
        dbg("tryPreprocessor_ch result="+result);
    return result;
}

/// Wrap \c tryToKeepInlineComment as \e caret-handler
function tryToKeepInlineComment_ch(line)
{
    tryToKeepInlineComment(line);
    return -1;
}

/**
 * \brief Handle \c ENTER key
 */
function caretPressed(cursor)
{
    var result = -1;
    var line = cursor.line;

    // Dunno what to do if previous line isn't available
    if (line - 1 < 0)
        return result;                                      // Nothing (dunno) to do if no previous line...

    // Register all indent functions
    var handlers = [
        tryBraceSplit_ch                                    // Handle ENTER between braces
      , tryMultilineCommentStart_ch
      , tryMultilineCommentCont_ch
      , trySplitComment_ch
      , tryToAlignAfterOpenBrace_ch                         // Handle {,[,(,< on a previous line
      , tryToAlignBeforeCloseBrace_ch                       // Handle },],),> on a current line before cursor
      , tryToAlignBeforeComma_ch                            // Handle ENTER pressed before comma or semicolon
      , tryIndentAfterSomeKeywords_ch                       // NOTE It must follow after trySplitComment_ch!
      , tryAfterDanglingSemicolon_ch
      , tryMacroDefinition_ch
      , tryBeforeDanglingDelimiter_ch
      , tryAfterEqualChar_ch
      , tryPreprocessor_ch
      , tryToKeepInlineComment_ch                           // NOTE This must be a last checker!
    ];

    // Apply all all functions until result gets changed
    for (
        var i = 0
      ; i < handlers.length && result == -1
      ; result = handlers[i++](line)
      );

    return result;
}

/**
 * \brief Handle \c '/' key pressed
 *
 * Check if is it start of a comment. Here is few cases possible:
 * \li very first \c '/' -- do nothing
 * \li just entered \c '/' is a second in a sequence. If no text before or some present after,
 *     do nothing, otherwise align a \e same-line-comment to \c gSameLineCommentStartAt
 *     position.
 * \li just entered \c '/' is a 3rd in a sequence. If there is some text before and no after,
 *     it looks like inlined doxygen comment, so append \c '<' char after. Do nothing otherwise.
 *
 * \todo Due a BUG in a current version of Kate, this code doesn't work as expected!
 * It always returns a <em>"NormalText"</em>!
 * \code
 * var cm = document.attributeName(cursor);
 * if (cm.indexOf("String") != -1)
 *    return;
 * \endcode
 *
 * \bug This code doesn't work properly in the following case:
 * \code
 *  std::string bug = "some text//
 * \endcode
 *
 * \todo Refactoring required to avoid regex here... better to use \c splitByComment()
 */
function trySameLineComment(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    // Fisrt of all check that we r not withing a string
    // BUG See note in a function's documentation
    var cm = document.attributeName(cursor);
    if (cm.indexOf("String") != -1)
       return;

    var match = /^([^\/]*)(\/\/+)(.*)$/.exec(document.line(line));

    if (match != null)                                      // Is matched?
    {
        dbg("match_before  = '" + match[1] + "'");
        dbg("match_comment = '" + match[2] + "'");
        dbg("match_after   = '" + match[3] + "'");
        if (match[2] == "///" && match[3].trim().length == 0)
        {
            // 3rd case here!
            var filler = (match[1].trim().length > 0)       // Is there any text before comment?
                ? "< "                                      // turn it into inline-doxygen comment
                : " ";                                      // just usual doxygen comment
            document.insertText(cursor, filler);
        }
        else if (match[2] == "//" && 0 < match[1].trim().length && match[3].length == 0)
        {
            // 2nd case here! Check if padding required
            if (match[1].length < gSameLineCommentStartAt)
            {
                var filler = String().fill(' ', gSameLineCommentStartAt - match[1].length) + "//";
                document.editBegin();
                document.removeText(line, column - 2, line, column);
                document.insertText(line, column - 2, filler);
                document.editEnd();
            }
        }
    }
}

/**
 * \brief Maybe '>' needs to be added?
 *
 * Here is a few cases possible:
 * \li user entered <em>"template &gt;</em>
 * \li user entered smth like <em>std::map&gt;</em>
 * \li user wants to output smth to C++ I/O stream by typing <em>&gt;&gt;</em>
 */
function tryTemplate(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    // Check for 'template' keyword at line start
    var currentString = document.line(line);
    var prevWord = document.wordAt(line, column - 1);
    dbg("tryTemplate: prevWord='"+prevWord+"'");
    dbg("tryTemplate: prevWord.match="+prevWord.match(/\b[A-Za-z_][A-Za-z0-9_]*/));
    var isCloseAngleBracketNeeded = currentString.match(/^\s*template\s*<$/)
      || prevWord.match(/\b[A-Za-z_][A-Za-z0-9_]*/)         // Does a word before '<' looks like identifier?
      ;
    if (isCloseAngleBracketNeeded)
    {
        document.insertText(cursor, ">");
        view.setCursorPosition(cursor);
    }
    else if (document.charAt(line, column - 2) == '<')
    {
        // Looks like case 3... add a space after operator<<
        document.insertText(line, column, " ");
    }
}

/**
 * \brief Try to align parameters list
 *
 * If (just entered) comma is a first symbol on a line,
 * just move it on a half-tab left relative to a previous line
 * (if latter doesn't starts w/ comma or ':').
 * Do nothing otherwise. A space would be added after it anyway.
 */
function tryComma(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    if (document.firstChar(line) == ',' && document.firstColumn(line) == (column - 1))
    {
        var prevLineFirstChar = document.firstChar(line - 1);
        var mustMove = !(prevLineFirstChar == ',' || prevLineFirstChar == ':');
        result = document.firstColumn(line - 1) - (mustMove ? 2 : 0);
    }
    document.insertText(cursor, " ");                       // Always add one space after comma
    return result;
}

function trySemicolon(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    if (document.firstChar(line) == ';' && document.firstColumn(line) == (column - 1))
    {
        // Check if we are inside a `for' statement
        var openBracePos = document.anchor(line, column, '(');
        if (openBracePos.isValid())
        {
            // Add a half-tab relative '('
            result = document.firstColumn(openBracePos.line) + 2;
            document.insertText(cursor, " ");
        }
    }
    return result;
}

function tryOperator(cursor, ch)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    var halfTabNeeded = document.firstChar(line) == ch
      && document.firstColumn(line) == (column - 1)
      && document.line(line - 1).search(/^\s*[A-Za-z_][A-Za-z0-9_]*/) != -1
      ;
    dbg("halfTabNeeded=",halfTabNeeded);
    if (halfTabNeeded)
    {
        // check if we r at function call or array index
        var insideBraces = document.anchor(line, document.firstColumn(line), '(').isValid()
          || document.anchor(line, document.firstColumn(line), '[').isValid()
          ;
        dbg("insideBraces=",insideBraces);
        result = document.firstColumn(line - 1) + (insideBraces && ch != '.' ? -2 : 2);
    }
    if (ch == '?')
        document.insertText(cursor, " ");                   // Add space only after '?' of a trenary operator
    return result;
}

/**
 * \brief Try to align a given close bracket
 */
function tryCloseBracket(cursor, ch)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    // Check if a given closing brace is a first char on a line
    // (i.e. it is 'dangling' brace)...
    if (document.firstChar(line) == ch && document.firstColumn(line) == (column - 1))
    {
        var braceCursor = Cursor.invalid();
        if (ch != '>')
            braceCursor = document.anchor(line, column - 1, gBraceMap[ch]);
            // TODO Otherwise, it seems we have a template parameters list...
        if (braceCursor.isValid())
            result = document.firstColumn(braceCursor.line) + 2;
    }

    return result;
}

/**
 * \brief Indent new scope block
 *
 * ... try to unindent to be precise...
 */
function tryBlock(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    // Check for a dangling close brace on a previous line
    // (this may mean that `for' or `if' or `while' w/ looong parameters list on it)
    if (document.firstChar(line - 1) == ')')
        result = Math.floor(document.firstColumn(line - 1) / gIndentWidth) * gIndentWidth;
    else
    {
        // Otherwise, check for a keyword on the previous line and
        // indent the started block to it...
        var prevString = document.line(line - 1);
        var r = /^(\s*)((catch|if|for|while)\s*\(|do|else|try|(default|case\s+.*)\s*:).*$/.exec(prevString);
        if (r != null)
            result = r[1].length;
    }
    return result;
}

/**
 * \brief Align preprocessor directives
 */
function tryPreprocessor(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    // Check if just entered '#' is a first on a line
    if (document.firstChar(line) == '#' && document.firstColumn(line) == (column - 1))
    {
        // Get current indentation level
        var currentLevel = getPreprocessorLevelAt(line);
        if (currentLevel > 0)
        {
            var padding = String().fill(' ', (currentLevel - 1) * 2 + 1);
            document.insertText(cursor, padding);
        }
        result = 0;
    }
    return result;
}

/**
 * \brief Try to align access modifiers or class initialization list
 *
 * Here is few cases possible:
 * \li \c ':' pressed after a keyword \c public, \c protected or \c private.
 *     Then align a current line to corresponding class/struct definition.
 *     Check a previous line and if it is not starts w/ \c '{' add a new line before.
 * \li \c ':' is a first char on the line, then it looks like a class initialization
 *     list or 2nd line of trenary operator.
 */
function tryColon(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    // Check if just entered ':' is a first on a line
    if (document.firstChar(line) == ':' && document.firstColumn(line) == (column - 1))
    {
        // Check if there a dangling ')' or '?' (trenary operator) on a previous line
        var ch = document.firstChar(line - 1);
        if (ch == ')' || ch == '?')
            result = document.firstVirtualColumn(line - 1);
        else
            result = document.firstVirtualColumn(line - 1) + 2;
        document.insertText(cursor, " ");
    }
    else
    {
        var currentString = document.line(line);
        if (currentString.search(/^\s*((public|protected|private)\s*(slots|Q_SLOTS)?|(signals|Q_SIGNALS)\s*):\s*$/) != -1)
        {
            var definitionCursor = document.anchor(line, 0, '{');
            if (definitionCursor.isValid())
            {
                result = document.firstVirtualColumn(definitionCursor.line);
            }
        }
    }
    return result;
}

/**
 * \brief Try to add one space after keywords and before an open brace
 */
function tryOpenBrace(cursor)
{
    var line = cursor.line;
    var column = cursor.column;
    var wordBefore = document.wordAt(line, column - 1);
    dbg("word before: '"+wordBefore+"'");
    if (wordBefore.search(/\b(catch|for|if|switch|while)\b/) != -1)
        document.insertText(line, column - 1, " ");
}

function getMacroRange(line)
{
    function stripLastCharAndRTrim(str)
    {
        return str.substring(0, str.length - 1).rtrim();
    }
    var maxLength = 0;
    var macroStartLine = -1;
    // Look up towards begining of a document
    for (var i = line; i >= 0; --i)
    {
        var currentLineText = document.line(i);
        dbg("up: '"+currentLineText+"'");
        if (currentLineText.search(/^\s*#\s*define\s+.*\\$/) != -1)
        {
            macroStartLine = i;
            maxLength = Math.max(maxLength, stripLastCharAndRTrim(currentLineText).length);
            break;                                          // Ok, we've found the macro start!
        }
        else if (currentLineText.search(/\\$/) == -1)
            break;                                          // Oops! No backslash found and #define still not reached!
        maxLength = Math.max(maxLength, stripLastCharAndRTrim(currentLineText).length);
    }

    if (macroStartLine == -1)
        return null;

    // Look down towards end of the document
    var macroEndLine = -1;
    for (var i = line; i < document.lines(); ++i)
    {
        var currentLineText = document.line(i);
        dbg("dw: '"+currentLineText+"'");
        if (currentLineText.search(/\\$/) != -1)            // Make sure the current line have a '\' at the end
        {
            macroEndLine = i;
            maxLength = Math.max(maxLength, stripLastCharAndRTrim(currentLineText).length);
        }
        else break;                                         // No backslash at the end --> end of macro!
    }

    if (macroEndLine == -1)
        return null;

    macroEndLine++;
    return {
        range: new Range(macroStartLine, 0, macroEndLine, 0)
      , max: maxLength
      };
}

/**
 * \brief Try to align a backslashes in macro definition
 *
 * \note It is \b illegal to have smth after a backslash in source code!
 */
function tryBackslash(cursor)
{
    var line = cursor.line;
    var result = getMacroRange(line);                       // Look up and down for macro definition range
    if (result != null)
    {
        dbg("macroRange:",result.range);
        dbg("maxLength:",result.max);
        // Iterate over macro definition, strip backslash
        // and add a padding string up to result.max length + backslash
        document.editBegin();
        for (var i = result.range.start.line; i < result.range.end.line; ++i)
        {
            var currentLineText = document.line(i);
            var originalTextLength = currentLineText.length;
            currentLineText = currentLineText.substring(0, currentLineText.length - 1).rtrim();
            var textLength = currentLineText.length;
            document.removeText(i, textLength, i, originalTextLength);
            document.insertText(i, textLength, String().fill(' ', result.max - textLength + 1) + "\\");
        }
        document.editEnd();
    }
}

/**
 * \brief Process one character
 *
 * NOTE Cursor positioned right after just entered character and has +1 in column.
 *
 */
function processChar(line, ch)
{
    var result = -2;                                        // By default, do nothing...
    var cursor = view.cursorPosition();
    if (!cursor)
        return result;

    // TODO Is there any `assert' in JS?
    if (line != cursor.line)
    {
        dbg("ASSERTION FAILURE: line != cursor.line");
        return result;
    }

    var column = cursor.column;

    switch (ch)
    {
        case '\n':
            result = caretPressed(cursor);
            break;
        case '/':
            trySameLineComment(cursor);                     // Possible user wants to start a comment
            break;
        case '<':
            tryTemplate(cursor);                            // Possible need to add closing '>' after template
            break;
        case ',':
            result = tryComma(cursor);                      // Possible need to align parameters list
            break;
        case ';':
            result = trySemicolon(cursor);                  // Possible `for ()` loop speaded on few lines
            break;
        case '?':
        case '|':
        case '%':
        case '/':
        case '.':
            result = tryOperator(cursor, ch);               // Possible need to align some operator
            break;
        case '}':
        case ')':
        case ']':
        case '>':
            result = tryCloseBracket(cursor, ch);           // Try to align a given close bracket
            break;
        case '{':
            result = tryBlock(cursor);
            break;
        case '#':
            result = tryPreprocessor(cursor);
            break;
        case ':':
            result = tryColon(cursor);
            break;
        case '(':
            tryOpenBrace(cursor);
            break;
        case '\\':
            tryBackslash(cursor);
            break;
        default:
            break;                                          // Nothing to do...
    }

    return result;
}

function alignPreprocessor(line)
{
    if (tryPreprocessor_ch(line) == -1)                     // Is smth happened?
        return -2;                                          // No! Signal to upper level to try next aligner...
    return 0;                                               // NOTE preprocessor directives always aligned to 0!
}

/**
 * Try to find a next non comment line assuming that a given
 * one is a start or middle of a multiline comment.
 *
 * \attention This function would ignore anything else than
 * a simple comments like this one... I.e. if \b right after
 * star+slash starts anything (non comment, or even maybe after
 * that another one comment begins), it will be \b IGNORED.
 * (Just because this is a damn ugly style!)
 *
 * \return line number or \c 0 if not found
 * \note \c 0 is impossible value, so suitable to indicate an error!
 *
 * \sa \c alignInsideBraces()
 */
function findMultiLineCommentBlockEnd(line)
{
    for (; line < document.lines(); line++)
    {
        var text = document.line(line).rtrim();
        if (text.endsWith("*/"))
            break;
    }
    line++;                                                 // Move to *next* line
    // Make sure it is not another one comment, and if so,
    // going to find it's end as well...
    var currentLineText = document.line(line).ltrim();
    if (currentLineText.startsWith("//"))
        line = findSingleLineCommentBlockEnd(line);
    else if (currentLineText.startsWith("/*"))
        line = findMultiLineCommentBlockEnd(line);
    else if (document.lines() <= line)
        line = 0;
    return line;
}

/**
 * Try to find a next non comment line assuming that a given
 * one is a single-line one
 *
 * \return line number or \c 0 if not found
 * \note \c 0 is impossible value, so suitable to indicate an error!
 *
 * \sa \c alignInsideBraces()
 */
function findSingleLineCommentBlockEnd(line)
{
    while (++line < document.lines())
    {
        var text = document.line(line).ltrim();
        if (text.length == 0) continue;                     // Skip empty lines...
        if (!text.startsWith("//")) break;                  // Yeah! Smth was found finally.
    }
    // Make sure it is not another one multiline comment, and if so,
    // going to find it's end as well...
    var currentLineText = document.line(line).ltrim();
    if (currentLineText.startsWith("/*"))
        line = findMultiLineCommentBlockEnd(line);
    else if (document.lines() <= line)
        line = 0;
    return line;
}

/**
 * Almost anything in a code is places whithin a some brackets.
 * So the ideas is simple:
 * \li find nearest open bracket of any kind
 * \li depending on its type and presence of leading delimiters (non identifier charscters)
 *     add one or half TAB relative a first non-space char of a line w/ found bracket.
 *
 * But here is some details:
 * \li do nothing on empty lines
 * \li do nothing if first position is a \e string
 * \li align comments according next non-comment and non-preprocessor line
 *     (i.e. it's desired indent cuz it maybe still unaligned)
 *
 * \attention Current Kate version has a BUG: \c anchor() unable to find smth
 * in a multiline macro definition (i.e. where every line ends w/ a backslash)!
 */
function alignInsideBraces(line)
{
    // Make sure there is a text on a line, otherwise nothing to align here...
    var thisLineIndent = document.firstColumn(line);
    if (thisLineIndent == -1 || document.isString(line, 0))
        return 0;

    // Check for comment on the current line
    var currentLineText = document.line(line).ltrim();
    var nextNonCommentLine = -1;
    var middleOfMultilineBlock = false;
    var isSingleLineComment = false;
    if (currentLineText.startsWith('//'))                   // Is single line comment on this line?
    {
        dbg("found a single-line comment");
        // Yep, go to find a next non-comment line...
        nextNonCommentLine = findSingleLineCommentBlockEnd(line);
        isSingleLineComment = true;
    }
    else if (currentLineText.startsWith('/*'))              // Is multiline comment starts on this line?
    {
        // Yep, go to find a next non-comment line...
        dbg("found start of a multiline comment");
        nextNonCommentLine = findMultiLineCommentBlockEnd(line);
    }
    // Are we already inside of a multiline comment?
    // NOTE To be sure that we are not inside of #if0/#endif block,
    // lets check that current line starts w/ '*' also!
    // NOTE Yep, it is expected (hardcoeded) that multiline comment has
    // all lines strarted w/ a star symbol!
    else if (document.isComment(line, 0) && currentLineText.startsWith("*"))
    {
        dbg("found middle of a multiline comment");
        // Yep, go to find a next non-comment line...
        nextNonCommentLine = findMultiLineCommentBlockEnd(line);
        middleOfMultilineBlock = true;
    }

    if (nextNonCommentLine == 0)                            // End of comment not found?
        // ... possible due temporary invalid code...
        // anyway, dunno how to align it!
        return -2;
    // So, are we inside a comment? (and we know where it ends)
    if (nextNonCommentLine != -1)
    {
        // Yep, lets try to get desired indent for next non-comment line
        dbg("** next non comment line is "+nextNonCommentLine);
        dbg("** going to get its' indentation...");
        var desiredIndent = indentLine(nextNonCommentLine);
        dbg("** got desired indentation for next non comment line: "+desiredIndent);
        if (desiredIndent < 0)
            return desiredIndent;                           // Have no idea how to indent this comment!
        // TODO Make sure that next non-comment line do not starts
        // w/ 'special' chars...
        return desiredIndent + middleOfMultilineBlock;
    }

    var brackets = [
        document.anchor(line, document.firstColumn(line), '(')
      , document.anchor(line, document.firstColumn(line), '{')
      , document.anchor(line, document.firstColumn(line), '[')
      ].sort();
    dbg("Found open brackets @ "+brackets);

    // Check if we are at some brackets, otherwise do nothing
    var nearestBracket = brackets[brackets.length - 1];
    if (!nearestBracket.isValid())
        return 0;

    // Make sure it is not a `namespace' level
    // NOTE '{' brace should be at the same line w/ a 'namespace' keyword
    // (yep, according my style... :-)
    var bracketChar = document.charAt(nearestBracket);
    var parentLineText = document.line(nearestBracket.line).ltrim();
    if (bracketChar == '{' && parentLineText.startsWith("namespace"))
        return 0;

    // Ok, (re)align it!
    var result = -2;
    switch (bracketChar)
    {
        case '{':
        case '(':
        case '[':
            // If current line has some leading delimiter, i.e. non alphanumeric character
            // add a half-TAB, otherwise add a one TAB... if needed!
            var parentIndent = document.firstColumn(nearestBracket.line);
            var openBraceIsFirst = parentIndent == nearestBracket.column;
            var firstChar = document.charAt(line, thisLineIndent);
            var isCloseBraceFirst = firstChar == ')' || firstChar == ']' || firstChar == '}';
            var doNotAddAnything = openBraceIsFirst && isCloseBraceFirst;
            var mustAddHalfTab = (!openBraceIsFirst && isCloseBraceFirst)
              || firstChar == ','
              || firstChar == '?'
              || firstChar == ':'
              || firstChar == ';'
              ;
            var desiredIndent = parentIndent + (
                mustAddHalfTab
              ? 2
              : (doNotAddAnything ? 0 : gIndentWidth)
              );
            result = desiredIndent;                         // Reassign a result w/ desired value!
            //BEGIN SPAM
            dbg("parentIndent="+parentIndent);
            dbg("openBraceIsFirst="+openBraceIsFirst);
            dbg("firstChar="+firstChar);
            dbg("isCloseBraceFirst="+isCloseBraceFirst);
            dbg("doNotAddAnything="+doNotAddAnything);
            dbg("mustAddHalfTab="+mustAddHalfTab);
            dbg("desiredIndent="+desiredIndent);
            //END SPAM
            break;
        default:
            dbg("Dunno how to align this line...");
            break;
    }
    return result;
}

function alignAccessSpecifier(line)
{
    var result = -2;
    var currentLineText = document.line(line).ltrim();
    var match = currentLineText.search(
        /^\s*((public|protected|private)\s*(slots|Q_SLOTS)?|(signals|Q_SIGNALS)\s*):\s*$/
      );
    if (match != -1)
    {
        // Ok, lets find an open brace of the `class'/`struct'
        var openBracePos = document.anchor(line, document.firstColumn(line), '{');
        if (openBracePos.isValid())
            result = document.firstColumn(openBracePos.line);
    }
    return result;
}

/**
 * Try to align \c case statements in a \c switch
 *
 * \todo Nowadays it is badly required a function, similar to \c anchor(),
 * to get a \c Range starting from open brace 'till corresponding close brace.
 * Without such fucntion if would be kinda hard to implement what I want
 * to do w/ \c case indenter... so let it be damn simple/stupid for awhile...
 *
 * \bug Due a lack of required functions in Kate (and yes, I'm lazy to write it
 * using pure JS), it's incorrectly indent \c break statement (actually it doesn't
 * care about \c break at all nowadays).
 */
function alignCase(line)
{
    var result = -2;
    var currentLineText = document.line(line).ltrim();
    if (currentLineText.startsWith("case ") || currentLineText.startsWith("default:"))
    {
        // Ok, lets find an open brace of the `switch'
        var openBracePos = document.anchor(line, document.firstColumn(line), '{');
        if (openBracePos.isValid())
            result = document.firstColumn(openBracePos.line) + gIndentWidth;
    }
    return result;
}

/**
 * Try to align a given line
 * \todo More actions
 */
function indentLine(line)
{
    dbg(">> Going to indent line "+line);
    var result = alignPreprocessor(line);                   // Try to align a preprocessor directive
    if (result == -2)                                       // Nothing has changed?
        result = alignAccessSpecifier(line);                // Try to align access specifiers in a class
    if (result == -2)                                       // Nothing has changed?
        result = alignCase(line);                           // Try to align `case' statements in a `switch'
    if (result == -2)                                       // Nothing has changed?
        result = alignInsideBraces(line);                   // Try to align a generic line
    alignInlineComment(line);                               // Always try to align inline comments

    dbg("indentLine result="+result);

    if (result == -2)                                       // Still dunno what to do?
        result = -1;                                        // ... just align according a previous non empty line
    return result;
}

/**
 * \brief Process a newline or one of \c triggerCharacters character.
 *
 * This function is called whenever the user hits \c ENTER key.
 *
 * It gets three arguments: \c line, \c indentwidth in spaces and typed character
 *
 * Called for each newline (<tt>ch == \n</tt>) and all characters specified in
 * the global variable \c triggerCharacters. When calling \e Tools->Align
 * the variable \c ch is empty, i.e. <tt>ch == ''</tt>.
 */
function indent(line, indentWidth, ch)
{
    // NOTE Update some global variables
    gIndentWidth = indentWidth;
    gMode = document.highlightingModeAt(view.cursorPosition());
    gAttr = document.attributeName(view.cursorPosition());

    dbg("indentWidth: " + indentWidth);
    dbg("      gMode: " + gMode);
    dbg("      gAttr: " + gAttr);
    dbg("       line: " + line);
    dbg("         ch: '" + ch + "'");

    if (ch != "")
        return processChar(line, ch);

    return indentLine(line);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
