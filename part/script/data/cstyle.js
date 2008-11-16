/** kate-script
 * name: C Style
 * license: LGPL
 * author: Dominik Haumann <dhdev@gmx.de>
 * version: 1
 * kate-version: 3.0
 * type: indentation
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
var cfgIndentCase = true;         // indent 'case' and 'default' in a switch?
var cfgIndentNamespace = true;    // indent after 'namespace'?
var cfgAutoInsertStar = false;    // auto insert '*' in C-comments
var cfgSnapSlash = false;         // snap '/' to '*/' in C-comments
var cfgAutoInsertSlashes = false; // auto insert '//' after C++-comments
//END USER CONFIGURATION

// indent gets three arguments: line, indentwidth in spaces, typed character
// indent

// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = "{}/:;";

//BEGIN global variables and functions
// maximum number of lines we look backwards/forward to find out the indentation
// level (the bigger the number, the longer might be the delay)
var gLineDelimiter = 50;     // number

var gIndentWidth = 4;
//END global variables and functions


/**
 * Search for a corresponding '{' and return its indentation level. If not found
 * return null; (line/column) are the start of the search.
 */
function findLeftBrace(line, column)
{
    var cursor = document.anchor(line, column, '{');
    if (cursor) {
        var parenthesisCursor = tryParenthesisBeforeBrace(cursor.line, cursor.column);
        if (parenthesisCursor)
            cursor = parenthesisCursor;

        debug("findLeftBrace: success in line " + cursor.line);
        return document.firstVirtualColumn(cursor.line);
    }

    return -1;
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
        return document.anchor(line, column, '(');
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
        return -1;

    var indentation = -1;
    var lineDelimiter = gLineDelimiter;
    var currentLine = line;

    while (currentLine > 0 && lineDelimiter > 0) {
        --currentLine;
        --lineDelimiter;
        if (document.firstColumn(currentLine) == -1)
            continue;

        currentString = document.line(currentLine);
        if (currentString.search(/^\s*(default\s*|case\b.*):/) != -1) {
            indentation = document.firstVirtualColumn(currentLine);
            break;
        } else if (currentString.search(/^\s*switch\b/) != -1) {
            indentation = document.firstVirtualColumn(currentLine);
            if (cfgIndentCase)
                indentation += gIndentWidth;
            break;
        }
    }

    if (indentation != -1) debug("trySwitchStatement: success in line " + currentLine);
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
        return -1;

    var cursor = document.anchor(line, 0, '{');
    if (!cursor)
        return -1;

    var indentation = document.firstVirtualColumn(cursor.line);
    if (cfgIndentCase)
        indentation += gIndentWidth;

    if (indentation != -1) debug("tryAccessModifiers: success in line " + cursor.line);
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

// /**
//  * Check, whether the beginning of line is inside a "..." string context.
//  * Note: the function does not check for comments
//  * return: leading whitespaces as string, or null if not in a string
//  */
// function inString(line)
// {
//     var currentLine = line;
//     var currentString;
//
//     // go line up as long as the previous line ends with an escape character '\'
//     while (currentLine >= 0) {
//         currentString = document.line(currentLine - 1);
//         if (currentString.charAt(document.lastColumn(currentLine - 1)) != '\\')
//             break;
//         --currentLine;
//     }
//
//     // iterate through all lines and toggle bool insideString for every quote
//     var insideString = false;
//     var indentation = null;
//     while (currentLine < line) {
//         currentString = document.line(currentLine);
//         var char1;
//         var i;
//         var lineLength = document.lineLength(currentLine);
//         for (i = 0; i < lineLength; ++i) {
//             char1 = currentString.charAt(i);
//             if (char1 == "\\") {
//                 // skip escaped character
//                 ++i;
//             } else if (char1 == "\"") {
//                 insideString = !insideString;
//                 if (insideString)
//                     indentation = currentString.substring(0, document.firstColumn(currentLine) + 1);
//             }
//         }
//         ++currentLine;
//     }
//
//     return insideString ? indentation : null;
// }

/**
 * C comment checking. If the previous line begins with a "/*" or a "* ", then
 * return its leading white spaces + ' *' + the white spaces after the *
 * return: filler string or null, if not in a C comment
 */
function tryCComment(line)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return -1;

    var indentation = -1;

    // we found a */, search the opening /* and return its indentation level
    if (document.endsWith(currentLine, "*/", true)) {
        var cursor = {line: currentLine, column: document.line(currentLine).search(/\/\*.*\*\//)};
        if (cursor && cursor.column == document.firstColumn(cursor.line))
            indentation = document.firstVirtualColumn(cursor.line);

        if (indentation != -1) debug("tryCComment: success (1) in line " + cursor.line);
        return indentation;
    }

    // inbetween was an empty line, so do not copy the "*" character
    if (currentLine != line - 1)
        return -1;

    var firstPos = document.firstColumn(currentLine);
    var lastPos = document.lastColumn(currentLine);
    var char1 = document.charAt(currentLine, firstPos);
    var char2 = document.charAt(currentLine, firstPos + 1);

    if (char1 == '/' && char2 == '*') {
        indentation = document.firstVirtualColumn(currentLine);
        if (cfgAutoInsertStar) {
            // only add '*', if there is none yet.
            indentation += 1;
            if (document.firstChar(line) != '*')
                document.insertText(line, view.cursorPosition().column, '*');
            if (!document.isSpace(line, document.firstColumn(line) + 1))
                document.insertText(line, document.firstColumn(line) + 1, ' ');
        }
    } else if (char1 == '*' && (firstPos == lastPos || document.isSpace(currentLine, firstPos + 1))) {
        var currentString = document.line(currentLine);
        currentString.search(/^\s*\*(\s*)/);
        // in theory, we could search for opening /*, and use its indentation
        // and then one alignment character. Let's not do this for now, though.
        var end = RegExp.$1;
        indentation = document.firstVirtualColumn(currentLine);
        // only add '*', if there is none yet.
        if (cfgAutoInsertStar && document.firstChar(line) != '*') {
            document.insertText(line, view.cursorPosition().column, '*');
            if (!document.isSpace(line, document.firstColumn(line) + 1))
                document.insertText(line, document.firstColumn(line) + 1, ' ');
        }
    }

    if (indentation != -1) debug("tryCComment: success (2) in line " + currentLine);
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
        return -1;

    var indentation = -1;
    var comment = document.startsWith(currentLine, "//", true);

    // allowed are: //, ///, //! ///<, //!< and ////...
    if (comment) {
        var firstPos = document.firstColumn(currentLine);
        var currentString = document.line(currentLine);

        var char3 = currentString.charAt(firstPos + 2);
        var char4 = currentString.charAt(firstPos + 3);
        indentation = document.firstVirtualColumn(currentLine);

        if (cfgAutoInsertSlashes) {
            if (char3 == '/' && char4 == '/') {
                // match ////... and replace by only two: //
                currentString.search(/^\s*(\/\/)/);
            } else if (char3 == '/' || char3 == '!') {
                // match ///, //!, ///< and //!
                currentString.search(/^\s*(\/\/[\/!][<]?\s*)/);
            } else {
                // only //, nothing else
                currentString.search(/^\s*(\/\/\s*)/);
            }
            document.insertText(line, view.cursorPosition().column, RegExp.$1);
        }
    }

    if (indentation != -1) debug("tryCppComment: success in line " + currentLine);
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
        return -1;

    var lastPos = document.lastColumn(currentLine);
    var indentation = -1;

    if (document.charAt(currentLine, lastPos) == '{') {
        var cursor = tryParenthesisBeforeBrace(currentLine, lastPos);
        if (cursor) {
            indentation = document.firstVirtualColumn(cursor.line) + gIndentWidth;
        } else {
            indentation = document.firstVirtualColumn(currentLine);
            if (cfgIndentNamespace || !isNamespace(currentLine, lastPos))
                // take its indentation and add one indentation level
                indentation += gIndentWidth;
        }
    }

    if (indentation != -1) debug("tryBrace: success in line " + currentLine);
    return indentation;
}

/**
 * Check for if, else, while, do, switch, private, public, protected, signals,
 * default, case etc... keywords, as we want to indent then. If   is
 * non-null/true, then indentation is not increased.
 * Note: The code is written to be called *after* tryCComment and tryCppComment!
 */
function tryCKeywords(line, isBrace)
{
    var currentLine = document.prevNonEmptyLine(line - 1);
    if (currentLine < 0)
        return -1;

    // if line ends with ')', find the '(' and check this line then.
    var lastPos = document.lastColumn(currentLine);
    var cursor = null;
    if (document.charAt(currentLine, lastPos) == ')')
        cursor = document.anchor(currentLine, lastPos, '(');
    if (cursor)
        currentLine = cursor.line;

    // found non-empty line
    var currentString = document.line(currentLine);
    if (currentString.search(/^\s*(if\b|for|do\b|while|switch|[}]?\s*else|((private|public|protected|case|default|signals|Q_SIGNALS).*:))/) == -1)
        return -1;
//    debug("Found first word: " + RegExp.$1);
    lastPos = document.lastColumn(currentLine);
    var lastChar = currentString.charAt(lastPos);
    var indentation = -1;

    // try to ignore lines like: if (a) b; or if (a) { b; }
    if (lastChar != ';' && lastChar != '}') {
        // take its indentation and add one indentation level
        indentation = document.firstVirtualColumn(currentLine);
        if (!isBrace)
            indentation += gIndentWidth;
    }

    if (indentation != -1) debug("tryCKeywords: success in line " + currentLine);
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
        return -1;

    // found non-empty line
    var currentString = document.line(currentLine);
    var lastPos = document.lastColumn(currentLine);
    var lastChar = currentString.charAt(lastPos);
    var indentation = -1;

    if (lastChar == ';'
        && currentString.search(/^\s*(if\b|[}]?\s*else|do\b|while\b|for)/) == -1)
    {
        // idea: we had something like:
        //   if/while/for (expression)
        //       statement();  <-- we catch this trailing ';'
        // Now, look for a line that starts with if/for/while, that has one
        // indent level less.
        indentation = document.firstVirtualColumn(currentLine);
        if (indentation == 0)
            return -1;

        var lineDelimiter = 10; // 10 limit search, hope this is a sane value
        while (currentLine > 0 && lineDelimiter > 0) {
            --currentLine;
            --lineDelimiter;
            var firstPosVirtual = document.firstVirtualColumn(currentLine);
            if (firstPosVirtual == -1)
                continue;

            if (firstPosVirtual < indentation) {
                currentString = document.line(currentLine);
                if (currentString.search(/^\s*(if\b|[}]?\s*else|do\b|while\b|for)[^{]*$/) != -1)
                    indentation = firstPosVirtual;
                break;
            }
        }
    }
        debug("9: " + indentation);


    if (indentation != -1) debug("tryCondition: success in line " + currentLine);
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
        return -1;

    var indentation = -1;
    var currentString = document.line(currentLine);
    var result = /^(.*)(,|[)];)\s*(\/\/.*|\/\*.*\*\/\s*)?$/.exec(currentString);
    if (result != null && result.index == 0) {
        var comma = (result[2].length == 1);
        var cursor = document.anchor(currentLine, result[1].length, '(');
        if (cursor) {
            if (comma) {
                currentLine = cursor.line;
                var column = cursor.column + 1;
                var lastColumn = document.lastColumn(currentLine);
                while (column < lastColumn && document.isSpace(currentLine, ++column));
                indentation = document.toVirtualColumn(currentLine, column);
            } else {
                currentLine = cursor.line;
                indentation = document.firstVirtualColumn(currentLine);
            }
        }
    }

    if (indentation != -1) debug("tryStatement: success in line " + currentLine);
    return indentation;
}

