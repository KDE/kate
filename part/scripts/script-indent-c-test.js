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
    var _i;
    var _char;
    for (_i = 0; _i < _text.length; ++_i) {
        _char = _text.charAt(_i);
        if (_char != ' ' && _char != '\t')
            return _i;
    }

    return -1;
}

/**
 * Get the position of the last non-space character
 * return: position or -1, if there is no non-space character
 */
function lastNonSpace(_text)
{
    var _i;
    var _char;
    for (_i = _text.length - 1; _i >= 0; --_i) {
        _char = _text.charAt(_i);
        if( _char != ' ' && _char != '\t' )
            return _i;
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
    while (_currentLine > 0) {
        _currentString = document.line(_currentLine - 1);
        if (_currentString.charAt(lastNonSpace(_currentString)) != '\\')
            break;
        --_currentLine;
    }

    // iterate through all lines and toggle bool _inString everytime we hit a "
    var _inString = false;
    var _indentation = "";
    while (_currentLine < _line) {
        _currentString = document.line(_currentLine);
        var _char1;
        var _i;
        for (_i = 0; _i < document.lineLength(_currentLine); ++_i) {
            _char1 = _currentString.charAt(_i);
            if (_char1 == "\\") {
                ++_i;
            } else if (_char1 == "\"") {
                _inString = !_inString;
                if (_inString)
                    _indentation = _currentString.substring(0, firstNonSpace(_currentString) + 1);
            }
        }
        ++_currentLine;
    }

    return _inString ? _indentation : -1;
}

/**
 * C comment checking. If the previous line begins with a "/*" or a "* ", then
 * return its leading white spaces + ' *' + the white spaces after the *
 * return: filler string or -1, if not in a star comment
 */
function inStarComment(_line)
{
    var _currentLine = _line - 1;
    if (_currentLine < 0)
        return -1;

    var _currentString = document.line(_currentLine);
    var _lastChar = lastNonSpace(_currentString);

    if (_lastChar == -1)
        return -1;

    if (_currentString.charAt(_lastChar) == "/"
        && _currentString.charAt(_lastChar - 1) == "*")
        return -1;

    var _firstChar = firstNonSpace(_currentString);
    var _char1 = _currentString.charAt(_firstChar);
    var _char2 = _currentString.charAt(_firstChar + 1);
    var _indentation = -1;

    if (_char1 == "/" && _char2 == "*") {
        _indentation = _currentString.match(/^\s*\/\*+\s*/);
        _indentation = _indentation[0];
        _indentation = _indentation.replace(/\//, " "); // replace / by " "
        if (_firstChar + 1 != lastNonSpace(_indentation)) {
            // more than just one star -> replace them and return 1 trailing space
            _indentation = _indentation.substring(0, _firstChar + 1) + "* ";
        }
    } else if (_char1 == "*" && (_char2 == "" || _char2 == ' ' || _char2 == '\t')) {
        _indentation = _currentString.match(/^\s*[*]\s*/);
        _indentation = _indentation[0];
    }

    return _indentation;
}

/**
 * C++ comment checking. If the previous line begins with a "//", then
 * return its leading white spaces + '//'. Special treatment for:
 * //, ///, //! ///<, //!< and ////...
 * return: filler string or -1, if not in a star comment
 */
function inCppComment(_line)
{
    var _currentLine = _line - 1;
    if (_currentLine < 0)
        return -1;

    var _currentString = document.line(_currentLine);
    var _firstChar = firstNonSpace(_currentString);

    if (_firstChar == -1)
        return -1;

    var _char1 = _currentString.charAt(_firstChar);
    var _char2 = _currentString.charAt(_firstChar + 1);
    var _indentation = -1;

    // allowed are: //, ///, //! ///<, //!< and /////////...
    if (_char1 == "/" && _char2 == "/") {
        var _char3 = _currentString.charAt(_firstChar + 2);
        var _char4 = _currentString.charAt(_firstChar + 3);

        if (_char3 == "/" && _char4 == "/") {
            // match ////... and replace by only two: //
            _indentation = _currentString.match(/^\s*\/\//);
        } else if (_char3 == "/" || _char3 == "!") {
            // match ///, //!, ///< and //!
            _indentation = _currentString.match(/^\s*\/\/[/!][<]?\s*/);
        } else {
            // only //, nothing else
            _indentation = _currentString.match(/^\s*\/\/\s*/);
        }
        _indentation = _indentation[0];
        if (lastNonSpace(_indentation) == _indentation.length - 1)
            _indentation += " ";
    }

    return _indentation;
}


/**
 * Check, whether the beginning of _line is inside a multiline comment
 * return: true or false
 */
// function inMultilineComment(_line)
// {
//     var _currentLine = _line - 1;
// 
//     if (_currentLine < 0)
//         return false;
// 
//     var _currentString = document.line(_line);
//     if (currentString)
// 
//     return false;
// }

function indentNewLine()
{
	var tabWidth = 4;
	var spaceIndent = true;
    var indentWidth = document.indentWidth;

	var intStartLine = view.cursorLine();
	var intStartColumn = view.cursorColumn();

    var filler = -1;

    if (filler == -1)
        filler = inStarComment(intStartLine);
    if (filler == -1)
        filler = inCppComment(intStartLine);

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
