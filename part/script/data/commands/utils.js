/* kate-script
 * author: Dominik Haumann <dhdev@gmx.de>, Milian Wolff <mail@milianw.de>, Gerald Senarclens de Grancy <oss@senarclens.eu>
 * license: LGPL
 * revision: 6
 * kate-version: 3.4
 * functions: sort, moveLinesDown, moveLinesUp, natsort, uniq, rtrim, ltrim, trim, join, rmblank, unwrap, each, filter, map, duplicateLinesUp, duplicateLinesDown, rewrap
 */

// required katepart js libraries
require ("range.js");

function sort()
{
    each(function(lines){return lines.sort()});
}

function uniq()
{
    each(function(lines) {
        for ( var i = 1; i < lines.length; ++i ) {
          for ( var j = i - 1; j >= 0; --j ) {
            if ( lines[i] == lines[j] ) {
              lines.splice(i, 1);
              // gets increased in the for
              --i;
              break;
            }
          }
        }
        return lines;
    });
}

function natsort()
{
    each(function(lines){return lines.sort(natcompare);});
}

function rtrim()
{
    map(function(l){ return l.replace(/\s+$/, ''); });
}

function ltrim()
{
    map(function(l){ return l.replace(/^\s+/, ''); });
}

function trim()
{
    map(function(l){ return l.replace(/^\s+|\s+$/, ''); });
}

function rmblank()
{
    filter(function(l) { return l.length > 0; });
}

function join(separator)
{
    if (typeof(separator) != "string") {
        separator = "";
    }
    each(function(lines){
        return [lines.join(separator)];
    });
}

// unwrap does the opposite of the script word wrap
function unwrap ()
{
    var selectionRange = view.selection();
    if (selectionRange.isValid()) {
        // unwrap all paragraphs in the selection range
        var currentLine = selectionRange.start.line;
        var count = selectionRange.end.line - selectionRange.start.line;

        document.editBegin();
        while (count >= 0) {
            // skip empty lines
            while (count >= 0 && document.firstColumn(currentLine) == -1) {
                --count;
                ++currentLine;
            }

            // find block of text lines to join
            var anchorLine = currentLine;
            while (count >= 0 && document.firstColumn(currentLine) != -1) {
                --count;
                ++currentLine;
            }

            if (currentLine != anchorLine) {
                document.joinLines(anchorLine, currentLine - 1);
                currentLine -= currentLine - anchorLine - 1;
            }
        }
        document.editEnd();
    } else {
        // unwrap paragraph under the cursor
        var cursorPosition = view.cursorPosition();
        if (document.firstColumn(cursorPosition.line) != -1) {
            var startLine = cursorPosition.line;
            while (startLine > 0) {
                if (document.firstColumn(startLine - 1) == -1) {
                    break;
                }
                --startLine;
            }

            var endLine = cursorPosition.line;
            var lineCount = document.lines();
            while (endLine < lineCount) {
                if (document.firstColumn(endLine + 1) == -1) {
                    break;
                }
                ++endLine;
            }

            if (startLine != endLine) {
                document.editBegin();
                document.joinLines(startLine, endLine);
                document.editEnd();
            }
        }
    }
}

/// \note Range contains a block selected by lines (\b ONLY)!
/// And it is an open range! I.e. <em>[start, end)</em> (like in STL).
function _getBlockForAction()
{
    // Check if selection present in a view...
    var blockRange = Range(view.selection());
    var cursorPosition = view.cursorPosition();
    if (blockRange.isValid()) {
        blockRange.start.column = 0;
        if (blockRange.end.column != 0)
            blockRange.end.line++;
        blockRange.end.column = 0;
    } else {
        // No, it doesn't! Ok, lets select the current line only
        // from current position to the end
        blockRange = new Range(cursorPosition.line, 0, cursorPosition.line + 1, 0);
    }
    return blockRange;
}

