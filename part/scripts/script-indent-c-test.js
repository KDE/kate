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

/**
 * call in indentNewLine and indentChar to read document/view variables like
 * tab width, indent width, expand tabs etc.
 */
function readSettings()
{
    gTabWidth = document.tabWidth;
    gIndentWidth = document.indentWidth;
    gExpandTab = document.replaceTabs;

    var i;

    gIndentFiller = "";
    for (i = 0; i < gIndentWidth; ++i)
        gIndentFiller += " ";

    gTabFiller = "";
    for (i = 0; i < gTabWidth; ++i)
        gTabFiller += " ";
}
//END global variables and functions

/*
function indentChar() // also possible
{*/

function indentChar(c)
{
    var tabWidth = 4;
    var spaceIndent = true;
    var indentWidth = 4;


    var line = view.cursorLine();
    var col = view.cursorColumn();

    var textLine = document.line( line );
    var prevLine = document.line( line - 1 );

    var prevIndent = prevLine.match(/^\s*/);
    var addIndent = "";

    function unindent()
    {
    //     if (
    }

    // unindent } and {, if not in a comment
    if ( textLine.search( /^\s*\/\// ) == -1 )
    {
        if ( /*textLine.charAt( col-1 )*/ c == '}' || /*textLine.c( col-1 )*/ c == '{')
        {
            if ( textLine.search(/^\s\s\s\s/) != -1)
            {
               document.removeText( line, 0, line, tabWidth );
               view.setCursorPosition( line, col - tabWidth );
          }
       }
    }

}


/**
 * Get the position of the first non-space character
 * return: position or -1, if there is no non-space character
 */
function firstNonSpace(_text)
{
    if (_text && _text.search(/^(\s*)/) != -1) {
        if (RegExp.$1.length != _text.length)
            return RegExp.$1.length;
    }
    return -1;
}

/**
 * Get the position of the last non-space character
 * return: position or -1, if there is no non-space character
 */
function lastNonSpace(_text)
{
    if (_text && _text.search(/(\S\s*)$/) != -1) {
        if (RegExp.$1.length != _text.length)
        return _text.length - RegExp.$1.length;
    }
    return -1;
}

/**
 * Check, whether the beginning of _line is inside a "..." string context.
 * Note: the function does not check for comments
 * return: leading whitespaces as string, or -1 if not in a string
 */
function inString(_line)
{
    var _currentLine = _line;
    var _currentString;

    // go line up as long as the previous line ends with an escape character '\'
    while (_currentLine >= 0) {
        _currentString = document.line(_currentLine -1 );
        if (_currentString.charAt(document.lastChar(_currentLine - 1)) != '\\')
            break;
        --_currentLine;
    }

    // iterate through all lines and toggle bool _inString everytime we hit a "
    var _inString = false;
    var _indentation = "";
    while (_currentLine < _line) {
        _currentString = document.line(_currentLine);
        var _char1;
        var i;
        var _length = document.lineLength(_currentLine);
        for (i = 0; i < _length; ++i) {
            _char1 = _currentString.charAt(i);
            if (_char1 == "\\") {
                ++i;
            } else if (_char1 == "\"") {
                _inString = !_inString;
                if (_inString)
                    _indentation = _currentString.substring(0, document.firstChar(_currentLine) + 1);
            }
        }
        ++_currentLine;
    }

    return _inString ? _indentation : -1;
}

/**
 * C comment checking. If the previous line begins with a "/*" or a "* ", then
 * return its leading white spaces + ' *' + the white spaces after the *
 * return: filler string or -1, if not in a C comment
 */
function tryCComment(_line)
{
    var _currentLine = _line - 1;
    if (_currentLine < 0)
        return -1;

    var _currentString = document.line(_currentLine);
    var _lastPos = document.lastChar(_currentLine);
    var _indentation = -1;

    var _notInCComment = false;
    if (_lastPos == -1) {
        var _lineDelimiter = gLineDelimiter;
        // empty line: now do the following
        // 1. search for non-empty line
        // 2. then break and continue with the check for "*/"
        _notInCComment = true;
        // search non-empty line, then return leading white spaces
        while (_currentLine >= 0 && _lineDelimiter > 0) {
            _lastPos = document.lastChar(_currentLine);
            if (_lastPos != -1) {
                break;
            }
            --_currentLine;
            --_lineDelimiter;
        }

        _currentString = document.line(_currentLine);
    }

    // we found a */, search the opening /* and return its indentation level
    if (_currentString.charAt(_lastPos) == "/"
        && _currentString.charAt(_lastPos - 1) == "*")
    {
        var _startOfComment;
        var _lineDelimiter = gLineDelimiter;
        while (_currentLine >= 0 && _lineDelimiter > 0) {
            _currentString = document.line(_currentLine);
            _startOfComment = _currentString.indexOf("/*");
            if (_startOfComment != -1) {
                _indentation = _currentString.substring(0, document.firstChar(_currentLine));
                break;
            }
            --_currentLine;
            --_lineDelimiter;
        }
        return indentString(_indentation);
    }

    // there previously was an empty line, in this case we honor the circumstances
    if (_notInCComment)
        return -1;

    var _firstPos = firstNonSpace(_currentString);
    var _char1 = _currentString.charAt(_firstPos);
    var _char2 = _currentString.charAt(_firstPos + 1);
    var _endOfComment = _currentString.indexOf("*/");

    if (_char1 == "/" && _char2 == "*") {
        _currentString.search(/^(\s*)/);
        _indentation = indentString(RegExp.$1);
        _indentation += " * ";
    } else if (_char1 == "*" && (_char2 == "" || _char2 == ' ' || _char2 == '\t')) {
        _currentString.search(/^(\s*)[*](\s*)/);
        // in theory, we could search for opening /*, and use its indentation
        // and then one alignment character. Let's not do this for now, though.
        var end = RegExp.$2;
        _indentation = indentString(RegExp.$1) + "*" + end;
        if (_indentation.charAt(_indentation.length - 1) == '*')
            _indentation += " ";
    }

    return _indentation;
}

/**
 * C++ comment checking. If the previous line begins with a "//", then
 * return its leading white spaces + '//'. Special treatment for:
 * //, ///, //! ///<, //!< and ////...
 * return: filler string or -1, if not in a star comment
 */
function tryCppComment(_line)
{
    var _currentLine = _line - 1;
    if (_currentLine < 0)
        return -1;

    var _firstPos = document.firstChar(_currentLine);

    if (_firstPos == -1)
        return -1;

    var _currentString = document.line(_currentLine);
    var _char1 = _currentString.charAt(_firstPos);
    var _char2 = _currentString.charAt(_firstPos + 1);
    var _indentation = -1;

    // allowed are: //, ///, //! ///<, //!< and ////...
    if (_char1 == "/" && _char2 == "/") {
        var _char3 = _currentString.charAt(_firstPos + 2);
        var _char4 = _currentString.charAt(_firstPos + 3);

        if (_char3 == "/" && _char4 == "/") {
            // match ////... and replace by only two: //
            _currentString.search(/^(\s*)(\/\/)/);
        } else if (_char3 == "/" || _char3 == "!") {
            // match ///, //!, ///< and //!
            _currentString.search(/^(\s*)(\/\/[/!][<]?\s*)/);
        } else {
            // only //, nothing else
            _currentString.search(/^(\s*)(\/\/\s*)/);
        }
        var _ending = RegExp.$2;
        _indentation = indentString(RegExp.$1) + _ending;
        if (lastNonSpace(_indentation) == _indentation.length - 1)
            _indentation += " ";
    }

    return _indentation;
}

/**
 * If the last non-empty line ends with a {, take its indentation level and
 * return it increased by 1 indetation level. If not found, return -1.
 */
function tryBrace(_line)
{
    var _currentLine = _line - 1;
    if (_currentLine < 0)
        return -1;

    var _lastPos = -1;
    var _lineDelimiter = gLineDelimiter;

    // search non-empty line
    while (_currentLine >= 0 && _lineDelimiter > 0) {
        _lastPos = document.lastChar(_currentLine);
        if (_lastPos != -1) {
            break;
        }
        --_currentLine;
        --_lineDelimiter;
    }

    if (_lastPos == -1)
        return -1;

    // found non-empty line
    var _currentString = document.line(_currentLine);
    var _indentation = -1;
    var _char = _currentString.charAt(_lastPos);
    if (_char == "{") {
        // take its indentation and add one indentation level
        var _firstPos = document.firstChar(_currentLine);
        _indentation = incIndent(_currentString.substring(0, _firstPos));
    }

    return _indentation;
}

/**
 * Return indentation filler string.
 * If expandTab is true, the returned filler only contains spaces.
 * If expandTab is false, the returned filler contains tabs upto _alignment
 * if possible, and from _alignment to the end of _text spaces. This implements
 * mixed indentation, i.e. indentation+alignment.
 */
function indentString(_text, _alignment)
{
    if (_text == -1)
        return -1;

    var _indentation = _text.replace(/\t/g, gTabFiller);
    if (!gExpandTab) {
        var i;
        var _alignFiller = "";
        if (_alignment && _alignment >= 0 && _alignment < _indentation.length) {
            for (i = _alignment; i < _indentation.length; ++i)
                _alignFiller += " ";

            _indentation = _indentation.substring(0, _alignment);
        }

        var _spaceCount = _indentation.length % gTabWidth;
        var _tabCount = (_indentation.length - _spaceCount) / gTabWidth;
        _indentation = "";
        for (i = 0; i < _tabCount; ++i)
            _indentation += "\t";
        for (i = 0; i < _spaceCount; ++i)
            _indentation += " ";
        _indentation += _alignFiller;
    }

    return _indentation;
}

/**
 * Adds _levels times an indentation level. If _alignment is valid, mixed
 * indentation is turned on, see indentString() for further details.
 */
function incIndent(_text, _levels, _alignment)
{
    if (!_levels)
        _levels = 1;

    var i;
    for (i = 0; i < _levels; ++i)
        _text += gIndentFiller;

    return indentString(_text, _alignment);
}

/**
 * Search non-empty line and return its indentation string or -1, if not found
 */
function keepIndentation(_line)
{
    var _currentLine = _line - 1;

    if (_currentLine < 0)
        return -1;

    var _indentation = -1;
    var _firstPos;
    var _lineDelimiter = gLineDelimiter;

    // search non-empty line, then return leading white spaces
    while (_currentLine >= 0 && _lineDelimiter > 0) {
        _firstPos = document.firstChar(_currentLine);
        if (_firstPos != -1) {
            var _currentString = document.line(_currentLine);
            _indentation = indentString(_currentString.substring(0, _firstPos));
            break;
        }
        --_currentLine;
        --_lineDelimiter;
    }

    return _indentation;
}

function indentNewLine()
{
    readSettings();

    var intStartLine = view.cursorLine();
    var intStartColumn = view.cursorColumn();

    var filler = -1;

    if (filler == -1)
        filler = tryCComment(intStartLine);
    if (filler == -1)
        filler = tryCppComment(intStartLine);
    if (filler == -1)
        filler = tryBrace(intStartLine);

    // we don't know what to do, let's simply keep the indentation
    if (filler == -1)
        filler = keepIndentation(intStartLine);

    if (filler != -1) {
        var _currentString = document.line(intStartLine);
        var _leadingSpaces = _currentString.match(/^\s+/);
        var _removeText = 0;
        if (_leadingSpaces && _leadingSpaces.length > 0)
            _removeText = _leadingSpaces[0].length;

        document.editBegin();
        if (_removeText > 0)
            document.removeText(intStartLine, 0, intStartLine, _removeText);
        document.insertText(intStartLine, 0, filler);
        view.setCursorPosition(intStartLine, filler.length);
        document.editEnd();
    }
}

indenter.onchar=indentChar
indenter.onnewline=indentNewLine