/**
 * Indent line.
 * Return filler or null.
 */
function indentLine(line, alignOnly)
{
    var firstChar = document.firstChar(line);
    var lastChar = document.lastChar(line);

    var filler = -1;

    if (filler == -1 && firstChar == '}')
        filler = findLeftBrace(line, document.firstColumn(line));
    if (filler == -1)
        filler = tryCComment(line);
    if (filler == -1 && !alignOnly)
        filler = tryCppComment(line);
    if (filler == -1)
        filler = trySwitchStatement(line);
    if (filler == -1)
        filler = tryAccessModifiers(line);
    if (filler == -1)
        filler = tryBrace(line);
    if (filler == -1)
        filler = tryCKeywords(line, firstChar == '{');
    if (filler == -1)
        filler = tryCondition(line);
    if (filler == -1)
        filler = tryStatement(line);

    return filler;
}

function processChar(line, c)
{
    if (c != '{' && c != '}' && c != '/' && c != ':')
        return -2;

    var cursor = view.cursorPosition();
    if (!cursor)
        return -2;

    var column = cursor.column;
    var firstPos = document.firstColumn(line);
    var lastPos = document.lastColumn(line);

//     debug("firstPos: " + firstPos);
//     debug("column..: " + column);

    if (firstPos == column - 1 && c == '{') {
        // todo: maybe look for if etc.
        var filler = tryBrace(line);
        if (filler == -1)
            filler = tryCKeywords(line, true);
        if (filler == -1)
            filler = tryCComment(line); // checks, whether we had a "*/"
        if (filler == -1)
            filler = tryStatement(line);
        if (filler == -1)
            filler = -2;

        return filler;
    } else if (firstPos == column - 1 && c == '}') {
        var indentation = findLeftBrace(line, firstPos);
        if (indentation == -1)
            indentation = -2;
        return indentation;
    } else if (cfgSnapSlash && c == '/' && lastPos == column - 1) {
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
        return -2;
    } else if (c == ':') {
        // todo: handle case, default, signals, private, public, protected, Q_SIGNALS
        var filler = trySwitchStatement(line);
        if (filler == -1)
            filler = tryAccessModifiers(line);
        if (filler == -1)
            filler = -2;
        return filler;
    }
    return -2;
}

/**
 * Process a newline character.
 * This function is called whenever the user hits <return/enter>.
 */
function indent(line, indentWidth, ch)
{
    gIndentWidth = indentWidth;
    var alignOnly = (ch == "");

    if (ch != '\n' && !alignOnly)
        return processChar(line, ch);

    return indentLine(line, alignOnly);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
