/** kate-script
 * name: Ruby Indenter
 * license: LGPL
 * author: Robin Pedersen <robin.pedersen@runbox.com>
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
//END USER CONFIGURATION


// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = "cdefhilnrsuw}]";

// Indent after lines that match this regexp
var rxIndent = /^\s*(def|if|unless|for|while|until|class|module|else|elsif|case|when|begin|rescue|ensure|catch)\b/;

// Unindent lines that match this regexp
var rxUnindent = /^\s*((end|when|else|elsif|rescue|ensure)\b|[\]\}])(.*)$/;


// debug to the term in blue so that it's easier to make out amongst all
// of Kate's other debug output.
function dbg(s)
{
    debug("\u001B[44m" + s + "\u001B[0m");
}

function isStmtContinuing(line)
{
    // Check for operators at end of line
    var rx = /(\+|\-|\*|\/|\=|&&|\|\||and|or|\\)\s*$/;

    // TODO: Handle blank lines/comments after operators (but not after '\')

    return rx.test(document.line(line));
}

function findStmtStart(currLine)
{
    var p, l = currLine;
    do {
        if (l == 0) return 0;
        p = l;
        l = document.prevNonEmptyLine(l - 1);
    } while (isStmtContinuing(l));
    return p;
}

function isBlockStart(currLine)
{
    var rx0 = rxIndent;
    var rx1 = /(do|\{)(\s+\|.*\||\s*)$/;
    var currStr = document.line(currLine);

    return (rx0.test(currStr) || rx1.test(currStr))
}

function isBlockEnd(currLine)
{
    var currStr = document.line(currLine);

    return rxUnindent.test(currStr);
}

function findBlockStart(currLine)
{
    var nested = 0;
    var l = currLine;
    while (true) {
        if (l < 0) return -1;

        l = document.prevNonEmptyLine(l - 1);
        if (isBlockEnd(l)) {
            dbg("End line: " + l);
            nested++;
        }
        if (isBlockStart(l)) {
            dbg("Start line: " + l);
            if (nested == 0)
                return l;
            else
                nested--;
        }
    }
}

// check if the trigger characters are in the right context,
// otherwise running the indenter might be annoying to the user
function isValidTrigger(line, ch)
{
    if (ch == "" || ch == "\n")
        return true; // Explicit align or new line
    if (rxUnindent.test(document.line(line))) {
        if (RegExp.$3 == "")
            return true; // Exact match
    }
    return false;
}

// TODO: Make an API where multiline statements are passed around like this:
//
// function foo()
// {
//    return {start: 10, end: 20};
// }
//
// var ret = foo();
// var a = ret.start;
// var b = ret.end;
// dbg("a = " + a);
// dbg("b = " + b);

// indent gets three arguments: line, indentwidth in spaces,
// typed character indent
function indent(line, indentWidth, ch)
{
    if (!isValidTrigger(line, ch))
        return -2;

    var prevLine = document.prevNonEmptyLine(line - 1);
    if (prevLine < 0)
        return -2; // Can't indent the first line

    var prevStmt = findStmtStart(prevLine);
    var prevStmtStr = "";
    for (l = prevStmt; l <= prevLine; ++l) {
        prevStmtStr += document.line(l).replace(/\\?$/, '\n');
    }
    var prevStmtInd = document.firstVirtualColumn(prevStmt);

    // Handle indenting of multiline statements.
    // Manually indenting to be able to force spaces.
    if (isStmtContinuing(prevLine)) {
        var len = document.firstColumn(prevLine);
        var str = document.line(prevLine).substr(0, len);
        if (!document.startsWith(line, str)) {
            document.removeText(line, 0, line, document.firstColumn(line));
            if (prevStmt == prevLine)
                str += "    ";
            document.insertText(line, 0, str);
        }
        return -2;
    }

    var prevStmtStartAttr = document.attribute(prevStmt, prevStmtInd);
    dbg("StartAttr: " + prevStmtStartAttr);

    // Find comments
    if (prevStmtStartAttr == 30) {
        // Use same indent after comment
        return -1;
    }

    var string = document.line(line);

    if (rxUnindent.test(string)) {
        var startLine = findBlockStart(line);
        dbg("currLine: " + line);
        dbg("StartLine: " + startLine);
        if (startLine >= 0)
            return document.firstVirtualColumn(startLine);
        else
            return -2;
    } else if (ch != "" && ch != "\n") {
        // Ignore other typing
        return -2;
    }

    if (rxIndent.test(prevStmtStr)) {
        return prevStmtInd + indentWidth;
    } else {
        var p = prevStmtStr.search(/(do|\{)(\s+\|.*\||\s*)$/);
        if (p != -1) {
            return prevStmtInd + indentWidth;
        } else if (prevStmtStr.search(/[\[\{]\s*$/) != -1) {
            return prevStmtInd + indentWidth;
        }
    }

    // Keep current
    return -1;
}
