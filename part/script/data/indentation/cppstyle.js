/* kate-script
 * name: C++/boost Style
 * license: LGPL
 * author: Alex Turbov <i.zaufi@gmail.com>
 * revision: 30
 * kate-version: 3.4
 * priority: 10
 * indent-languages: C++,ISO C++
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
 * More info available here: http://zaufi.github.io/programming/2013/11/29/kate-cppstyle-indenter/
 *
 * Some settings it assumes being in effect:
 * - indent-width 4;
 * - space-indent true;
 * - auto-brackets true; <-- TODO REALLY?
 * - replace-tabs true;
 * - replace-tabs-save true;
 *
 * \todo Better to check (assert) some of that modelines...
 */

// required katepart js libraries
require ("range.js");
require ("string.js");
require ("utils.js")

// specifies the characters which should trigger indent, beside the default '\n'
// ':' is for `case'/`default' and class access specifiers: public, protected, private
// '/' is for single line comments
// ',' for parameter list
// '<' and '>' is for templates
// '#' is for preprocessor directives
// ')' is for align dangling close bracket
// ';' is for align `for' parts
// ' ' is to add a '()' after `if', `while', `for', ...
// TBD <others>
triggerCharacters = "{}()[]<>/:;,#\\?!|&/%.@ '\"=*^";

var debugMode = false;

//BEGIN global variables and functions
var gIndentWidth = 4;
var gSameLineCommentStartAt = 60;                           ///< Position for same-line-comments (inline comments)
var gBraceMap = {
    '(': ')', ')': '('
  , '<': '>', '>': '<'
  , '{': '}', '}': '{'
  , '[': ']', ']': '['
  };
//END global variables and functions

/**
 * Try to (re)align (to 60th position) inline comment if present
 * \return \c true if comment line was moved above
 */
function alignInlineComment(line)
{
    // Check is there any comment on the current line
    var sc = splitByComment(line);
    // Did we found smth and if so, make sure it is not a string or comment...
    if (sc.hasComment && !isStringOrComment(line, sc.before.length - 1))
    {
        var rbefore = sc.before.rtrim();
        var cursor = view.cursorPosition();
        /// \attention Kate has a BUG: even if everything is Ok and no realign
        /// required, document gets modified anyway! So condition below
        /// designed to prevent document modification w/o real actions won't
        /// help anyway :-( Need to fix Kate before!
        if (rbefore.length < gSameLineCommentStartAt && sc.before.length != gSameLineCommentStartAt)
        {
            // Ok, test on the line is shorter than needed.
            // But what about current padding?
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
            // Keep cursor at the place we've found it before
            view.setCursorPosition(cursor);
        }
        else if (gSameLineCommentStartAt < rbefore.length)
        {
            // Move inline comment before the current line
            var startPos = document.firstColumn(line);
            var currentLineText = String().fill(' ', startPos) + "//" + sc.after.rtrim() + "\n";
            document.removeText(line, rbefore.length, line, document.lineLength(line));
            document.insertText(line, 0, currentLineText);
            // Keep cursor at the place we've found it before
            view.setCursorPosition(new Cursor(line + 1, cursor.column));
            return true;
        }
    }
    return false;
}


function tryIndentRelativePrevNonCommentLine(line)
{
    var current_line = line - 1;
    while (0 <= current_line && isStringOrComment(current_line, document.firstColumn(current_line)))
        --current_line;
    if (current_line == -1)
        return -2;

    var prevLineFirstChar = document.firstChar(current_line);
    var needHalfUnindent = !(
        prevLineFirstChar == ','
      || prevLineFirstChar == ':'
      || prevLineFirstChar == '?'
      || prevLineFirstChar == '<'
      || prevLineFirstChar == '>'
      || prevLineFirstChar == '&'
      );
    return document.firstColumn(current_line) - (needHalfUnindent ? 2 : 0);
}

/**
 * Try to keep same-line comment.
 * I.e. if \c ENTER was hit on a line w/ inline comment and before it,
 * try to keep it on a previous line...
 */
function tryToKeepInlineComment(line)
{
    // Make sure that there is some text still present on a prev line
    // i.e. it was just splitted and same-line-comment must be moved back to it
    if (document.line(line - 1).trim().length == 0)
        return;

    // Check is there any comment on the current (non empty) line
    var sc = splitByComment(line);
    dbg("sc.hasComment="+sc.hasComment);
    dbg("sc.before.rtrim().length="+sc.before.rtrim().length);
    if (sc.hasComment)
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
            document.removeText(line, sc.before.rtrim().length, line, document.lineLength(line));
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
        document.insertText(line, document.firstColumn(line), "\n");
        document.indent(new Range(line + 1, 0, line + 1, 1), currentIndentation / gIndentWidth);
        // Add half-tab (2 spaces) if matched not a curve bracket or
        // open character isn't the only one on the line
        var isOpenCharTheOnlyOnLine = (document.firstColumn(line - 1) == firstCharPos);
        if (!(isCurveBracketsMatched || isOpenCharTheOnlyOnLine))
            document.insertText(line + 1, document.firstColumn(line + 1), "  ");
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

    if (ch == '(' || ch == '[')
    {
        result = document.firstColumn(line - 1) + gIndentWidth;
    }
    else if (ch == '{')
    {
        if (document.startsWith(line - 1, "namespace", true))
            result = 0;
        else
            result = document.firstColumn(line - 1) + gIndentWidth;
    }
    else if (ch == '<')
    {
        // Does it looks like 'operator<<'?
        if (document.charAt(line - 1, pos - 1) != '<')
            result = document.firstColumn(line - 1) + gIndentWidth;
        else
            result = document.firstColumn(line - 1) + (gIndentWidth / 2);
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
    // and ENTER wan't pressed right after a /*C-style comment*/
    if (document.startsWith(line - 1, "/*", true) && !document.endsWith(line - 1, "*/", true))
    {
        var filler = String().fill(' ', document.firstVirtualColumn(line - 1) + 1);
        var padding = filler + "* ";
        // If next line (if present) doesn't looks like a continue of the current comment,
        // then append a comment closer also...
        if ((line + 1) < document.lines())
        {
            // Maybe user wants to extend a multiline C-style/Doxygen comment
            // by pressing ENTER at start of it?
            if (!document.startsWith(line + 1, "*", true))
            {
                // ... doesn't looks like a multiline comment
                padding += "\n" + filler;
                // Maybe user just splits a C-style comment?
                if (!document.startsWith(line, "*/", true))
                    padding += document.endsWith(line, "*/", true) ? "* " : "*/";
                else
                    document.removeText(line, 0, line, document.firstColumn(line))
            }                                               // else, no need to append a closing */
        }
        else                                                // There is no a next line...
        {
            padding += "\n" + filler;
            if (!document.startsWith(line, "*/", true))
                padding += document.endsWith(line, "*/", true) ? "* " : "*/";
            else
                document.removeText(line, 0, line, document.firstColumn(line))
        }
        document.insertText(line, 0, padding);
        view.setCursorPosition(line, filler.length + 2);
        result = -2;
    }
    if (result != -1)
    {
        dbg("tryMultilineCommentStart_ch result="+result);
    }
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
    {
        dbg("tryMultilineCommentCont_ch result="+result);
    }
    return result;
}

function tryAfterCloseMultilineComment_ch(line)
{
    var result = -1;
    if (document.startsWith(line - 1, "*/", true))
    {
        result = document.firstColumn(line - 1) - 1;
    }
    if (result != -1)
    {
        dbg("tryAfterCloseMultilineComment_ch result="+result);
    }
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
    {
        dbg("trySplitComment_ch result="+result);
    }
    return result;
}

/**
 * \brief Indent a next line after some keywords.
 *
 * Incrase indent after the following keywords:
 * - \c if
 * - \c else
 * - \c for
 * - \c while
 * - \c do
 * - \c case
 * - \c default
 * - \c return
 * - and access modifiers \c public, \c protected and \c private
 */
function tryIndentAfterSomeKeywords_ch(line)
{
    var result = -1;
    // Check if ENTER was pressed after some keywords...
    var sr = splitByComment(line - 1);
    var prevString = sr.before;
    dbg("tryIndentAfterSomeKeywords_ch prevString='"+prevString+"'");
    var r = /^(\s*)((if|for|while)\s*\(|\bdo\b|\breturn\b|(((public|protected|private)(\s+(slots|Q_SLOTS))?)|default|case\s+.*)\s*:).*$/
      .exec(prevString);
    if (r != null)
    {
        dbg("r=",r);
        if (!r[2].startsWith("return") || !prevString.rtrim().endsWith(';'))
            result = r[1].length + gIndentWidth;
    }
    else
    {
        r = /^\s*\belse\b.*$/.exec(prevString)
        if (r != null)
        {
            var prevPrevString = stripComment(line - 2);
            dbg("tryIndentAfterSomeKeywords_ch prevPrevString='"+prevPrevString+"'");
            if (prevPrevString.endsWith('}'))
                result = document.firstColumn(line - 2);
            else if (prevPrevString.match(/^\s*[\])>]/))
                result = document.firstColumn(line - 2) - gIndentWidth - (gIndentWidth / 2);
            else
                result = document.firstColumn(line - 2) - gIndentWidth;
            // Realign 'else' statement if needed
            var pp = document.firstColumn(line - 1);
            if (pp < result)
                document.insertText(line - 1, 0, String().fill(' ', result - pp));
            else if (result < pp)
                document.removeText(line - 1, 0, line - 1, pp - result);
            result += gIndentWidth;
        }
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
    var r = /^(\s*)(([\)\]}]?\s*)*([\)\]]\s*))?;/.exec(prevString);
    if (r != null)
    {
        result = Math.floor(r[1].length / 4) * 4;
    }
    else
    {
        // Does it looks like a template tail?
        // i.e. smth like this:
        // typedef boost::mpl::blah<
        //    params
        //  > type;|
        r = /^(\s*)([>]+).*;/.exec(prevString);
        if (r != null)
            result = Math.floor(r[1].length / 4) * 4;
    }
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
    {
        dbg("tryMacroDefinition_ch result="+result);
    }
    return result;
}

