/** kate-script
 * name: Python
 * license: LGPL
 * author: Paul Giannaros <paul@giannaros.org>, Gerald Senarclens de Grancy <oss@senarclens.eu>
 * revision: 2
 * kate-version: 3.10
 */


// required katepart js libraries
require ("range.js");
require ("string.js");

openings = ['(', '[', '{'];
closings = [')', ']', '}'];  // requires same order as in openings
unindenters = ['continue', 'pass', 'raise', 'return']


// Return the given line without comments and leading or trailing whitespace.
// Eg.
// getCode(x) -> "for i in range(3):"
//     if document.line(x) == "  for i in range(3):"
// getCode(x) -> "for i in range(3):"
//     if document.line(x) == "for i in range(3):  "
// getCode(x) -> "for i in range(3):"
//     if document.line(x) == "for i in range(3):  # grand"
function getCode(lineNr) {
    var line = document.line(lineNr);
    var length = line.length;
    while (length > 0 && document.isComment(lineNr, length - 1)) {
        length--;
    }
    line = line.substr(0, length);  // strip the comment
    return line.trim();
}


// Return the indent if a opening bracket is not closed (incomplete sequence).
// The calculated intent is the innermost opening bracket's position plus 1.
// `lineNr`: the number of the line on which the brackets should be counted
function _calcOpeningIndent(lineNr) {
    var line = document.line(lineNr);
    var countClosing = new Array();
    closings.forEach(function(elem) {
        countClosing[elem] = 0;
    });
    for (i = line.length - 1; i >= 0; --i) {
        if (document.isComment(lineNr, i) || document.isString(lineNr, i))
            continue;
        if (closings.indexOf(line[i]) > -1)
            countClosing[line[i]]++;
        var index = openings.indexOf(line[i]);
        if (index > -1) {
            if (countClosing[closings[index]] == 0) {
                return i + 1;
            }
            countClosing[closings[index]]--;
        }
    }
    return -1;
}


// Return the indent if a closing bracket not opened (incomplete sequence).
// The intent is the same as on the line with the unmatched opening bracket.
// `lineNr`: the number of the line on which the brackets should be counted
function _calcClosingIndent(lineNr) {
    var line = document.line(lineNr);
    var countClosing = new Array();
    closings.forEach(function(elem) {
        countClosing[elem] = 0;
    });
    for (i = line.length - 1; i >= 0; --i) {
        if (document.isComment(lineNr, i) || document.isString(lineNr, i))
            continue;
        if (closings.indexOf(line[i]) > -1)
            countClosing[line[i]]++;
        var index = openings.indexOf(line[i]);
        if (index > -1)
            countClosing[closings[index]]--;
    }
    for (var key in countClosing) {
        if (countClosing[key] > 0) {  // unmatched closing bracket
            for (--lineNr; lineNr >= 0; --lineNr) {
                if (_calcOpeningIndent(lineNr) > -1)
                    return document.firstVirtualColumn(lineNr)
            }
        }
    }
    return -1;
}


// Returns the indent for mismatched (opening or closing) brackets.
// If there are no mismatched brackets, -1 is returned.
// `lineNr`: number of the line for which the indent is calculated
function calcBracketIndent(lineNr) {
    var indent = _calcOpeningIndent(lineNr - 1);
    if (indent > -1)
        return indent
    indent = _calcClosingIndent(lineNr - 1);
    if (indent > -1)
        return indent
    return -1;
}


// Return true if a single unindent should occur.
function shouldUnindent(LineNr) {
    lastLine = getCode(LineNr - 1);
    if (unindenters.indexOf(lastLine) >= 0 || lastLine.startsWith('raise ') ||
        lastLine.startsWith('return '))
        return 1;
    // unindent if the last line was indented b/c of a backslash
    if (LineNr >= 2) {
        secondLastLine = getCode(LineNr - 2);
        if (secondLastLine.length && secondLastLine.substr(-1) == "\\")
            return 1;
    }
    return 0;
}


// Return the amount of characters (in spaces) to be indented.
// Special indent() return values:
//   -2 = no indent
//   -1 = keep last indent
// Follow PEP8 for unfinished sequences and argument lists.
// Nested sequences are not implemented. (neither by Emacs' python-mode)
function indent(line, indentWidth, character) {
    if (line == 0)  // don't ever act on document's first line
        return -2;
    if (!document.line(line - 1).length)  // empty line
        return -2;
    var lastLine = getCode(line - 1);
    var lastChar = lastLine.substr(-1);

    // indent when opening bracket or backslash is at the end the previous line
    if (openings.indexOf(lastChar) >= 0 || lastChar == "\\") {
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }
    // continue, pass, raise, return etc. should unindent
    if (shouldUnindent(line))
        return Math.max(0, document.firstVirtualColumn(line - 1) - indentWidth);
    var indent = calcBracketIndent(line);
    if (lastLine.endsWith(':')) {
        if (indent > -1)
            return indent + indentWidth;
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }
    return indent;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
