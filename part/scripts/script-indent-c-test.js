/** kate-js-indenter
 * name: C++ Indenter
 * license: LGPL
 * author: Dominik Haumann <dhdev@gmx.de>
 * version: 1
 * kateversion: 3.0
 *
 * This file is part of the Kate Project.
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

//BEGIN USER CONFIGURATION
// indent 'case' and 'default' in a switch?
var cfgIndentCase = true;

// indent after 'namespace'?
var cfgIndentNamespace = true;
//END USER CONFIGURATION

indenter.processChar = processChar
indenter.processNewLine = processNewLine
indenter.processLine = processLine
indenter.processSection = processSection
indenter.processIndent = processIndent

//BEGIN global variables and functions
// maximum number of lines we look backwards/forward to find out the indentation
// level (the bigger the number, the longer might be the delay)
var gLineDelimiter = 50;     // number

// default settings. To read from current view/document, call readSettings()
var gTabWidth = 8;           // number
var gExpandTab = false;      // bool
var gIndentWidth = 4;        // number
var gIndentFiller = "    ";  // gIndentWidth many whitespaces
var gTabFiller = "        "; // tab as whitespaces, example: if gTabWidth is 4,
                             // then this is "    "

// dummy variables, 100 spaces, 50 tabs
var gSpaces = "                                                                                                    ";
var gTabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

/**
 * call in indentNewLine and indentChar to read document/view variables like
 * tab width, indent width, expand tabs etc.
 */
function readSettings()
{
    gTabWidth = document.tabWidth;
    gIndentWidth = document.indentWidth;
    gExpandTab = document.replaceTabs;

    // fill in fillers
    gIndentFiller = gSpaces.substring(0, gIndentWidth);
    gTabFiller = gSpaces.substring(0, gTabWidth);
}
//END global variables and functions


//BEGIN global variables to remember state
// remember previous line and column (note: only set in indentNewLine)
var gLastLine = -1;
var gLastColumn = -1;
//END global variables to remember state

//BEGIN indentation functions
/**
 * Return indentation filler string.
 * parameters: count [number], alignment [number]
 * If expandTab is true, the returned filler only contains spaces.
 * If expandTab is false, the returned filler contains tabs upto alignment
 * if possible, and from alignment to the end of text spaces. This implements
 * mixed indentation, i.e. indentation+alignment.
 */
function indentString(count, alignment)
{
    if (count == null || count < 0)
        return null;

    // create indentation string
    var indentation = gSpaces.substring(0, count);

    if (!gExpandTab) {
        var alignFiller = "";
        if (alignment && alignment >= 0 && alignment < indentation.length) {
            alignFiller = gSpaces.substring(0, indentation.length - alignment);
            indentation = indentation.substring(0, alignment);
        }

        // fill indentation with tabs and spaces
        var spaceCount = indentation.length % gTabWidth;
        var tabCount = (indentation.length - spaceCount) / gTabWidth;
        indentation = gTabs.substring(0, tabCount);
        indentation += gSpaces.substring(0, spaceCount);
        indentation += alignFiller;
    }

    return indentation;
}

/**
 * Adds one indentation level. If alignment is valid, mixed
 * indentation is turned on, see indentString() for further details.
 */
function incIndent(count, alignment)
{
    count += gIndentWidth;
    return indentString(count, alignment);
}
//END indentation functions

/**
 * Search for a corresponding '{' and return its indentation level. If not found
 * return null; (line/column) are the start of the search.
 */
function findLeftBrace(line, column)
{
    var cursor = document.findLeftBrace(line, column);
    if (cursor) {
        var parenthesisCursor = tryParenthesisBeforeBrace(cursor.line, cursor.column);
        if (parenthesisCursor)
            cursor = parenthesisCursor;

        debug("findLeftBrace: success in line " + cursor.line);
        return indentString(document.firstVirtualColumn(cursor.line, gTabWidth));
    }

    return null;
}