// Adjusts ("moves") the current selection by offset.
// Positive offsets move down, negatives up.
function _adjustSelection(selection, offset)
{
    if (selection.end.line + offset < document.lines()) {
        selection = new Range(selection.start.line + offset, selection.start.column,
                              selection.end.line + offset, selection.end.column);
    } else {
        selection = new Range(selection.start.line + offset, selection.start.column,
                              selection.end.line + offset - 1,
                              document.lineLength(selection.end.line + offset - 1));
    }
    view.setSelection(selection);
}

function moveLinesDown()
{
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    // Check is there a space to move?
    if (blockRange.end.line < document.lines()) {
        document.editBegin();
        // Move a block to one line down:
        // 0) take one line after the block
        var text = document.line(blockRange.end.line);
        // 1) remove the line after the block
        document.removeLine(blockRange.end.line);
        // 2) insert a line before the block
        document.insertLine(blockRange.start.line, text);
        document.editEnd();
        if (view.hasSelection())
            _adjustSelection(selection, 1);
    }
}

function moveLinesUp()
{
    var cursor = view.cursorPosition();
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    // Check is there a space to move?
    if (0 < blockRange.start.line) {
        document.editBegin();
        // Move a block to one line up:
        // 0) take one line before the block,
        var text = document.line(blockRange.start.line - 1);
        // 1) and insert it after the block
        document.insertLine(blockRange.end.line, text);
        // 2) remove the original line
        document.removeLine(blockRange.start.line - 1);
        view.setCursorPosition(cursor.line - 1, cursor.column);
        if (view.hasSelection())
            _adjustSelection(selection, -1);
        document.editEnd();
    }
}

function duplicateLinesDown()
{
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    document.editBegin();
    document.insertText(blockRange.start, document.text(blockRange));
    _adjustSelection(selection, 1);
    document.editEnd();
}

function duplicateLinesUp()
{
    var cursor = view.cursorPosition();
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    document.editBegin();
    if (blockRange.end.line == document.lines()) {
        var lastLine = document.lines() - 1;
        var lastCol = document.lineLength(document.lines() - 1);
        blockRange.end.line = lastLine;
        blockRange.end.column = lastCol;
        document.insertText(lastLine, lastCol, document.text(blockRange));
        document.wrapLine(lastLine, lastCol);
    } else {
        document.insertText(blockRange.end, document.text(blockRange));
    }
    view.setCursorPosition(cursor.line, cursor.column);
    _adjustSelection(selection, 0);
    document.editEnd();
}

function rewrap()
{
    // initialize line span
    var fromLine = view.cursorPosition().line;
    var toLine = fromLine;
    var hasSelection = view.hasSelection();

    // if a text selection is present, use it to reformat the paragraph
    if (hasSelection) {
        var range = view.selection();
        fromLine = range.start.line;
        toLine = range.end.line;
    } else {
        // abort, if the cursor is in an empty line
        if (document.firstColumn(fromLine) == -1)
            return;

        // no text selection present: search for start & end of paragraph
        while (fromLine > 0 &&
            document.prevNonEmptyLine(fromLine-1) == fromLine - 1) --fromLine;
        while (toLine < document.lines() - 1 &&
            document.nextNonEmptyLine(toLine+1) == toLine + 1) ++toLine;
    }

    // initialize wrap columns
    var wrapColumn = 80;
    var softWrapColumn = 82;
    var exceptionColumn = 70;

    document.editBegin();

    if (fromLine < toLine) {
        document.joinLines(fromLine, toLine);
    }

    var line = fromLine;
    // as long as the line is too long...
    while (document.lastColumn(line) > wrapColumn) {
        // ...search for current word boundaries...
        var range = document.wordRangeAt(line, wrapColumn);
        if (!range.isValid()) {
            break;
        }

        // ...and wrap at a 'smart' position
        var wrapCursor = range.start;
        if (range.start.column < exceptionColumn && range.end.column <= softWrapColumn) {
            wrapCursor = range.end;
        }
        if (!document.wrapLine(wrapCursor))
            break;
        ++line;
    }

    document.editEnd();
}

