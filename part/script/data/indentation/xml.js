/** kate-script
 * name: XML Style
 * license: LGPL
 * author: Milian Wolff <mail@milianw.de>, Gerald Senarclens de Grancy <oss@senarclens.eu>
 * revision: 2
 * kate-version: 3.10
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


// required katepart js libraries
require ("range.js");
require ("string.js");

var DEBUG = false;  // disable before checking in!

function dbg(s) {
    if (DEBUG)
        debug(s);
}

// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = ">";

// used regular expressions
// dear reader, please forgive me for using regular expressions in relation
// with xml/html

// match the beginning of any opening tag; special tags (<?..., <[... and <!...)
// are ignored
opening = /<[^\/?\[!]/g;
// matches the beginning of any closing tag
closing = /<\//g;
// matches if there is nothing but a single closing tag
single_closing_tag = /^<\/[^<>]*>$/;
open_tag = /<[^\[]/;  // starting of a non-CDATA tag
close_tag = /[^\]]>|^>/;  // end of a non-CDATA tag
// ignore indenting if a line is started with one of the following tags
ignore_tags = [/^<html/, /^<body/];


// Return the code string of the given lineNr (no comments or strings etc).
// Also strips self-closing tags.
// Eg.
// if document.line(x) is "  <p id="go">test</p> "
//   getCode(x) -> "<p>test</p>"
// if document.line(x) == "  <p id="go">test<br /></p> "
//   getCode(x) -> "<p>test</p>"
// if document.line(x) == "<p>test<!-- <em>comment</em> --></p>"
//   getCode(x) -> "<p>test</p>"
function getCode(lineNr) {
    var line = document.line(lineNr);
    var code = "";
    for (var col = 0; col < line.length; ++col) {
        if (document.isCode(lineNr, col))
            code += line[col];
    }
    code = replaceSelfClosing(code);
    return code.trim();
}


// Return given code with all self-closing tags removed.
function replaceSelfClosing(code) {
    return code.replace(/<[^<>]*\/>/g, '<tag></tag>');
}


// Check if the last line was endet inside a tag and return appropriate indent.
// If the last line wasn't endet inside a tag, return -1.
// Otherwise, return appropriate indent so that attributes are aligned.
function _calcAttributeIndent(lineNr, indentWidth) {
    var text = document.line(lineNr);
    var num_open_tag = text.countMatches(open_tag);
    var num_close_tag = text.countMatches(close_tag);
    if (num_open_tag > num_close_tag) {
        dbg("unfinished tag");
        for (col = text.lastIndexOf("<"); col < text.length; ++col) {
            if (document.isOthers(lineNr, col) &&
                document.isSpace(lineNr, col))
                return col + 1;
        }
    } else if (num_open_tag < num_close_tag) {
        dbg("closing unfinished tag");
        code = getCode(lineNr);
        do {
            lineNr--;
            var line = document.line(lineNr);
            code = getCode(lineNr) + code;
        } while ((line.countMatches(open_tag) <= line.countMatches(close_tag)) &&
                 (lineNr > 0));
        var prevIndent = Math.max(document.firstVirtualColumn(lineNr), 0);
        code = replaceSelfClosing(code);
        var steps = calcSteps(code);
        return prevIndent + indentWidth * steps;
    }
    return -1;  // by default, keep last line's indent (-1)
}


// Return the number of steps to indent/ un-indent.
// If the code is matched by any element of ignore_tags, 0 is returned.
function calcSteps(code) {
    for (var key in ignore_tags) {
        if (code.match(ignore_tags[key]))
            return 0;
    }
    var num_opening = code.match(opening) ? code.match(opening).length : 0;
    var num_closing = code.match(closing) ? code.match(closing).length : 0;
    return num_opening - num_closing;
}


// Return the amount of characters (in spaces) to be indented.
// Called for each newline (ch == '\n') and all characters specified in
// the global variable triggerCharacters. When calling Tools → Align
// the variable ch is empty, i.e. ch == ''.
// Special indent() return values:
//   -1: keep last indent
//   -2: do nothing
function indent(lineNr, indentWidth, char) {
    dbg("lineNr: " + lineNr + " indentWidth: " + indentWidth + " char: " + char);
    if (lineNr == 0)  // don't ever act on document's first line
        return -2;

    var lastLineNr = lineNr - 1;
    var lastLine = getCode(lastLineNr);
    dbg("lastLine1: " + lastLine);
    while (lastLine == "") {
        lastLineNr--;
        if (lastLineNr <= 0) {
            return -1;
        }
        lastLine = getCode(lastLineNr);
    }
    dbg("lastLine2: " + lastLine);

    // default action (for char == '\n' or char == '')
    var indent = _calcAttributeIndent(lastLineNr, indentWidth);
    if (indent != -1) {
        return indent;
    }
    dbg("indent: " + indent);

    indent = Math.max(document.firstVirtualColumn(lastLineNr), 0);
    dbg("indent: " + indent);


    var steps = calcSteps(lastLine);
    // unindenting separate closing tags are dealt with by last line
    if (steps && !lastLine.match(single_closing_tag))
        indent += indentWidth * steps;

    // set char if required (eg. in case of align or copy and paste)
    if (char == ">" || !char) {  // ok b/c of inner if
        // if there is nothing but a separate closing tag, unindent
        var curLine = getCode(lineNr);
        dbg("curLine: " + curLine);
        if (curLine.match(single_closing_tag)) {
            return Math.max(indent - indentWidth, 0);
        }
    }
    return Math.max(indent, -1);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