/**
 * Character at (line, column) has to be a '{'.
 * Now try to find the right line for indentation for constructs like:
 * if (a == b
 *     && c == d) { <- check for ')', and find '(', then return its indentation
 * Returns null, if no success, otherwise an object {line, column}
 */
function tryParenthesisBeforeBrace(line, column)
{
    var firstColumn = document.firstColumn(line);
    while (column > firstColumn && document.isSpace(line, --column));
    if (document.charAt(line, column) == ')')
        return document.findLeftParenthesis(line, column);
    return null;
}

/**
 * Check for default and case keywords and assume we are in a switch statement.
 * Try to find a previous default, case or switch and return its indentation or
 * -1 if not found.
 */
function trySwitchStatement(line)
{
    var currentString = document.line(line);
    if (currentString.search(/^\s*(default\s*|case\b.*):/) == -1)
        return null;

    var indentation = null;
    var lineDelimiter = gLineDelimiter;
    var currentLine = line;

    while (currentLine > 0 && lineDelimiter > 0) {
        --currentLine;
        --lineDelimiter;
        if (document.firstColumn(currentLine) == -1)
            continue;

        currentString = document.line(currentLine);
        if (currentString.search(/^\s*(default\s*|case\b.*):/) != -1) {
            indentation = indentString(document.firstVirtualColumn(currentLine, gTabWidth));
            break;
        } else if (currentString.search(/^\s*switch\b/) != -1) {
            if (cfgIndentCase) {
                indentation = incIndent(document.firstVirtualColumn(currentLine, gTabWidth));
            } else {
                indentation = indentString(document.firstVirtualColumn(currentLine, gTabWidth));
            }

            break;
        }
    }

    if (indentation != null) debug("trySwitchStatement: success in line " + currentLine);
    return indentation;
}

/**
 * Check for private, protected, public, signals etc... and assume we are in a
 * class definition. Try to find a previous private/protected/private... or
 * class and return its indentation or null if not found.
 */
function tryAccessModifiers(line)
{
    var currentString = document.line(line);
    if (currentString.search(/^\s*((public|protected|private)\s*\S*|(signals|Q_SIGNALS)\s*):/) == -1)
        return null;

    var cursor = document.findLeftBrace(line, 0);
    if (!cursor)
        return null;

    var firstPos = document.firstVirtualColumn(cursor.line, gTabWidth);
    var indentation = null;

    if (cfgIndentCase) {
        indentation = incIndent(firstPos);
    } else {
        indentation = indentString(firstPos);
    }

    if (indentation != null) debug("tryAccessModifiers: success in line " + cursor.line);
    return indentation;
}



/**
 * Get the position of the first non-space character
 * return: position or -1, if there is no non-space character
 */
function firstNonSpace(text)
{
    if (text && text.search(/^(\s*)\S/) != -1)
        return RegExp.$1.length;

    return -1;
}

/**
 * Get the position of the last non-space character
 * return: position or -1, if there is no non-space character
 */
function lastNonSpace(text)
{
    if (text && text.search(/(.*)\S\s*$/) != -1)
        return RegExp.$1.length;

    return -1;
}

/**
 * Check, whether the beginning of line is inside a "..." string context.
 * Note: the function does not check for comments
 * return: leading whitespaces as string, or null if not in a string
 */
function inString(line)
{
    var currentLine = line;
    var currentString;

    // go line up as long as the previous line ends with an escape character '\'
    while (currentLine >= 0) {
        currentString = document.line(currentLine - 1);
        if (currentString.charAt(document.lastColumn(currentLine - 1)) != '\\')
            break;
        --currentLine;
    }

    // iterate through all lines and toggle bool insideString for every quote
    var insideString = false;
    var indentation = null;
    while (currentLine < line) {
        currentString = document.line(currentLine);
        var char1;
        var i;
        var lineLength = document.lineLength(currentLine);
        for (i = 0; i < lineLength; ++i) {
            char1 = currentString.charAt(i);
            if (char1 == "\\") {
                // skip escaped character
                ++i;
            } else if (char1 == "\"") {
                insideString = !insideString;
                if (insideString)
                    indentation = currentString.substring(0, document.firstColumn(currentLine) + 1);
            }
        }
        ++currentLine;
    }

    return insideString ? indentation : null;
}