function action(cmd)
{
    var a = new Object();
    a.icon = "";
    a.category = i18n("Editing");
    a.interactive = false;
    if (cmd == "sort") {
        a.text = i18n("Sort Selected Text");
        a.shortcut = "";
    } else if (cmd == "moveLinesDown") {
        a.text = i18n("Move Lines Down");
        a.shortcut = "Ctrl+Shift+Down";
    } else if (cmd == "moveLinesUp") {
        a.text = i18n("Move Lines Up");
        a.shortcut = "Ctrl+Shift+Up";
    } else if (cmd == "duplicateLinesUp") {
        a.text = i18n("Duplicate Selected Lines Up");
        a.shortcut = "Ctrl+Alt+Up";
    } else if (cmd == "duplicateLinesDown") {
        a.text = i18n("Duplicate Selected Lines Down");
        a.shortcut = "Ctrl+Alt+Down";
    }

    return a;
}

function help(cmd)
{
    if (cmd == "sort") {
        return i18n("Sort the selected text or whole document.");
    } else if (cmd == "moveLinesDown") {
        return i18n("Move selected lines down.");
    } else if (cmd == "moveLinesUp") {
        return i18n("Move selected lines up.");
    } else if (cmd == "uniq") {
        return i18n("Remove duplicate lines from the selected text or whole document.");
    } else if (cmd == "natsort") {
        return i18n("Sort the selected text or whole document in natural order.<br>Here is an example to show the difference to the normal sort method:<br>sort(a10, a1, a2) => a1, a10, a2<br>natsort(a10, a1, a2) => a1, a2, a10");
    } else if (cmd == "rtrim") {
        return i18n("Trims trailing whitespace from selection or whole document.");
    } else if (cmd == "ltrim") {
        return i18n("Trims leading whitespace from selection or whole document.");
    } else if (cmd == "trim") {
        return i18n("Trims leading and trailing whitespace from selection or whole document.");
    } else if (cmd == "join") {
        return i18n("Joins selected lines or whole document. Optionally pass a separator to put between each line:<br><code>join ', '</code> will e.g. join lines and separate them by a comma.");
    } else if (cmd == "rmblank") {
        return i18n("Removes empty lines from selection or whole document.");
    } else if (cmd == "unwrap") {
        return "Unwraps all paragraphs in the text selection, or the paragraph under the text cursor if there is no selected text.";
    } else if (cmd == "each") {
        return i18n("Given a JavaScript function as argument, call that for the list of (selected) lines and replace them with the return value of that callback.<br>Example (join selected lines):<br><code>each 'function(lines){return lines.join(\", \");}'</code><br>To save you some typing, you can also do this to achieve the same:<br><code>each 'lines.join(\", \")'</code>");
    } else if (cmd == "filter") {
        return i18n("Given a JavaScript function as argument, call that for the list of (selected) lines and remove those where the callback returns false.<br>Example (see also <code>rmblank</code>):<br><code>filter 'function(l){return l.length > 0;}'</code><br>To save you some typing, you can also do this to achieve the same:<br><code>filter 'line.length > 0'</code>");
    } else if (cmd == "map") {
        return i18n("Given a JavaScript function as argument, call that for the list of (selected) lines and replace the line with the return value of the callback.<br>Example (see also <code>ltrim</code>):<br><code>map 'function(line){return line.replace(/^\s+/, \"\");}'</code><br>To save you some typing, you can also do this to achieve the same:<br><code>map 'line.replace(/^\s+/, \"\")'</code>");
    } else if (cmd == "duplicateLinesUp") {
        return i18n("Duplicates the selected lines up.");
    } else if (cmd == "duplicateLinesDown") {
        return i18n("Duplicates the selected lines down.");
    }
}

/// helper code below:

function __toFunc(func, defaultArgName)
{
    if ( typeof(func) != "function" ) {
        try {
            func = eval("(" + func + ")");
        } catch(e) {}
        debug(func, typeof(func))
        if ( typeof(func) != "function" ) {
            try {
                // one more try to support e.g.:
                // map 'l+l'
                // or:
                // each 'lines.join("\n")'
                func = eval("(function(" + defaultArgName + "){ return " + func + ";})");
                debug(func, typeof(func))
            } catch(e) {}
            if ( typeof(func) != "function" ) {
                throw "parameter is not a valid JavaScript callback function: " + typeof(func);
            }
        }
    }
    return func;
}

