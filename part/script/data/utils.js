/* kate-script
 * author: Dominik Haumann <dhdev@gmx.de>
 * license: LGPL
 * revision: 2
 * kate-version: 3.4
 * type: commands
 * functions: sort, natsort, uniq, rtrim, ltrim, trim, join, test
 */

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
    each(function(lines){
        for ( var i = 0; i < lines.length; ++i ) {
            lines[i] = lines[i].replace(/\s+$/, '');
        }
        return lines;
    });
}

function ltrim()
{
    each(function(lines){
        for ( var i = 0; i < lines.length; ++i ) {
            lines[i] = lines[i].replace(/^\s+/, '');
        }
        return lines;
    });
}

function trim()
{
    each(function(lines){
        for ( var i = 0; i < lines.length; ++i ) {
            lines[i] = lines[i].replace(/^\s+|\s+$/, '');
        }
        return lines;
    });
}

///TODO: make it possible to pass a deliminator
///      but this requires a better cmd interpretation
///      in katepart. currently e.g. `join ' | '` passes three
///      arguments (', |, ') to the function, instead one with
///      the whitespaces...
function join()
{
    each(function(lines){
        return [lines.join("")];
    });
}

function test()
{
    var start = new Cursor(3, 6);
    var end = new Cursor(start);
    end.column = 12;

    var range = new Range(start, end);
    debug("line: " + end.line);
    debug("column: " + end.column);
    debug("range:" + range);
    debug("range:" + range.start.line);
    debug("range:" + range.start.column);
    debug("range:" + range.end.line);
    debug("range:" + range.end.column);

    view.setCursorPosition(start);
}

function help(cmd)
{
    if (cmd == "sort") {
        return "Sort the selected text or whole document.";
    } else if (cmd == "uniq") {
        return "Remove duplicate lines from the selected text or whole document.";
    } else if (cmd == "natsort") {
        return "Sort the selected text or whole document in natural order.<br>"
              +"Here's an example to show the difference to the normal sort method:<br>"
              +"sort(a10, a1, a2) => a1, a10, a2<br>"
              +"natsort(a10, a1, a2) => a1, a2, a10";
    } else if (cmd == "rtrim") {
        return "Trims trailing whitespace from selection or whole document.";
    } else if (cmd == "ltrim") {
        return "Trims leading whitespace from selection or whole document.";
    } else if (cmd == "trim") {
        return "Trims leading and trailing whitespace from selection or whole document.";
    } else if (cmd == "join") {
        return "Joins selected lines or whole document.";
    }
}

/// helper code below:

function each(func)
{
    if ( typeof(func) != "function" ) {
        debug("parameter is not a function: " + typeof(func));
        return;
    }

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
    text = lines.join("\n");

    view.clearSelection();

    document.editBegin();
    document.removeText(selection);
    document.insertText(selection.start, text);
    document.editEnd();
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

