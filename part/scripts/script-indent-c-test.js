/**KATE
 *NAME: C style indenter
 *LICENSE: short name of the license
 *COPYRIGHT:
 *Based on work Copyright 2005 by Dominik Haumann
 *Copyright 2005 by Joseph Wenninger
 *Here will be the license text, Dominik has to choose
 * The following line is not empty
 *
 *An empty line ends this block
 *
 *VERSION: 0.1
 *ANUNKNOWNKEYWORD: Version has to be in the format major.minor (both numbers)
 *IGNOREALSO: All keywords, except COPYRIGHT are expected to have their data on one line
 *UNKNOWN: unknown keywords are simply ignored from the information parser
 *CURRENTLY_KNOWN_KEYWORDS: NAME,VERSION, COPYRIGHT, LICENSE
 *INFORMATION: This block has to begin in the first line at the first character position
 *INFORMATION: It is optional, but at least all files within the kde cvs,
 *INFORMATION: which are ment for publishing are supposed to have at least the
 *INFORMATION: COPYRIGHT block
 *INFORMATION: These files have to be stored as UTF8
 *INFORMATION: The copyright text should be in english
 *INFORMATION: A localiced copyright statement could be put into a blah.desktop file
 **/

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
        return -1;

    // create indentation string
    var indentation = gSpaces.substring(0, count);
    if (!gExpandTab) {
        var i;
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

//BEGIN process character
function indentChar(c)
{
    if (c != '{' && c != '}' && c != '/' && c != ':')
        return;

    readSettings();

    var line = view.cursorLine();
    var column = view.cursorColumnReal();
    var firstPos = document.firstCharPos(line);
    var lastPos = document.lastCharPos(line);

    debug("firstPos: " + firstPos);
    debug("column..: " + column);

    if (firstPos == column - 1 && c == '{') {
        // todo: maybe look for if etc.
        var filler = tryBrace(line);
        if (filler == -1)
            filler = tryCComment(line); // checks, whether we had a "*/"
        if (filler == -1)
            filler = keepIndentation(line);

        if (filler != -1) {
            document.editBegin();
            if (firstPos > 0)
                document.removeText(line, 0, line, firstPos);
            document.insertText(line, 0, filler);
            view.setCursorPosition(line, filler.length + 1);
            document.editEnd();
        }
    } else if (firstPos == column - 1 && c == '}') {
        var filler = findOpeningBrace(line, column);
        if (filler != -1) {
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
    } else if (firstPos == column - 1 && c == ':') {
        // todo: handle case, default, signals, private, public, protected, Q_SIGNALS
        var filler = tryColon(line, column);
        if (filler != -1) {
//             var newColumn = column - (firstPos - filler.length);
// 
//             document.editBegin();
//             if (firstPos > 0)
//                 document.removeText(line, 0, line, firstPos);
//             document.insertText(line, 0, filler);
//             view.setCursorPosition(line, newColumn);
//             document.editEnd();
        }
    }
}

/**
 * Search for a corresponding '{' and return its indentation level. If not found
 * return -1; (line/column) are the start of the search.
 */
function findOpeningBrace(line, column)
{
    var currentLine = line;
    var count = 1;

    var firstPos;
    var lastPos;
    var currentString;
    var firstChar;
    var lastChar;
    var indentation = -1;

    // note: no delimiter in this case, yet
    while (currentLine > 0 && count > 0) {
        --currentLine;
        firstPos = document.firstCharPos(currentLine);

        // skip empty lines
        if (firstPos == -1)
            continue; 

        currentString = document.line(currentLine);
        firstChar = currentString.charAt(firstPos);
        // idea: ignore // and c comments and also preprocessor
        if (firstChar != '/' && firstChar != '*' && firstChar != '#') {
            var braces = currentString.replace(/[^{}]+/g, "");
            var i;
            for (i = braces.length - 1; i >= 0; --i) {
                var currentChar = braces.charAt(i);
                if (currentChar == '{' ) {
                    --count;
                    if (count == 0)
                        break;
                } else {
                    ++count;
                }
            }
        }
    }

    if (count == 0)
    {
        debug("found matching { in line: " + currentLine);
        indentation = indentString(document.firstCharPosVirtual(currentLine));
    }

    if (indentation != -1) debug("findOpeningBrace: success");
    return indentation;
}
//END process character

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
 * return: leading whitespaces as string, or -1 if not in a string
 */
function inString(line)
{
    var currentLine = line;
    var currentString;

    // go line up as long as the previous line ends with an escape character '\'
    while (currentLine >= 0) {
        currentString = document.line(currentLine -1 );
        if (currentString.charAt(document.lastCharPos(currentLine - 1)) != '\\')
            break;
        --currentLine;
    }

    // iterate through all lines and toggle bool insideString everytime we hit a "
    var insideString = false;
    var indentation = "";
    while (currentLine < line) {
        currentString = document.line(currentLine);
        var char1;
        var i;
        var lineLength = document.lineLength(currentLine);
        for (i = 0; i < lineLength; ++i) {
            char1 = currentString.charAt(i);
            if (char1 == "\\") {
                ++i;
            } else if (char1 == "\"") {
                insideString = !insideString;
                if (insideString)
                    indentation = currentString.substring(0, document.firstCharPos(currentLine) + 1);
            }
        }
        ++currentLine;
    }

    return insideString ? indentation : -1;
}

//BEGIN process newline
/**
 * C comment checking. If the previous line begins with a "/*" or a "* ", then
 * return its leading white spaces + ' *' + the white spaces after the *
 * return: filler string or -1, if not in a C comment
 */
function tryCComment(line)
{
    var currentLine = line - 1;
    if (currentLine < 0)
        return -1;

    var currentString = document.line(currentLine);
    var lastPos = document.lastCharPos(currentLine);
    var indentation = -1;

    var notInCComment = false;
    if (lastPos == -1) {
        var lineDelimiter = gLineDelimiter;
        // empty line: now do the following
        // 1. search for non-empty line
        // 2. then break and continue with the check for "*/"
        notInCComment = true;
        // search non-empty line, then return leading white spaces
        while (currentLine >= 0 && lineDelimiter > 0) {
            lastPos = document.lastCharPos(currentLine);
            if (lastPos != -1) {
                break;
            }
            --currentLine;
            --lineDelimiter;
        }

        currentString = document.line(currentLine);
    }

    // we found a */, search the opening /* and return its indentation level
    if (currentString.charAt(lastPos) == '/'
        && currentString.charAt(lastPos - 1) == '*')
    {
        var startOfComment;
        var lineDelimiter = gLineDelimiter;
        while (currentLine >= 0 && lineDelimiter > 0) {
            currentString = document.line(currentLine);
            startOfComment = currentString.indexOf("/*");
            if (startOfComment != -1) {
                indentation = indentString(document.firstCharPosVirtual(currentLine));
                break;
            }
            --currentLine;
            --lineDelimiter;
        }
        if (indentation != -1) debug("tryCComment: success (1)");
        return indentation;
    }

    // there previously was an empty line, in this case we honor the circumstances
    if (notInCComment)
        return -1;

    var firstPos = firstNonSpace(currentString);
    var char1 = currentString.charAt(firstPos);
    var char2 = currentString.charAt(firstPos + 1);

    if (char1 == '/' && char2 == '*') {
        indentation = indentString(document.firstCharPosVirtual(currentLine));
        indentation += " * ";
    } else if (char1 == '*' && (char2 == '' || char2 == ' ' || char2 == '\t')) {
        currentString.search(/^\s*\*(\s*)/);
        // in theory, we could search for opening /*, and use its indentation
        // and then one alignment character. Let's not do this for now, though.
        var end = RegExp.$1;
        indentation =
            indentString(document.firstCharPosVirtual(currentLine)) + '*' + end;
        if (indentation.charAt(indentation.length - 1) == '*')
            indentation += ' ';
    }

    if (indentation != -1) debug("tryCComment: success (2)");
    return indentation;
}

/**
 * C++ comment checking. If the previous line begins with a "//", then
 * return its leading white spaces + '//'. Special treatment for:
 * //, ///, //! ///<, //!< and ////...
 * return: filler string or -1, if not in a star comment
 */
function tryCppComment(line)
{
    var currentLine = line - 1;
    if (currentLine < 0)
        return -1;

    var firstPos = document.firstCharPos(currentLine);

    if (firstPos == -1)
        return -1;

    var currentString = document.line(currentLine);
    var char1 = currentString.charAt(firstPos);
    var char2 = currentString.charAt(firstPos + 1);
    var indentation = -1;

    // allowed are: //, ///, //! ///<, //!< and ////...
    if (char1 == '/' && char2 == '/') {
        var char3 = currentString.charAt(firstPos + 2);
        var char4 = currentString.charAt(firstPos + 3);
        var firstPosVirtual = document.firstCharPosVirtual(currentLine);

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

    if (indentation != -1) debug("tryCppComment: success");
    return indentation;
}

/**
 * If the last non-empty line ends with a {, take its indentation level and
 * return it increased by 1 indetation level. If not found, return -1.
 */
function tryBrace(line)
{
    var currentLine = line - 1;
    if (currentLine < 0)
        return -1;

    var lastPos = -1;
    var lineDelimiter = gLineDelimiter;

    // search non-empty line
    while (currentLine >= 0 && lineDelimiter > 0) {
        lastPos = document.lastCharPos(currentLine);
        if (lastPos != -1) {
            break;
        }
        --currentLine;
        --lineDelimiter;
    }

    if (lastPos == -1)
        return -1;

    // found non-empty line
    var currentString = document.line(currentLine);
    var indentation = -1;

    if (currentString.charAt(lastPos) == '{') {
        // take its indentation and add one indentation level
        var firstPosVirtual = document.firstCharPosVirtual(currentLine);
        indentation = incIndent(firstPosVirtual);
    }

    if (indentation != -1) debug("tryBrace: success");
    return indentation;
}

/**
 * Check for if, else, while, do, switch, private, public, protected, signals,
 * default, case etc... keywords, as we want to indent then.
 * Note: The code is written to be called *after* tryCComment and tryCppComment!
 */
function tryCKeywords(line)
{
    var currentLine = line - 1;
    if (currentLine < 0)
        return -1;

    var lastPos = -1;
    var lineDelimiter = gLineDelimiter;

    // search non-empty line
    while (currentLine >= 0 && lineDelimiter > 0) {
        lastPos = document.lastCharPos(currentLine);
        if (lastPos != -1) {
            break;
        }
        --currentLine;
        --lineDelimiter;
    }

    if (lastPos == -1)
        return -1;

    // found non-empty line
    var currentString = document.line(currentLine);
    if (currentString.search(/^\s*(if\b|for|do\b|while|switch|[}]?\s*else|((private|public|protected|case|default|signals|Q_SIGNALS).*:))/) == -1)
        return -1;
//    var firstWord = RegExp.$1;
//    debug("Found first word: " + firstWord);
    var lastChar = currentString.charAt(lastPos);
    var indentation = -1;

    // try to ignore lines like: if (a) b; or if (a) { b; }
    if (lastChar != ';' && lastChar != '}') {
        // take its indentation and add one indentation level
        var firstPosVirtual = document.firstCharPosVirtual(currentLine);
        indentation = incIndent(firstPosVirtual);
    }

    if (indentation != -1) debug("tryCKeywords: success");
    return indentation;
}

/**
 * Search for if, do, while, for, ... as we want to indent then.
 * Return -1, if nothing useful found.
 * Note: The code is written to be called *after* tryCComment and tryCppComment!
 */
function tryCondition(line)
{
    var currentLine = line - 1;
    if (currentLine < 0)
        return -1;

    var lastPos = -1;
    var lineDelimiter = gLineDelimiter;

    // search non-empty line
    while (currentLine >= 0 && lineDelimiter > 0) {
        lastPos = document.lastCharPos(currentLine);
        if (lastPos != -1) {
            break;
        }
        --currentLine;
        --lineDelimiter;
    }

    if (lastPos == -1)
        return -1;

    // found non-empty line
    var currentString = document.line(currentLine);
    var lastChar = currentString.charAt(lastPos);
    var indentation = -1;

    if (lastChar == ';'
        && currentString.search(/^\s*(if\b|do\b|while\b|for)/) == -1)
    {
        // idea: we had something like:
        //   if/while/for (expression)
        //       statement();  <-- we catch this trailing ';'
        // Now, look for a line that starts with if/for/while, that has one
        // indent level less.
        var indentLevelVirtual = document.firstCharPosVirtual(currentLine);
        if (indentLevelVirtual == 0)
            return -1;

        var lineDelimiter = 10; // 10 limit search, hope this is a sane value
        while (currentLine > 0 && lineDelimiter > 0) {
            --currentLine;
            --lineDelimiter;

            var firstPosVirtual = document.firstCharPosVirtual(currentLine);
            if (firstPosVirtual == -1)
                continue;

            if (firstPosVirtual < indentLevelVirtual) {
                currentString = document.line(currentLine);
                if (currentString.search(/^\s*(if\b|do\b|while\b|for)[^{]*$/) != -1)
                    indentation = indentString(firstPosVirtual);
                break;
            }
        }
    }

    if (indentation != -1) debug("tryCondition: success");
    return indentation;
}

/**
 * Search non-empty line and return its indentation string or -1, if not found
 */
function keepIndentation(line)
{
    var currentLine = line - 1;

    if (currentLine < 0)
        return -1;

    var indentation = -1;
    var firstPosVirtual;
    var lineDelimiter = gLineDelimiter;

    // search non-empty line, then return leading white spaces
    while (currentLine >= 0 && lineDelimiter > 0) {
        firstPosVirtual = document.firstCharPosVirtual(currentLine);
        if (firstPosVirtual != -1) {
            indentation = indentString(firstPosVirtual);
            break;
        }
        --currentLine;
        --lineDelimiter;
    }

    if (indentation != -1) debug("keepIndentation: success");
    return indentation;
}

function indentNewLine()
{
    readSettings();

    var line = view.cursorLine();
    var column = view.cursorColumnReal();

    var currentString = document.line(line);
    var firstChar = currentString.charAt(document.firstCharPos(line));


    var filler = -1;

    if (filler == -1 && firstChar == '}')
        filler = findOpeningBrace(line, column);
    if (filler == -1)
        filler = tryCComment(line);
    if (filler == -1)
        filler = tryCppComment(line);
    if (filler == -1)
        filler = tryBrace(line);
    if (filler == -1 && firstChar != '{')
        filler = tryCKeywords(line);
    if (filler == -1)
        filler = tryCondition(line);

    // we don't know what to do, let's simply keep the indentation
    if (filler == -1)
        filler = keepIndentation(line);

    if (filler != -1) {
        var firstPos = document.firstCharPos(line);

        document.editBegin();
        if (firstPos > 0)
            document.removeText(line, 0, line, firstPos);
        document.insertText(line, 0, filler);
        view.setCursorPosition(line, filler.length);
        document.editEnd();

        // remember position of last action
        gLastLine = line;
        gLastColumn = filler.length;
    } else {
        gLastLine = -1;
        gLastColumn = -1;
    }
}
//END process newline

indenter.onchar=indentChar
indenter.online=indentNewLine
indenter.onnewline=indentNewLine

// kate: space-indent on; indent-width 4; replace-tabs on;