/**
 * Do not incrase indent if ENTER pressed before access
 * specifier (i.e. public/private/protected)
 */
function tryBeforeAccessSpecifier_ch(line)
{
    var result = -1;
    if (document.line(line).match(/(public|protected|private):/))
    {
        var openPos = document.anchor(line, 0, '{');
        if (openPos.isValid())
            result = document.firstColumn(openPos.line);
    }
    if (result != -1)
    {
        tryToKeepInlineComment(line);
        dbg("tryBeforeAccessSpecifier_ch result="+result);
    }
    return result;
}

/**
 * Try to align a line w/ a leading (word) delimiter symbol
 * (i.e. not an identifier and a brace)
 */
function tryBeforeDanglingDelimiter_ch(line)
{
    var result = -1;
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
    {
        dbg("tryPreprocessor_ch result="+result);
    }
    return result;
}

/**
 * Check if \c ENTER was pressed on a start of line and
 * after a block comment.
 */
function tryAfterBlockComment_ch(line)
{
    var result = -1;
    if (0 < line)
    {
        var prev_non_empty_line = document.prevNonEmptyLine(line - 1);
        if (prev_non_empty_line != -1 && document.line(prev_non_empty_line).trim().startsWith("*/"))
        {
            var p = document.firstColumn(prev_non_empty_line);
            if ((p % gIndentWidth) != 0)
                result = Math.floor(p / gIndentWidth) * gIndentWidth;
        }
    }
    if (result != -1)
    {
        dbg("tryAfterBlockComment_ch result="+result);
    }
    return result;
}

/**
 * Check if \c ENTER was pressed after \c break or \c continue statements
 * and if so, unindent the current line.
 */
function tryAfterBreakContinue_ch(line)
{
    var result = -1;
    var currentLineText = document.line(line - 1).ltrim();
    var should_proceed = currentLineText.startsWith("break;") || currentLineText.startsWith("continue;")
    if (should_proceed)
    {
        result = document.firstColumn(line - 1) - gIndentWidth;
    }
    if (result != -1)
    {
        dbg("tryAfterBreakContinue_ch result="+result);
    }
    return result;
}

/// \internal
function getStringAligmentAfterSplit(line)
{
    var prevLineFirstChar = document.firstChar(line - 1);
    var halfIndent = prevLineFirstChar == ','
      || prevLineFirstChar == ':'
      || prevLineFirstChar == '?'
      || prevLineFirstChar == '<'
      || prevLineFirstChar == '>'
      || prevLineFirstChar == '&'
      ;
    return document.firstColumn(line - 1) + (
        prevLineFirstChar != '"'
      ? (halfIndent ? (gIndentWidth / 2) : gIndentWidth)
      : 0
      );
}

/**
 * Handle the case when \c ENTER has pressed in the middle of a string.
 * Find a string begin (a quote char) and analyze if it is a C++11 raw
 * string literal. If it is not, add a "closing" quote to a previous line
 * and to the current one. Align a 2nd part (the moved down one) of a string
 * according a previous line. If latter is a pure string, then give the same
 * indentation level, otherwise incrase it to one \c TAB.
 *
 * Here is few cases possible:
 * - \c ENTER has pressed in a line <code>auto some = ""|</code>, so a new
 *   line just have an empty string or some text which is doesn't matter now;
 * - \c ENTER has pressed in a line <code>auto some = "possible some text here| and here"</code>,
 *   then a new line will have <code> and here"</code> text
 *
 * In both cases attribute at (line-1, lastColumn-1) will be \c String
 */
function trySplitString_ch(line)
{
    var result = -1;
    var column = document.lastColumn(line - 1);

    if (isComment(line - 1, column))
        return result;                                      // Do nothing for comments

    // Check if last char on a prev line has string attribute
    var lastColumnIsString = isString(line - 1, column);
    var firstColumnIsString = isString(line, 0);
    var firstChar = (document.charAt(line, 0) == '"');
    if (!lastColumnIsString)                                // If it is not,
    {
        // TODO TBD
        if (firstColumnIsString && firstChar == '"')
            result = getStringAligmentAfterSplit(line);
        return result;                                      // then nothing to do...
    }

    var lastChar = (document.charAt(line - 1, column) == '"');
    var prevLastColumnIsString = isString(line - 1, column - 1);
    var prevLastChar = (document.charAt(line - 1, column - 1) == '"');
    dbg("trySplitString_ch: lastColumnIsString="+lastColumnIsString);
    dbg("trySplitString_ch: lastChar="+lastChar);
    dbg("trySplitString_ch: prevLastColumnIsString="+prevLastColumnIsString);
    dbg("trySplitString_ch: prevLastChar="+prevLastChar);
    dbg("trySplitString_ch: isString(line,0)="+firstColumnIsString);
    dbg("trySplitString_ch: firstChar="+firstChar);
    var startOfString = firstColumnIsString && firstChar;
    var endOfString = !(firstColumnIsString || firstChar);
    var should_proceed = !lastChar && prevLastColumnIsString && (endOfString || !prevLastChar && startOfString)
      || lastChar && !prevLastColumnIsString && !prevLastChar && (endOfString || startOfString)
      ;
    dbg("trySplitString_ch: ------ should_proceed="+should_proceed);
    if (should_proceed)
    {
        // Add closing quote to the previous line
        document.insertText(line - 1, document.lineLength(line - 1), '"');
        // and open quote to the current one
        document.insertText(line, 0, '"');
        // NOTE If AutoBrace plugin is used, it won't add a quote
        // char, if cursor positioned right before another quote char
        // (which was moved from a line above in this case)...
        // So, lets force it!
        if (startOfString && document.charAt(line, 1) != '"')
        {
            document.insertText(line, 1, '"');              // Add one more!
            view.setCursorPosition(line, 1);                // Step back inside of string
        }
        result = getStringAligmentAfterSplit(line);
    }
    if (result != -1)
    {
        dbg("trySplitString_ch result="+result);
        tryToKeepInlineComment(line);
    }
    return result;
}