/**
 * C comment checking. If the previous line begins with a "/*" or a "* ", then
 * return its leading white spaces + ' *' + the white spaces after the *
 * return: filler string or null, if not in a C comment
 */
function tryCComment(line)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return null;

    var indentation = null;

    // we found a */, search the opening /* and return its indentation level
    if (document.endsWith(currentLine, "*/", true)) {
        var cursor = document.findStartOfCComment(currentLine, document.lastColumn(currentLine));
        if (cursor && cursor.column == document.firstColumn(cursor.line))
            indentation = indentString(document.firstVirtualColumn(cursor.line, gTabWidth));

        if (indentation != null) debug("tryCComment: success (1) in line " + cursor.line);
        return indentation;
    }

    // inbetween was an empty line, so do not copy the "*" character
    if (currentLine != line - 1)
        return null;

    var firstPos = document.firstColumn(currentLine);
    var lastPos = document.lastColumn(currentLine);
    var char1 = document.charAt(currentLine, firstPos);
    var char2 = document.charAt(currentLine, firstPos + 1);

    if (char1 == '/' && char2 == '*') {
        indentation = indentString(document.firstVirtualColumn(currentLine, gTabWidth));
        indentation += ' ';
        // only add '*', if there is none yet.
        if (document.charAt(line, document.firstColumn(line)) != '*')
            indentation += "* ";
    } else if (char1 == '*' && (firstPos == lastPos || document.isSpace(currentLine, firstPos + 1))) {
        var currentString = document.line(currentLine);
        currentString.search(/^\s*\*(\s*)/);
        // in theory, we could search for opening /*, and use its indentation
        // and then one alignment character. Let's not do this for now, though.
        var end = RegExp.$1;
        indentation = indentString(document.firstVirtualColumn(currentLine, gTabWidth));
        // only add '*', if there is none yet.
        if (document.charAt(line, document.firstColumn(line)) != '*')
            indentation += "* ";
        if (indentation.charAt(indentation.length - 1) == '*')
            indentation += ' ';
    }

    if (indentation != null) debug("tryCComment: success (2) in line " + currentLine);
    return indentation;
}

/**
 * C++ comment checking. If the previous line begins with a "//", then
 * return its leading white spaces + '//'. Special treatment for:
 * //, ///, //! ///<, //!< and ////...
 * return: filler string or null, if not in a star comment
 */
function tryCppComment(line)
{
    var currentLine = line - 1;
    if (currentLine < 0)
        return null;

    var indentation = null;
    var comment = document.startsWith(currentLine, "//", true);

    // allowed are: //, ///, //! ///<, //!< and ////...
    if (comment) {
        var firstPos = document.firstColumn(currentLine);
        var currentString = document.line(currentLine);

        var char3 = currentString.charAt(firstPos + 2);
        var char4 = currentString.charAt(firstPos + 3);
        var firstPosVirtual = document.firstVirtualColumn(currentLine, gTabWidth);

        if (char3 == '/' && char4 == '/') {
            // match ////... and replace by only two: //
            currentString.search(/^\s*(\/\/)/);
        } else if (char3 == '/' || char3 == '!') {
            // match ///, //!, ///< and //!
            currentString.search(/^\s*(\/\/[/!][<]?\s*)/);
        } else {
            // only //, nothing else
            currentString.search(/^\s*(\/\/\s*)/);
        }
        var ending = RegExp.$1;
        indentation = indentString(firstPosVirtual) + ending;
// if we want to force a space after //, remove the comments
//        if (lastNonSpace(indentation) == indentation.length - 1)
//            indentation += ' ';
    }

    if (indentation != null) debug("tryCppComment: success in line " + currentLine);
    return indentation;
}