function each(func)
{
    func = __toFunc(func, 'lines');

    var selection = view.selection();
    if (!selection.isValid()) {
        // use whole range
        selection = document.documentRange();
    } else {
        selection.start.column = 0;
        selection.end.column = document.lineLength(selection.end.line);
    }

    var text = document.text(selection);

    var lines = text.split("\n");
    lines = func(lines);
    if ( typeof(lines) == "object" ) {
      text = lines.join("\n");
    } else if ( typeof(lines) == "string" ) {
      text = lines
    } else {
      throw "callback function for each has to return object or array of lines";
    }

    view.clearSelection();

    document.editBegin();
    document.removeText(selection);
    document.insertText(selection.start, text);
    document.editEnd();
}

function filter(func)
{
    each(function(lines) { return lines.filter(__toFunc(func, 'line')); });
}

function map(func)
{
    each(function(lines) { return lines.map(__toFunc(func, 'line')); });
}

/*
natcompare.js -- Perform 'natural order' comparisons of strings in JavaScript.
Copyright (C) 2005 by SCK-CEN (Belgian Nucleair Research Centre)
Written by Kristof Coomans <kristof[dot]coomans[at]sckcen[dot]be>

Based on the Java version by Pierre-Luc Paour, of which this is more or less a straight conversion.
Copyright (C) 2003 by Pierre-Luc Paour <natorder@paour.com>

The Java version was based on the C version by Martin Pool.
Copyright (C) 2000 by Martin Pool <mbp@humbug.org.au>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


function isWhitespaceChar(a)
{
    var charCode;
    charCode = a.charCodeAt(0);

    if ( charCode <= 32 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

function isDigitChar(a)
{
    var charCode;
    charCode = a.charCodeAt(0);

    if ( charCode >= 48  && charCode <= 57 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

function compareRight(a,b)
{
    var bias = 0;
    var ia = 0;
    var ib = 0;

    var ca;
    var cb;

    // The longest run of digits wins.  That aside, the greatest
    // value wins, but we can't know that it will until we've scanned
    // both numbers to know that they have the same magnitude, so we
    // remember it in BIAS.
    for (;; ia++, ib++) {
        ca = a.charAt(ia);
        cb = b.charAt(ib);

        if (!isDigitChar(ca)
                && !isDigitChar(cb)) {
            return bias;
        } else if (!isDigitChar(ca)) {
            return -1;
        } else if (!isDigitChar(cb)) {
            return +1;
        } else if (ca < cb) {
            if (bias == 0) {
                bias = -1;
            }
        } else if (ca > cb) {
            if (bias == 0)
                bias = +1;
        } else if (ca == 0 && cb == 0) {
            return bias;
        }
    }
}

function natcompare(a,b) {

    var ia = 0, ib = 0;
        var nza = 0, nzb = 0;
        var ca, cb;
        var result;

    while (true)
    {
        // only count the number of zeroes leading the last number compared
        nza = nzb = 0;

        ca = a.charAt(ia);
        cb = b.charAt(ib);

        // skip over leading spaces or zeros
        while ( isWhitespaceChar( ca ) || ca =='0' ) {
            if (ca == '0') {
                nza++;
            } else {
                // only count consecutive zeroes
                nza = 0;
            }

            ca = a.charAt(++ia);
        }

        while ( isWhitespaceChar( cb ) || cb == '0') {
            if (cb == '0') {
                nzb++;
            } else {
                // only count consecutive zeroes
                nzb = 0;
            }

            cb = b.charAt(++ib);
        }

        // process run of digits
        if (isDigitChar(ca) && isDigitChar(cb)) {
            if ((result = compareRight(a.substring(ia), b.substring(ib))) != 0) {
                return result;
            }
        }

        if (ca == 0 && cb == 0) {
            // The strings compare the same.  Perhaps the caller
            // will want to call strcmp to break the tie.
            return nza - nzb;
        }

        if (ca < cb) {
            return -1;
        } else if (ca > cb) {
            return +1;
        }

        ++ia; ++ib;
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