/**
 * Here is few cases possible:
 * \code
 *  // set some var to lambda function
 *  auto some = [foo](bar)|
 *
 *  // lambda as a parameter (possible the first one,
 *  // i.e. w/o a leading comma)
 *  std::foreach(
 *      begin(container)
 *    , end(container)
 *    , [](const value_type& v)|
 *    );
 * \endcode
 */
function tryAfterLambda_ch(line)
{
    var result = -1;
    var column = document.lastColumn(line - 1);

    if (isComment(line - 1, column))
        return result;                                      // Do nothing for comments

    var sr = splitByComment(line - 1);
    if (sr.before.match(/\[[^\]]*\]\([^{]*\)[^{}]*$/))
    {
        var align = document.firstColumn(line - 1);
        var before = sr.before.ltrim();
        if (before.startsWith(','))
            align += 2;
        var padding = String().fill(' ', align);
        var tail = before.startsWith('auto ') ? "};" : "}";
        document.insertText(
            line
          , 0
          , padding + "{\n" + padding + String().fill(' ', gIndentWidth) + "\n" + padding + tail
          );
        view.setCursorPosition(line + 1, align + gIndentWidth);
        result = -2;
    }

    if (result != -1)
    {
        dbg("tryAfterLambda_ch result="+result);
    }
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
      , tryAfterCloseMultilineComment_ch
      , trySplitComment_ch
      , tryToAlignAfterOpenBrace_ch                         // Handle {,[,(,< on a previous line
      , tryToAlignBeforeCloseBrace_ch                       // Handle },],),> on a current line before cursor
      , tryToAlignBeforeComma_ch                            // Handle ENTER pressed before comma or semicolon
      , tryIndentAfterSomeKeywords_ch                       // NOTE It must follow after trySplitComment_ch!
      , tryAfterDanglingSemicolon_ch
      , tryMacroDefinition_ch
      , tryBeforeDanglingDelimiter_ch
      , tryBeforeAccessSpecifier_ch
      , tryAfterEqualChar_ch
      , tryPreprocessor_ch
      , tryAfterBlockComment_ch
      , tryAfterBreakContinue_ch
      , trySplitString_ch                                   // Handle ENTER pressed in the middle of a string
      , tryAfterLambda_ch                                   // Handle ENTER after lambda prototype and before body
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
 * \li if there is a <tt>'// '</tt> string right before just entered \c '/', form a
 *     doxygen comment <tt>'///'</tt> or <tt>'///<'</tt> depending on presence of some text
 *     on a line before the comment.
 *
 * \todo Due the BUG #316809 in a current version of Kate, this code doesn't work as expected!
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
 */
function trySameLineComment(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    // First of all check that we are not withing a string
    if (document.isString(line, column))
        return;

    var sc = splitByComment(line);
    if (sc.hasComment)                                      // Is there any comment on a line?
    {
        // Make sure we r not in a comment already
        if (document.isComment(line, document.firstColumn(line)) && (document.line(line) != '///'))
            return;
        // If no text after the comment and it still not aligned
        var text_len = sc.before.rtrim().length;
        if (text_len != 0 && sc.after.length == 0 && text_len < gSameLineCommentStartAt)
        {
            // Align it!
            document.insertText(
                line
              , column - 2
              , String().fill(' ', gSameLineCommentStartAt - sc.before.length)
              );
            document.insertText(line, gSameLineCommentStartAt + 2, ' ');
        }
        // If text in a comment equals to '/' or ' /' -- it looks like a 3rd '/' pressed
        else if (sc.after == " /" || sc.after == "/")
        {
            // Form a Doxygen comment!
            document.removeText(line, column - sc.after.length, line, column);
            document.insertText(line, column - sc.after.length, text_len != 0 ? "/< " : "/ ");
        }
        // If right trimmed text in a comment equals to '/' -- it seems user moves cursor
        // one char left (through space) to add one more '/'
        else if (sc.after.rtrim() == "/")
        {
            // Form a Doxygen comment!
            document.removeText(line, column, line, column + sc.after.length);
            document.insertText(line, column, text_len != 0 ? "< " : " ");
        }
        else if (text_len == 0 && sc.after.length == 0)
        {
            document.insertText(line, column, ' ');
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
 *     possible at the line start, so it must be half indented
 * \li shortcut: <tt>some(<|)</tt> transformed into <tt>some()<|</tt>
 *
 * But, do not add '>' if there some text after cursor.
 */
function tryTemplate(cursor)
{
    var line = cursor.line;
    var column = cursor.column;
    var result = -2;

    if (isStringOrComment(line, column))
        return result;                                      // Do nothing for comments and strings

    // Check for 'template' keyword at line start
    var currentString = document.line(line);
    var prevWord = document.wordAt(line, column - 1);
    dbg("tryTemplate: prevWord='"+prevWord+"'");
    dbg("tryTemplate: prevWord.match="+prevWord.match(/\b[A-Za-z_][A-Za-z0-9_]*/));
    // Add a closing angle bracket if a prev word is not a 'operator'
    // and it looks like an identifier or current line starts w/ 'template' keyword
    var isCloseAngleBracketNeeded = (prevWord != "operator")
      && (currentString.match(/^\s*template\s*<$/) || prevWord.match(/\b[A-Za-z_][A-Za-z0-9_]*/))
      && (column == document.lineLength(line) || document.charAt(cursor).match(/\W/))
      ;
    if (isCloseAngleBracketNeeded)
    {
        document.insertText(cursor, ">");
        view.setCursorPosition(cursor);
    }
    else if (justEnteredCharIsFirstOnLine(line, column, '<'))
    {
        result = tryIndentRelativePrevNonCommentLine(line);
    }
    // Add a space after 2nd '<' if a word before is not a 'operator'
    else if (document.charAt(line, column - 2) == '<')
    {
        if (document.wordAt(line, column - 3) != "operator")
        {
            // Looks like case 3...
            // 0) try to remove '>' if user typed 'some<' before
            // (and closing '>' was added by tryTemplate)
            if (column < document.lineLength(line) && document.charAt(line, column) == '>')
            {
                document.removeText(line, column, line, column + 1);
                addCharOrJumpOverIt(line, column - 2, ' ');
                view.setCursorPosition(line, column + 1);
                column = column + 1;
            }
            // add a space after operator<<
            document.insertText(line, column, " ");
        }
        else
        {
            document.insertText(line, column, "()");
            view.setCursorPosition(line, column + 1);
        }
    }
    else
    {
        cursor = tryJumpOverParenthesis(cursor);            // Try to jump out of parenthesis
        tryAddSpaceAfterClosedBracketOrQuote(cursor);
    }
    return result;
}

/**
 * This function called for some characters and trying to do the following:
 * if the cursor (right after a trigger character is entered) is positioned withing
 * a parenthesis, move the entered character out of parenthesis.
 *
 * For example:
 * \code
 *  auto a = two_params_func(get_first(,|))
 *  // ... transformed into
 *  auto a = two_params_func(get_first(),|)
 * \endcode
 *
 * because entering comma right after <tt>(</tt> definitely incorrect, but
 * we can help the user (programmer) to avoid 3 key presses ;-)
 * (RightArrow, ',', space)
 *
 * except comma here are other "impossible" characters:
 * \c ., \c ?, \c :, \c %, \c |, \c /, \c =, \c <, \c >, \c ], \c }
 *
 * But \c ; will be handled separately to be able to jump over all closing \c ).
 *
 * \sa \c trySemicolon()
 *
 * \note This valid if we r not inside a comment or a string literal,
 * and the char out of the parenthesis is not the same as just entered ;-)
 *
 * \param cursor initial cursor position
 * \param es edit session instance
 * \return new (possible modified) cursor position
 *
 * \attention This function \b never calls \c editEnd() for a given \c es instance!
 */
function tryJumpOverParenthesis(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    if (2 < column && isStringOrComment(line, column))
        return cursor;                                      // Do nothing for comments of string literals

    // Check that we r inside of parenthesis and some symbol between
    var pc = document.charAt(line, column - 2);
    var cc = document.charAt(cursor);
    if ((pc == '(' && cc == ')') || (pc == '{' && cc == '}'))
    {
        var c = document.charAt(line, column - 1);
        switch (c)
        {
            case '.':
                if (pc == '(' && cc == ')')
                {
                    // Make sure this is not a `catch (...)`
                    if (document.startsWith(line, "catch (.)", true))
                    {
                        document.insertText(line, column, "..");
                        view.setCursorPosition(line, column + 3);
                        break;
                    }
                }
            case ',':
            case '?':
            case ':':
            case '%':
            case '^':
            case '|':
            case '/':                                       // TODO ORLY?
            case '=':
            case '<':
            case '>':
            case '}':
            case ')':
            case ']':                                       // NOTE '[' could be a part of lambda
            {
                // Ok, move character out of parenthesis
                document.removeText(line, column - 1, line, column);
                // Check is a character after the closing brace the same as just entered one
                addCharOrJumpOverIt(line, column, c);
                return view.cursorPosition();
            }
            default:
                break;
        }
    }
    return cursor;
}

/**
 * Handle the case when some character was entered after a some closing bracket.
 * Here is few close brackets possible:
 * \li \c ) -- ordinal function call
 * \li \c } -- C++11 constructor call
 * \li \c ] -- array access
 * \li \c " -- end of a string literal
 * \li \c ' -- and of a char literal
 *
 * This function try to add a space between a closing quote/bracket and operator char.
 *
 * \note This valid if we r not inside a comment or a string literal.
 */
function tryAddSpaceAfterClosedBracketOrQuote(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    if (isStringOrComment(line, column - 1))
        return cursor;                                      // Do nothing for comments of string literals

    // Check if we have a closing bracket before a last entered char
    var b = document.charAt(line, column - 2);
    if (!(b == ']' || b == '}' || b == ')' || b == '"' || b == "'"))
        return cursor;

    // Ok, lets check what we've got as a last char
    var c = document.charAt(line, column - 1);
    dbg("tryAddSpaceAfterClosedBracketOrQuote: c='"+c+"', @"+new Cursor(line, column-1));
    switch (c)
    {
        case '*':
        case '/':
        case '%':
        case '&':
        case '|':
        case '=':
        case '^':
        case '?':
        case ':':
        case '<':
            document.insertText(line, column - 1, " ");
            view.setCursorPosition(line, column + 1);
            return view.cursorPosition();
        case '>':
            // Close angle bracket may be a part of template instantiation
            // w/ some function type parameter... Otherwise, add a space after.
            if (b != ')')
            {
                document.insertText(line, column - 1, " ");
                view.setCursorPosition(line, column + 1);
                return view.cursorPosition();
            }
            break;
        default:
            break;
    }
    return cursor;
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
    // Check is comma a very first character on a line...
    if (justEnteredCharIsFirstOnLine(line, column, ','))
        result = tryIndentRelativePrevNonCommentLine(line);

    cursor = tryJumpOverParenthesis(cursor);                // Try to jump out of parenthesis
    if (document.charAt(cursor) != ' ')
        document.insertText(cursor, " ");                   // Add space only if not present
    else
        view.setCursorPosition(line, column + 1);           // Otherwise just move cursor after it
    return result;
}

/**
 * \brief Move towards a document start and look for control flow keywords
 *
 * \note Keyword must be at the line start
 *
 * \return found line's indent, otherwise \c -2 if nothing found
 */
function tryBreakContinue(line, is_break)
{
    var result = -2;
    // Ok, look backward and find a loop/switch statement
    for (; 0 <= line; --line)
    {
        var text = document.line(line).ltrim();
        var is_loop_or_switch = text.startsWith("for ")
          || text.startsWith("do ")
          || text.startsWith("while ")
          || text.startsWith("if ")
          || text.startsWith("else if ")
          ;
        if (is_break)
            is_loop_or_switch =  is_loop_or_switch
              || text.startsWith("case ")
              || text.startsWith("default:")
              ;
        if (is_loop_or_switch)
            break;
    }
    if (line != -1)                                     // Found smth?
        result = document.firstColumn(line) + gIndentWidth;

    return result;
}

/**
 * \brief Handle \c ; character.
 *
 * Here is few cases possible (handled):
 * \li semicolon is a first char on a line -- then, it looks like \c for statement
 *      splitted accross the lines
 * \li semicolon entered after some keywords: \c break or \c continue, then we
 *      need to align this line taking in account a previous one
 * \li and finally here is a trick: when auto brackets extension enabled, and user types
 *      a function call like this:
 * \code
 *  auto var = some_call(arg1, arg2|)
 * \endcode
 * (\c '|' shows a cursor position). Note there is no final semicolon in this expression,
 * cuz pressing <tt>'('</tt> leads to the following snippet: <tt>some_call(|)</tt>, so to
 * add a semicolon you have to move cursor out of parenthesis. The trick is to allow to press
 * <tt>';'</tt> at position shown in the code snippet, so indenter will transform it into this:
 * \code
 *  auto var = some_call(arg1, arg2);|
 * \endcode
 * same works even there is no arguments...
 *
 * All the time, when simicolon is not a first non-space symbol (and not a part of a comment
 * or string) it will be stiked to the last non-space character on the line.
 */
function trySemicolon(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    if (isStringOrComment(line, column))
        return result;                                      // Do nothing for comments and strings

    // If ';' is a first char on a line?
    if (justEnteredCharIsFirstOnLine(line, column, ';'))
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
    else
    {
        // Stick ';' to the last "word"
        var lcsc = document.prevNonSpaceColumn(line, column - 2);
        if (2 < column && lcsc < (column - 2))
        {
            document.removeText(line, column - 1, line, column);
            if (document.charAt(line, lcsc) != ';')
            {
                document.insertText(line, lcsc + 1, ";");
                view.setCursorPosition(line, lcsc + 2);
            }
            else view.setCursorPosition(line, lcsc + 1);
            cursor = view.cursorPosition();
            column = cursor.column;
        }
        var text = document.line(line).ltrim();
        var is_break = text.startsWith("break;");
        var should_proceed = is_break || text.startsWith("continue;")
        if (should_proceed)
        {
            result = tryBreakContinue(line - 1, is_break);
            if (result == -2)
                result = -1;
        }
        // Make sure we r not inside of `for' statement
        /// \todo Make sure cursor is really inside of \c for and
        /// not smth like this: <tt>for (blah; blah; blah) some(arg,;|)</tt>
        else if (!text.startsWith("for "))
        {
            // Check if next character(s) is ')' and nothing after
            should_proceed = true;
            var lineLength = document.lineLength(line);
            for (var i = column; i < lineLength; ++i)
            {
                var c = document.charAt(line, i);
                if (!(c == ')' || c == ']'))
                {
                    should_proceed = false;
                    break;
                }
            }
            // Ok, lets move ';' out of "a(b(c(;)))" of any level...
            if (should_proceed)
            {
                // Remove ';' from column - 1
                document.removeText(line, column - 1, line, column);
                // Append ';' to the end of line
                document.insertText(line, lineLength - 1, ";");
                view.setCursorPosition(line, lineLength);
                cursor = view.cursorPosition();
                column = cursor.column;
            }
        }
        // In C++ there is no need to have more than one semicolon.
        // So remove a redundant one!
        if (document.charAt(line, column - 2) == ';')
        {
            // Remove just entered ';'
            document.removeText(line, column - 1, line, column);
        }
    }
    return result;
}

/**
 * Handle possible dangling operators (moved from a previous line)
 *
 * \c ?, \c |, \c ^, \c %, \c .
 *
 * Add spaces around ternary operator.
 */
function tryOperator(cursor, ch)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    if (isStringOrComment(line, column))
        return result;                                      // Do nothing for comments and strings

    var halfTabNeeded = justEnteredCharIsFirstOnLine(line, column, ch)
      && document.line(line - 1).search(/^\s*[A-Za-z_][A-Za-z0-9_]*/) != -1
      ;
    dbg("tryOperator: halfTabNeeded =", halfTabNeeded);
    if (halfTabNeeded)
    {
        // check if we r at function call or array index
        var insideBraces = document.anchor(line, document.firstColumn(line), '(').isValid()
          || document.anchor(line, document.firstColumn(line), '[').isValid()
          || document.anchor(line, document.firstColumn(line), '{').isValid()
          ;
        dbg("tryOperator: insideBraces =",insideBraces);
        result = document.firstColumn(line - 1) + (insideBraces && ch != '.' ? -2 : 2);
    }
    var prev_pos = cursor;
    cursor = tryJumpOverParenthesis(cursor);                // Try to jump out of parenthesis
    cursor = tryAddSpaceAfterClosedBracketOrQuote(cursor);

    // Check if a space before '?' still needed
    if (prev_pos == cursor && ch == '?' && document.charAt(line, cursor.column - 1) != ' ')
        document.insertText(line, cursor.column - 1, " ");  // Add it!

    cursor = view.cursorPosition();                         // Update cursor position
    line = cursor.line;
    column = cursor.column;

    if (ch == '?')
    {
        addCharOrJumpOverIt(line, column, ' ');
    }
    // Handle operator| and/or operator||
    else if (ch == '|')
    {
        /**
         * Here is 6+3 cases possible (the last bar is just entered):
         * 0) <tt>???</tt> -- add a space before bar and after if needed
         * 1) <tt>?? </tt> -- add a space after if needed
         * 2) <tt>??|</tt> -- add a space before 1st bar and after the 2nd if needed
         * 3) <tt>? |</tt> -- add a space after the 2nd bar if needed
         * 4) <tt>?| </tt> -- add a space before 1st bar, remove the mid one, add a space after 2nd bar
         * 5) <tt> | </tt> -- remove the mid space, add one after 2nd bar
         * and finally,
         * 6a) <tt>|| </tt> -- add a space before 1st bar if needed, remove the last bar
         * 6b) <tt> ||</tt> -- remove the last bar and add a space after 2nd bar if needed
         * 6c) <tt>||</tt> -- add a space after if needed
         */
        var prev = document.text(line, column - 4, line, column - 1);
        dbg("tryOperator: checking @Cursor("+line+","+(column - 4)+"), prev='"+prev+"'");
        var space_offset = 0;
        if (prev.endsWith(" | "))
        {
            // case 5: remove the mid space
            document.removeText(line, column - 2, line, column - 1);
            space_offset = -1;
        }
        else if (prev.endsWith("|| "))
        {
            // case 6a: add a space before 1st bar if needed, remove the last bar
            document.removeText(line, column - 1, line, column);
            var space_has_added = addCharOrJumpOverIt(line, column - 4, ' ');
            space_offset = (space_has_added ? 1 : 0) - 2;
        }
        else if (prev.endsWith(" ||"))
        {
            // case 6b: remove the last bar
            document.removeText(line, column - 1, line, column);
            space_offset = -1;
        }
        else if (prev.endsWith("||"))
        {
            // case 6a: add a space before and remove the last bar
            document.removeText(line, column - 1, line, column);
            document.insertText(line, column - 3, " ");
        }
        else if (prev.endsWith("| "))
        {
            // case 4: add a space before 1st bar, remove the mid one
            document.removeText(line, column - 2, line, column - 1);
            document.insertText(line, column - 3, " ");
        }
        else if (prev.endsWith(" |") || prev.endsWith(" "))
        {
            // case 3 and 1
        }
        else
        {
            // case 2: add a space before 1st bar
            if (prev.endsWith('|'))
                space_offset = 1;
            // case 0: add a space before bar
            document.insertText(line, column - 1 - space_offset, " ");
            space_offset = 1;
        }
        addCharOrJumpOverIt(line, column + space_offset, ' ');
    }
    // Handle operator% and/or operator^
    else if (ch == '%' || ch == '^')
    {
        var prev = document.text(line, column - 4, line, column - 1);
        dbg("tryOperator: checking2 @Cursor("+line+","+(column - 4)+"), prev='"+prev+"'");
        var patterns = [" % ", "% ", " %", "%", " "];
        for (
            var i = 0
          ; i < patterns.length
          ; i++
          ) patterns[i] = patterns[i].replace('%', ch);

        var space_offset = 0;
        if (prev.endsWith(patterns[0]))
        {
            // case 0: remove just entered char
            document.removeText(line, column - 1, line, column);
            space_offset = -2;
        }
        else if (prev.endsWith(patterns[1]))
        {
            // case 1: remove just entered char, add a space before
            document.removeText(line, column - 1, line, column);
            document.insertText(line, column - 3, " ");
            space_offset = -1;
        }
        else if (prev.endsWith(patterns[2]))
        {
            // case 2: remove just entered char
            document.removeText(line, column - 1, line, column);
            space_offset = -1;
        }
        else if (prev.endsWith(patterns[3]))
        {
            // case 3: add a space before
            document.removeText(line, column - 1, line, column);
            document.insertText(line, column - 2, " ");
            space_offset = 0;
        }
        else if (prev.endsWith(patterns[4]))
        {
            // case 4: no space needed before
            space_offset = 0;
        }
        else
        {
            // case everything else: surround operator w/ spaces
            document.insertText(line, column - 1, " ");
            space_offset = 1;
        }
        addCharOrJumpOverIt(line, column + space_offset, ' ');
    }
    else if (ch == '.')                                     // Replace '..' w/ '...'
    {
        var prev = document.text(line, column - 3, line, column);
        dbg("tryOperator: checking3 @Cursor("+line+","+(column - 4)+"), prev='"+prev+"'");
        if (prev == "...")                                  // If there is already 3 dots
        {
            // Remove just entered (redundant) one
            document.removeText(line, column - 1, line, column);
        }
        else if (prev[1] == '.' && prev[2] == '.')          // Append one more if only two here
        {
            addCharOrJumpOverIt(line, column, '.');
        }                                                   // Otherwise, do nothing...
    }
    if (result != -2)
    {
        dbg("tryOperator result="+result);
    }
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

    var braceCursor = Cursor.invalid();
    if (ch != '>')
    {
        // TODO Make sure a given `ch` in the gBraceMap
        braceCursor = document.anchor(line, column - 1, gBraceMap[ch]);
        // TODO Otherwise, it seems we have a template parameters list...
    }

    // Check if a given closing brace is a first char on a line
    // (i.e. it is 'dangling' brace)...
    if (justEnteredCharIsFirstOnLine(line, column, ch) && braceCursor.isValid())
    {
        // Move to one half-TAB right, if anything but not closing '}', else
        // align to the corresponding open char
        result = document.firstColumn(braceCursor.line) + (ch != '}' ? 2 : 0);
        dbg("tryCloseBracket: setting result="+result);
    }

    // Check if ';' required after closing '}'
    if (ch == '}' && braceCursor.isValid())
    {
        var is_check_needed = false;
        // Check if corresponding anchor is a class/struct/union/enum,
        // (possible keyword located on same or prev line)
        // and check for trailing ';'...
        var anchoredString = document.line(braceCursor.line);
        dbg("tryCloseBracket: anchoredString='"+anchoredString+"'");
        var regex = /^(\s*)(class|struct|union|enum).*$/;
        var r = regex.exec(anchoredString);
        if (r != null)
        {
            dbg("tryCloseBracket: same line");
            is_check_needed = true;
        }
        else (!is_check_needed && 0 < braceCursor.line)     // Is there any line before?
        {
            dbg("tryCloseBracket: cheking prev line");

            // Ok, lets check it!
            anchoredString = document.line(braceCursor.line - 1);
            dbg("tryCloseBracket: anchoredString-1='"+anchoredString+"'");
            r = regex.exec(anchoredString);
            if (r != null)
            {
                is_check_needed = true;
                dbg("tryCloseBracket: prev line");
            }
        }
        dbg("tryCloseBracket: is_check_needed="+is_check_needed);
        if (is_check_needed)
        {
            var is_ok = document.line(line)
              .substring(column, document.lineLength(line))
              .ltrim()
              .startsWith(';')
              ;
            if (!is_ok)
            {
                document.insertText(line, column, ';');
                view.setCursorPosition(line, column + 1);
            }
        }
    }
    else if (ch == '>')
    {
        // If user typed 'some' + '<' + '>', jump over the '>'
        // (which was added by the tryTemplate)
        if (document.charAt(line, column) == '>')
        {
            document.removeText(line, column, line, column + 1);
        }
    }

    tryJumpOverParenthesis(view.cursorPosition());

    return result;
}

/**
 * \brief Indent a new scope block
 *
 * ... try to unindent to be precise... First of all check that open
 * \c '{' is a first symbol on a line, and if it doesn't,
 * add space (if absent at previous position) after <tt>')'</tt> or \c '='
 * or if line stats w/ some keywords: \c enum, \c class, \c struct or \c union.
 * Otherwise, look at the previous line for dangling <tt>')'</tt> or
 * a line started w/ one of flow control keywords.
 *
 */
function tryBlock(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    // Make sure we r not in a comment or string
    dbg("tryBlock: isStringOrComment(line, column - 2)="+isStringOrComment(line, column - 2))
    if (isStringOrComment(line, column - 2))
        return result;

    if (justEnteredCharIsFirstOnLine(line, column, '{'))
    {
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
    }
    else
    {
        // '{' is not a first char. Check for a previous one...
        if (1 < column)
        {
            var prevChar = document.charAt(line, column - 2);
            dbg("tryBlock: prevChar='"+prevChar+"'");
            if (prevChar == ')' || prevChar == '=')
                document.insertText(line, column - 1, ' ');
            else if (prevChar != ' ')
            {
                var currentLine = document.line(line).ltrim();
                var starts_with_keyword = currentLine.startsWith('struct ')
                  || currentLine.startsWith('class ')
                  || currentLine.startsWith('union ')
                  || currentLine.startsWith('enum ')
                  ;
                if (starts_with_keyword)
                    document.insertText(line, column - 1, ' ');
            }
        }
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
    if (justEnteredCharIsFirstOnLine(line, column, '#'))
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
 *     list or 2nd line of ternary operator.
 * \li \c ':' is pressed on a line started w/ \c for statement and after a space
 * \li \c ':' after '&gt;' looks like an access to template's member
 * \li shortcut: transform <tt>some(:|)</tt> into <tt>some() :|</tt>
 *
 * \todo Should it be done only for non strings and comments?
 */
function tryColon(cursor)
{
    var result = -2;
    var line = cursor.line;
    var column = cursor.column;

    if (isStringOrComment(line, column))
        return result;                                      // Do nothing for comments and strings

    // Check if just entered ':' is a first on a line
    if (justEnteredCharIsFirstOnLine(line, column, ':'))
    {
        // Check if there a dangling ')' or '?' (ternary operator) on a previous line
        var ch = document.firstChar(line - 1);
        if (ch == ')' || ch == '?')
            result = document.firstVirtualColumn(line - 1);
        else
            result = document.firstVirtualColumn(line - 1) + 2;
        document.insertText(cursor, " ");
    }
    else
    {
        var currentLine = document.line(line);
        if (currentLine.search(/^\s*((public|protected|private)\s*(slots|Q_SLOTS)?|(signals|Q_SIGNALS)\s*):\s*$/) != -1)
        {
            var definitionCursor = document.anchor(line, 0, '{');
            if (definitionCursor.isValid())
            {
                result = document.firstVirtualColumn(definitionCursor.line);
                dbg("tryColon: result="+result);
                if (0 < line)                               // Is there any line before?
                {
                    // Check if previous line is not empty and not starts w/ '{'
                    var prevLine = document.line(line - 1).trim()
                    if (prevLine.length && !prevLine.startsWith("{"))
                    {
                        // Cuz a new line will be added in place of current, returning
                        // result will not affect indentation. So do it manually.
                        var firstColumn = document.firstColumn(line);
                        var padding = "";
                        if (firstColumn < result)
                            padding = String().fill(' ', result - firstColumn);
                        else if (result < firstColumn)
                            document.removeText(line, 0, line, firstColumn - result);
                        // Add an empty line before the current
                        document.insertText(line, 0, "\n" + padding);
                        result = 0;
                    }
                }
            }
        }
        else if (document.charAt(line, column - 2) == ' ')
        {
            // Is it looks like a range based `for' or class/struct/enum?
            var add_space = currentLine.ltrim().startsWith("for (")
              || currentLine.ltrim().startsWith("class ")
              || currentLine.ltrim().startsWith("struct ")
              || currentLine.ltrim().startsWith("enum ")
              ;
            if (add_space)
            {
                // Add a space after ':'
                document.insertText(line, column, " ");
            }
            else if (document.charAt(line, column - 3) == ':')
            {
                // Transform ': :' -> '::'
                document.removeText(line, column - 2, line, column - 1);
            }
        }
        else if (document.charAt(line, column - 2) == ':')
        {
            // A char before is (already) a one more colon.
            // Make sure there is no more than two colons...
            // NOTE In C++ it is not possible to have more than two of them adjacent!
            if (document.charAt(line, column - 3) == ':')
            {
                // Remove the current (just entered) one...
                document.removeText(line, column - 1, line, column);
            }
        }
        else
        {
            // Check that it is not a 'case' and not a magic sequence.
            // NOTE "Magic sequence" means support for dynamic expand functions.
            // http://zaufi.github.io/programming/2014/02/13/kate-c++-stuff/
            var is_magic_sequence = document.charAt(
                line
              , document.wordRangeAt(line, column - 1).start.column - 1
              ) == ';';
            if (!currentLine.ltrim().startsWith("case ") && !is_magic_sequence)
            {
                // Add one more ':'
                // Example some<T>: --> some<T>:: or std: --> std::
                document.insertText(line, column, ":");
            }
            else
            {
                // Try to jump out of parenthesis
                cursor = tryJumpOverParenthesis(cursor);
                // Try add a space after close bracket
                tryAddSpaceAfterClosedBracketOrQuote(cursor);
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
    if (wordBefore.search(/\b(catch|for|if|switch|while|return)\b/) != -1)
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
        for (var i = result.range.start.line; i < result.range.end.line; ++i)
        {
            var currentLineText = document.line(i);
            var originalTextLength = currentLineText.length;
            currentLineText = currentLineText.substring(0, currentLineText.length - 1).rtrim();
            var textLength = currentLineText.length;
            document.removeText(i, textLength, i, originalTextLength);
            document.insertText(i, textLength, String().fill(' ', result.max - textLength + 1) + "\\");
        }
    }
}

/**
 * \brief Handle a <tt>@</tt> symbol
 *
 * Possible user wants to add a Doxygen group
 */
function tryDoxygenGrouping(cursor)
{
    var line = cursor.line;
    var column = cursor.column;
    var firstColumn = document.firstColumn(line);
    // Check the symbol before the just entered
    var looks_like_doxgorup = isStringOrComment(line, column - 2)
      && firstColumn == (column - 4)
      && document.line(line).ltrim().startsWith("// ")
      ;
    if (looks_like_doxgorup)
    {
        document.removeText(line, column - 2, line, column - 1);
        var padding = String().fill(' ', firstColumn);
        document.insertText(line, column - 1, "{\n" + padding + "\n" + padding + "//@}");
        view.setCursorPosition(line + 1, document.lineLength(line + 1));
    }
}

/**
 * \brief Handle quote character
 *
 * Look back for \c 'R' char right before \c '"' and if
 * the next one (after \c '"') is not an alphanumeric,
 * then add a delimiters.
 *
 * \attention Effect of AutoBrace extension has already neutralized at this point :)
 */
function tryStringLiteral(cursor, ch)
{
    var line = cursor.line;
    var column = cursor.column;

    if (isComment(line, column - 2))                        // Do nothing for comments
        return;

    // First of all we have to determinate where we are:
    // 0) new string literal just started, or ...
    // 1) string literal just ends

    // Check if the '"' is a very first char on a line
    var new_string_just_started;
    if (column < 2)
        // Yes, then we have to look to the last char of the previous line
        new_string_just_started = !(line != 0 && isString(line - 1, document.lastColumn(line - 1)));
    else
        // Ok, just check attribute of the char right before '"'
        new_string_just_started = !isString(line, column - 2);

    // TODO Add a space after possible operator right before just
    // started string literal...
    if (new_string_just_started)
    {
        // Is there anything after just entered '"'?
        var nc = document.charAt(line, column);
        var need_closing_quote = column == document.lineLength(line)
          || document.isSpace(nc)
          || nc == ','                                      // user tries to add new string param,
          || nc == ')'                                      // ... or one more param to the end of some call
          || nc == ']'                                      // ... or string literal as subscript index
          || nc == ';'                                      // ... or one more string before end of expression
          || nc == '<'                                      // ... or `some << "|<<`
          ;
        if (need_closing_quote)
        {
            // Check for 'R' right before '"'
            if (ch == '"' && document.charAt(line, column - 2) == 'R')
            {
                // Yeah, looks like a raw string literal
                /// \todo Make delimiter configurable... HOW?
                /// It would be nice if indenters can have a configuration page somehow...
                document.insertText(cursor, "~()~\"");
                view.setCursorPosition(line, column + 2);
            }
            else
            {
                document.insertText(cursor, ch);
                view.setCursorPosition(line, column);
            }
        }
    }
}

/**
 * \brief Handle \c '!' char
 *
 * Exclamation symbol can be a part of \c operator!= or unary operator.
 * in both cases, a space required before it! Except few cases:
 * - when it is at the line start
 * - when a char before it \c '(' -- i.e. argument of a control flow keyword (\c if, \c while)
 *   or a function call parameter
 * - when a char before it \c '[' (array subscript)
 * - when a char before it \c '<' -- here is two cases possible:
 *      - it is a first non-type template parameter (w/ type \c bool obviously)
 *      - it is a part of less or shift operators.
 * To distinct last case, it is enough to check that a word before \c '<' (w/o space)
 * is an identifier.
 * \note Yep, operators supposed to be separated from around text.
 */
function tryExclamation(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    if (column == 0)                                        // Do nothing for very first char
        return;

    if (isStringOrComment(line, column - 1))                // Do nothing for comments and stings
        return;

    if (document.firstColumn(line) == column - 1)           // Make sure '!' is not a first char on a line
        return;

    if (column < 2)                                         // Do nothing if there is less than 2 chars before
        return;

    var pc = document.charAt(line, column - 2);             // Do nothing if one of 'stop' chars:
    if (pc == ' ' || pc == '(' || pc == '[' || pc == '{')
        return;

    // And finally make sure it is not a part of 'relation operator'
    if (pc == '<' && column >= 3)
    {
        // Make sure a char before is not a space or another '<'
        var ppc = document.charAt(line, column - 3);
        if (ppc != ' ' && ppc != '<')
            return;
    }

    // Ok, if we r here, just insert a space ;)
    document.insertText(line, column - 1, " ");
}

/**
 * \brief Handle a space
 *
 * - add <tt>'()'</tt> pair after some keywords like: \c if, \c while, \c for, \c switch
 * - add <tt>';'</tt> if space pressed right after \c return, and no text after it
 * - if space pressed inside of angle brackets 'some<|>' transform into 'some < |'
 */
function tryKeywordsWithBrackets(cursor)
{
    var line = cursor.line;
    var column = cursor.column;
    var text = document.line(line).ltrim();
    var need_brackets = text == "if "
      || text == "else if "
      || text == "while "
      || text == "for "
      || text == "switch "
      || text == "catch "
      ;
    if (need_brackets)
    {
        document.insertText(cursor, "()");
        view.setCursorPosition(line, column + 1);
    }
    else if (text == "return ")
    {
        document.insertText(cursor, ";");
        view.setCursorPosition(line, column);
    }
    else if (document.charAt(line, column - 2) == '<' && document.charAt(cursor) == '>')
    {
        document.removeText(line, column, line, column + 1);
        document.insertText(line, column - 2, " ");
    }
}

/**
 * Try to add space before, after some equal operators.
 */
function tryEqualOperator(cursor)
{
    var line = cursor.line;
    var column = cursor.column;

    // Do nothing for comments or string literals or lines shorter than 2
    if (2 < column && isStringOrComment(line, column))
        return cursor;

    var c = document.charAt(line, column - 2);
    dbg("tryEqualOperator: checking @Cursor("+line+","+(column - 2)+"), c='"+c+"'");

    switch (c)
    {
        // Two chars operators: !=, ==
        case '*':
        case '%':
        case '/':
        case '^':
        case '|':
        case '&':
        case '!':
        case '=':
            addCharOrJumpOverIt(line, column, ' ');         // Make sure there is a space after it!
            // Make sure there is a space before it!
            if (column >= 3 && document.charAt(line, column - 3) != ' ')
                document.insertText(line, column - 2, " ");
            break;
        case '(':                                           // some(=|) --> some() =|
            cursor = tryJumpOverParenthesis(cursor);
            tryEqualOperator(cursor);                       // Call self again to handle "some()=|"
            break;
        case ')':                                           // "some()=" or "(expr)=" --> ") =|"
        case '}':                                           // It can be a ctor of some proxy object
            // Add a space between closing bracket and just entered '='
            document.insertText(line, column - 1, " ");
            break;
        case '<':
            // Shortcut: transfrom "some<=|>" -> "some <= |"
            if (document.charAt(cursor) == '>')
                document.removeText(line, column, line, column + 1);
        case '>':
            // This could be '<<=', '>>=', '<=', '>='
            // Make sure there is a space after it!
            addCharOrJumpOverIt(line, column, ' ');         // Make sure there is a space after it!
            // Check if this is one of >>= or <<=
            if (column >= 3)
            {
                if (document.charAt(line, column - 3) == c)
                {
                    if (column >= 4 && document.charAt(line, column - 4) != ' ')
                        document.insertText(line, column - 3, " ");
                }
                else if (document.charAt(line, column - 3) != ' ')
                {
                    // <= or >=
                    document.insertText(line, column - 2, " ");
                }
            }
            break;
        case '[':                                           // This could be a part of lambda capture [=]
            break;
        case ' ':
            // Lookup one more character towards left
            if (column >= 3)
            {
                var pc = document.charAt(line, column - 3);
                dbg("tryEqualOperator: checking @Cursor("+line+","+(column - 3)+"), pc='"+pc+"'");
                switch (pc)
                {
                    case '=':                               // Stick the current '=' to the previous char
                    case '|':
                    case '&':
                    case '^':
                    case '<':
                    case '>':
                    case '*':
                    case '/':
                    case '%':
                        document.removeText(line, column - 1, line, column);
                        document.insertText(line, column - 2, '=');
                        break;
                    default:
                        break;
                }
            }
            break;
        case '+':
        case '-':
            addCharOrJumpOverIt(line, column, ' ');         // Make sure there is a space after it!
            // Here is few things possible:
            // some+=| --> some += |
            // some++=| --> some++ = |
            // some+++=| --> some++ += |
            var space_offset = -1;
            if (column >= 3)
            {
                if (document.charAt(line, column - 3) == c)
                {
                    if (column >= 4)
                    {
                        if (document.charAt(line, column - 4) == c)
                            space_offset = 2;
                        else if (document.charAt(line, column - 4) != ' ')
                            space_offset = 1;
                    }
                }
                else if (document.charAt(line, column - 3) != ' ')
                    space_offset = 2;
            }
            if (space_offset != -1)
                document.insertText(line, column - space_offset, " ");
            break;
        default:
            dbg("tryEqualOperator: default");
            // '=' always surrounded by spaces!
            addCharOrJumpOverIt(line, column, ' ');         // Make sure there is a space after it!
            document.insertText(line, column - 1, " ");     // Make sure there is a space before it!
            break;
    }
}

/**
 * \brief Process one character
 *
 * NOTE Cursor positioned right after just entered character and has +1 in column.
 *
 * \attention This function will roll back the effect of \b AutoBrace extension
 * for quote chars. So this indenter can handle that chars withing predictable
 * surround...
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

    document.editBegin();
    // Check if char under cursor is the same as just entered,
    // and if so, remove it... to make it behave like "overwrite" mode
    if (ch != ' ' && document.charAt(cursor) == ch)
        document.removeText(line, cursor.column, line, cursor.column + 1);

    switch (ch)
    {
        case '\n':
            result = caretPressed(cursor);
            break;
        case '/':
            trySameLineComment(cursor);                     // Possible user wants to start a comment
            break;
        case '<':
            result = tryTemplate(cursor);                   // Possible need to add closing '>' after template
            break;
        case ',':
            result = tryComma(cursor);                      // Possible need to align parameters list
            break;
        case ';':
            result = trySemicolon(cursor);                  // Possible `for ()` loop spread over lines
            break;
        case '?':
        case '|':
        case '^':
        case '%':
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
            tryOpenBrace(cursor);                           // Try to add a space after some keywords
            break;
        case '\\':
            tryBackslash(cursor);
            break;
        case '@':
            tryDoxygenGrouping(cursor);
            break;
        case '"':
        case '\'':
            tryStringLiteral(cursor, ch);
            break;
        case '!':                                           // Almost all the time there should be a space before!
            tryExclamation(cursor);
            break;
        case ' ':
            tryKeywordsWithBrackets(cursor);
            break;
        case '=':
            tryEqualOperator(cursor);
            break;
        case '*':
        case '&':
            tryAddSpaceAfterClosedBracketOrQuote(cursor);
            break;
        default:
            break;                                          // Nothing to do...
    }

    // Make sure it is not a pure comment line
    var currentLineText = document.line(cursor.line).ltrim();
    if (ch != '\n' && !currentLineText.startsWith("//"))
    {
        // Ok, try to keep an inline comment aligned (if any)...
        // BUG If '=' was inserted (and a space added) in a code line w/ inline comment,
        // it seems kate do not update highlighting, so position, where comment was before,
        // still counted as a 'Comment' attribute, but actually it should be 'Normal Text'...
        // It is why adding '=' will not realign an inline comment...
        if (alignInlineComment(cursor.line) && ch == ' ')
            document.insertText(view.cursorPosition(), ' ');
    }

    document.editEnd();
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
 * one is a start or middle of a multi-line comment.
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
    if (line < document.lines())
    {
        // Make sure it is not another one comment, and if so,
        // going to find it's end as well...
        var currentLineText = document.line(line).ltrim();
        if (currentLineText.startsWith("//"))
            line = findSingleLineCommentBlockEnd(line);
        else if (currentLineText.startsWith("/*"))
            line = findMultiLineCommentBlockEnd(line);
    }
    else line = 0;                                          // EOF found
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
    if (line < document.lines())
    {
        var currentLineText = document.line(line).ltrim();  // Get text of the found line
        while (currentLineText.length == 0)                 // Skip empty lines if any
            currentLineText = document.line(++line).ltrim();
        // Make sure it is not another one multiline comment, and if so,
        // going to find it's end as well...
        if (currentLineText.startsWith("/*"))
            line = findMultiLineCommentBlockEnd(line);
    }
    else line = 0;                                          // EOF found
    return line;
}

/**
 * Almost anything in a code is placed withing some brackets.
 * So the ideas is simple:
 * \li find nearest open bracket of any kind
 * \li depending on its type and presence of leading delimiters (non identifier characters)
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
    // NOTE Yep, it is expected (hardcoded) that multiline comment has
    // all lines started w/ a star symbol!
    // TODO BUG Kate has a bug: when multiline code snippet gets inserted into
    // a multiline comment block (like Doxygen's @code/@endcode)
    // document.isComment() returns true *only& for the first line of it!
    // So some other way needs to be found to indent comments properly...
    // TODO DAMN... it doesn't work that way also... for snippets longer than 2 lines.
    // I suppose kate first insert text, then indent it, and after that highlight it
    // So indenters based on a highlighting info will not work! BUT THEY DEFINITELY SHOULD!
    else if (currentLineText.startsWith("*") && document.isComment(line, 0))
    {
        dbg("found middle of a multiline comment");
        // Yep, go to find a next non-comment line...
        nextNonCommentLine = findMultiLineCommentBlockEnd(line);
        middleOfMultilineBlock = true;
    }
    dbg("line="+line);
    dbg("document.isComment(line, 0)="+document.isComment(line, 0));
    //dbg("document.defStyleNum(line, 0)="+document.defStyleNum(line-1, 0));
    dbg("currentLineText='"+currentLineText+"'");
    dbg("middleOfMultilineBlock="+middleOfMultilineBlock);

    if (nextNonCommentLine == 0)                            // End of comment not found?
        // ... possible due temporary invalid code...
        // anyway, dunno how to align it!
        return -2;
    // So, are we inside a comment? (and we know where it ends)
    if (nextNonCommentLine != -1)
    {
        // Yep, lets try to get desired indent for next non-comment line
        var desiredIndent = indentLine(nextNonCommentLine);
        if (desiredIndent < 0)
        {
            // Have no idea how to indent this comment! So try to align it
            // as found line:
            desiredIndent = document.firstColumn(nextNonCommentLine);
        }
        // TODO Make sure that next non-comment line do not starts
        // w/ 'special' chars...
        return desiredIndent + (middleOfMultilineBlock|0);
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
 * Try to align \c break or \c continue statements in a loop or \c switch.
 *
 * Also it take care about the following case:
 * \code
 *  for (blah-blah)
 *  {
 *      if (smth)
 *          break;
 *  }
 * \endcode
 */
function alignBreakContinue(line)
{
    var result = -2;
    var currentLineText = document.line(line).ltrim();
    var is_break = currentLineText.startsWith("break;");
    var should_proceed = is_break || currentLineText.startsWith("continue;")
    if (should_proceed)
        result = tryBreakContinue(line - 1, is_break);
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
        result = alignBreakContinue(line);                  // Try to align `break' or `continue' statements
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
    var crsr = view.cursorPosition();

    dbg("indentWidth: " + indentWidth);
    dbg("       Mode: " + document.highlightingModeAt(crsr));
    dbg("  Attribute: " + document.attributeName(crsr));
    dbg("       line: " + line);
    dbg("       char: " + crsr + " -> '" + ch + "'");

    if (ch != "")
        return processChar(line, ch);

    return indentLine(line);
}

/**
 * \todo Better to use \c defStyleNum() instead of \c attributeName() and string comparison
 */

// kate: space-indent on; indent-width 4; replace-tabs on;