/**
 * If the last non-empty line ends with a {, take its indentation level (or
 * maybe the one found by tryParenthesisBeforeBrace()) and return it increased
 * by 1 indetation level (special case: namespaces indentation depends on
 * cfgIndentNamespace). If not found, return null.
 */
function tryBrace(line)
{
    function isNamespace(line, column)
    {
        if (document.firstColumn(line) == column && line > 0)
            --line;
        var currentString = document.line(line);
        return (currentString.search(/^\s*namespace\b/) != -1);
    }

    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return null;

    var lastPos = document.lastColumn(currentLine);
    var indentation = null;

    if (document.charAt(currentLine, lastPos) == '{') {
        var cursor = tryParenthesisBeforeBrace(currentLine, lastPos);
        if (cursor) {
            var firstPosVirtual = document.firstVirtualColumn(cursor.line, gTabWidth);
            indentation = incIndent(firstPosVirtual);
        } else {
            var firstPosVirtual = document.firstVirtualColumn(currentLine, gTabWidth);
            if (cfgIndentNamespace || !isNamespace(currentLine, lastPos)) {
                // take its indentation and add one indentation level
                indentation = incIndent(firstPosVirtual);
            } else {
                indentation = indentString(firstPosVirtual);
            }
        }
    }

    if (indentation != null) debug("tryBrace: success in line " + currentLine);
    return indentation;
}

/**
 * Check for if, else, while, do, switch, private, public, protected, signals,
 * default, case etc... keywords, as we want to indent then. If isBrace is
 * non-null/true, then indentation is not increased.
 * Note: The code is written to be called *after* tryCComment and tryCppComment!
 */
function tryCKeywords(line, isBrace)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return null;

    // if line ends with ')', find the '(' and check this line then.
    var lastPos = document.lastColumn(currentLine);
    var cursor = null;
    if (document.charAt(currentLine, lastPos) == ')')
        cursor = document.findLeftParenthesis(currentLine, lastPos);
    if (cursor)
        currentLine = cursor.line;

    // found non-empty line
    var currentString = document.line(currentLine);
    if (currentString.search(/^\s*(if\b|for|do\b|while|switch|[}]?\s*else|((private|public|protected|case|default|signals|Q_SIGNALS).*:))/) == -1)
        return null;
//    debug("Found first word: " + RegExp.$1);
    lastPos = document.lastColumn(currentLine);
    var lastChar = currentString.charAt(lastPos);
    var indentation = null;

    // try to ignore lines like: if (a) b; or if (a) { b; }
    if (lastChar != ';' && lastChar != '}') {
        // take its indentation and add one indentation level
        if (isBrace) {
            indentation = indentString(document.firstVirtualColumn(currentLine, gTabWidth));
        } else {
            indentation = incIndent(document.firstVirtualColumn(currentLine, gTabWidth));
        }
    }

    if (indentation != null) debug("tryCKeywords: success in line " + currentLine);
    return indentation;
}

/**
 * Search for if, do, while, for, ... as we want to indent then.
 * Return null, if nothing useful found.
 * Note: The code is written to be called *after* tryCComment and tryCppComment!
 */
function tryCondition(line)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return null;

    // found non-empty line
    var currentString = document.line(currentLine);
    var lastPos = document.lastColumn(currentLine);
    var lastChar = currentString.charAt(lastPos);
    var indentation = null;

    if (lastChar == ';'
        && currentString.search(/^\s*(if\b|[}]?\s*else|do\b|while\b|for)/) == -1)
    {
        // idea: we had something like:
        //   if/while/for (expression)
        //       statement();  <-- we catch this trailing ';'
        // Now, look for a line that starts with if/for/while, that has one
        // indent level less.
        var indentLevelVirtual = document.firstVirtualColumn(currentLine, gTabWidth);
        if (indentLevelVirtual == 0)
            return null;

        var lineDelimiter = 10; // 10 limit search, hope this is a sane value
        while (currentLine > 0 && lineDelimiter > 0) {
            --currentLine;
            --lineDelimiter;

            var firstPosVirtual = document.firstVirtualColumn(currentLine, gTabWidth);
            if (firstPosVirtual == -1)
                continue;

            if (firstPosVirtual < indentLevelVirtual) {
                currentString = document.line(currentLine);
                if (currentString.search(/^\s*(if\b|[}]?\s*else|do\b|while\b|for)[^{]*$/) != -1)
                    indentation = indentString(firstPosVirtual);
                break;
            }
        }
    }

    if (indentation != null) debug("tryCondition: success in line " + currentLine);
    return indentation;
}

/**
 * If the non-empty line ends with ); or ',', then search for '(' and return its
 * indentation; also try to ignore trailing comments.
 */
function tryStatement(line)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return null;

    var indentation = null;
    var currentString = document.line(currentLine);
    var column = currentString.search(/^(.*)(,|[)];)\s*(\/\/.*|\/\*.*\*\/\s*)?$/);
    if (column == 0) {
        var comma = (RegExp.$2.length == 1);
        var cursor = document.findLeftParenthesis(currentLine, RegExp.$1.length);
        if (cursor) {
            if (comma) {
                currentLine = cursor.line;
                column = cursor.column;
                var lastColumn = document.lastColumn(currentLine);
                while (column < lastColumn && document.isSpace(currentLine, ++column));
                indentation = indentString(document.toVirtualColumn(currentLine, column, gTabWidth)); // add alignment?
            } else {
                currentLine = cursor.line;
                indentation = indentString(document.firstVirtualColumn(currentLine, gTabWidth));
            }
        }
    }

    if (indentation != null) debug("tryStatement: success in line " + currentLine);
    return indentation
}

/**
 * Search non-empty line and return its indentation string or null, if not found.
 */
function keepIndentation(line)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return null;

    debug("keepIndentation: success in line " + currentLine);
    return indentString(document.firstVirtualColumn(currentLine, gTabWidth));
}

/**
 * Indent line.
 * Return filler or null.
 */
function indentLine(line, indentOnly)
{
    var firstChar = document.firstChar(line);
    var lastChar = document.lastChar(line);

    var filler = null;

    if (filler == null && firstChar == '}')
        filler = findLeftBrace(line, document.firstColumn(line));
    if (filler == null)
        filler = tryCComment(line);
    if (filler == null && !indentOnly)
        filler = tryCppComment(line);
    if (filler == null)
        filler = trySwitchStatement(line);
    if (filler == null)
        filler = tryAccessModifiers(line);
    if (filler == null)
        filler = tryBrace(line);
    if (filler == null)
        filler = tryCKeywords(line, firstChar == '{');
    if (filler == null)
        filler = tryCondition(line);
    if (filler == null)
        filler = tryStatement(line);

    // we don't know what to do, let's simply keep the indentation
    if (filler == null)
        filler = keepIndentation(line);

    if (filler != null) {
        var firstPos = document.firstColumn(line);
        if (firstPos < 0)
            firstPos = document.virtualLineLength(line);

        document.editBegin();
        if (firstPos > 0)
            document.removeText(line, 0, line, firstPos);
        document.insertText(line, 0, filler);
        view.setCursorPosition(line, filler.length);
        document.editEnd();
    }

    return filler;
}

//BEGIN main/entry functions
function processChar(c)
{
    if (c != '{' && c != '}' && c != '/' && c != ':')
        return;

    readSettings();

    var cursor = view.cursorPosition();
    if (!cursor)
        return;

    var line = cursor.line;
    var column = cursor.column;
    var firstPos = document.firstColumn(line);
    var lastPos = document.lastColumn(line);

//     debug("firstPos: " + firstPos);
//     debug("column..: " + column);

    if (firstPos == column - 1 && c == '{') {
        // todo: maybe look for if etc.
        var filler = tryBrace(line);
        if (filler == null)
            filler = tryCKeywords(line, true);
        if (filler == null)
            filler = tryCComment(line); // checks, whether we had a "*/"
        if (filler == null)
            filler = tryStatement(line);
        if (filler == null)
            filler = keepIndentation(line);

        if (filler != null) {
            document.editBegin();
            if (firstPos > 0)
                document.removeText(line, 0, line, firstPos);
            document.insertText(line, 0, filler);
            view.setCursorPosition(line, filler.length + 1);
            document.editEnd();
        }
    } else if (firstPos == column - 1 && c == '}') {
        var filler = findLeftBrace(line, firstPos);
        if (filler != null) {
            document.editBegin();
            if (firstPos > 0)
                document.removeText(line, 0, line, firstPos);
            document.insertText(line, 0, filler);
            view.setCursorPosition(line, filler.length + 1);
            document.editEnd();
        }
    } else if (c == '/' && lastPos == column - 1 && gLastLine == line && gLastColumn == column - 1) {
        // try to snap the string "* /" to "*/"
        var currentString = document.line(line);
        if (currentString.search(/^(\s*)\*\s+\/\s*$/) != -1) {
            currentString = RegExp.$1 + "*/";
            document.editBegin();
            document.removeLine(line);
            document.insertLine(line, currentString);
            view.setCursorPosition(line, currentString.length);
            document.editEnd();
        }
    } else if (c == ':') {
        // todo: handle case, default, signals, private, public, protected, Q_SIGNALS
        var filler = trySwitchStatement(line);
        if (filler == null)
            filler = tryAccessModifiers(line);
        if (filler != null) {
            var newColumn = column - (firstPos - filler.length);

            document.editBegin();
            if (firstPos > 0)
                document.removeText(line, 0, line, firstPos);
            document.insertText(line, 0, filler);
            view.setCursorPosition(line, newColumn);
            document.editEnd();
        }
    }
}

/**
 * Process a newline character.
 * This function is called whenever the user hits <return/enter>.
 */
function processNewLine(line, column)
{
    readSettings();

    var filler = indentLine(line, false);
    if (filler != null) {
        // remember position of last action
        gLastLine = line;
        gLastColumn = filler.length;
    } else {
        gLastLine = -1;
        gLastColumn = -1;
    }
}

function processLine(line)
{
    indentLine(line, true);
}

function processSection(startLine, endLine)
{
    document.editBegin();
    while (startLine <= endLine)
        indentLine(startLine++, true);
    document.editEnd();
}

function processIndent(line, levels)
{
    function localProcessIndent(line, levels)
    {
        var firstVirtualColumn = document.firstVirtualColumn(line, gTabWidth);
        var column = firstVirtualColumn;
        if (column < 0)
            column = document.virtualLineLength(line, gTabWidth);
        column += levels * gIndentWidth;
        if (column < 0)
            column = 0;

        var diff = column % gIndentWidth;
        column -= diff;
        if (diff > 0 && levels < 0) {
            // do not unindent too much
            column += gIndentWidth;
        }

        var filler = indentString(column);
        var firstPos = document.firstColumn(line);
        if (firstPos == -1)
            firstPos = document.lineLength();

        if (firstPos > 0)
            document.removeText(line, 0, line, firstPos);
        document.insertText(line, 0, filler);
    }

    readSettings();

    document.editBegin();
    if (!view.hasSelection()) {
        localProcessIndent(line, levels);
    } else {
        var currentLine = view.startOfSelection().line;
        var end = view.endOfSelection().line;
        for (; currentLine <= end; ++currentLine) {
            if (document.firstColumn(currentLine) != -1)
                localProcessIndent(currentLine, levels);
        }
    }
    document.editEnd();
}
//END main/entry functions

// kate: space-indent on; indent-width 4; replace-tabs on;
