/** kate-script
 * name: XML Style
 * license: LGPL
 * author: Milian Wolff <mail@milianw.de>
 * revision: 1
 * kate-version: 3.4
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

var debugMode = true;

// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = "/>";

function dbg(s) {
    if (debugMode)
        debug(s);
}

/**
 * Process a newline character.
 * This function is called whenever the user hits <return/enter>.
 *
 * It gets three arguments: line, indentwidth in spaces, typed character.
 */
function indent(line, indentWidth, char)
{
    var prevLine = line;
    var prevLineString = " ";
    while (prevLine >= 0 && prevLineString.match(/^\s+$/)) {
        prevLine--;
        prevLineString = document.line(prevLine);
    }
    var prevIndent = document.firstVirtualColumn(prevLine);
    var lineString = document.line(line);

    var alignOnly = (char == "");
    if (alignOnly) {
        // XML might be all in one line, in which case we
        // want to break that up.
        var tokens = lineString.split(/>\s*</);
        if (tokens.length > 1) {
            var oldLine = line;
            var oldPrevIndent = prevIndent;
            for (var l in tokens) {
                var newLine = tokens[l];
                if (l > 0) {
                    newLine = '<' + newLine;
                }
                if (l < tokens.length - 1) {
                    newLine += '>';
                }

                if (newLine.match(/^\s*<\//)) {
                    char = '/';
                } else if (newLine.match(/\>[^<>]*$/)) {
                    char = '>';
                } else {
                    char = '\n';
                }
                var indentation = processChar(line, newLine, prevLineString, prevIndent, char, indentWidth);
                prevIndent = indentation;
                while (indentation > 0) {
                    //TODO: what about tabs
                    newLine = " " + newLine;
                    --indentation;
                }
                ++line;
                prevLineString = newLine;
                tokens[l] = newLine;
            }
            dbg(tokens.join('\n'));
            dbg(oldLine);
            document.editBegin();
            document.truncate(oldLine, 0);
            document.insertText(oldLine, 0, tokens.join('\n'));
            document.editEnd();
            return oldPrevIndent;
        } else {
            if (lineString.match(/^\s*<\//)) {
                char = '/';
            } else if (lineString.match(/\>[^<>]*$/)) {
                char = '>';
            } else {
                char = '\n';
            }
        }
    }

    dbg(line);
    dbg(lineString);
    dbg(prevLineString);
    dbg(prevIndent);
    dbg(char);
    return processChar(line, lineString, prevLineString, prevIndent, char, indentWidth);
}

function processChar(line, lineString, prevLineString, prevIndent, char, indentWidth)
{
    if (char == '/') {
        if (!lineString.match(/^\s*<\//)) {
            // might happen when we have something like <foo bar="asdf/ycxv">
            // don't change indentation then
            return document.firstVirtualColumn(line);
        }
        if (!prevLineString.match(/<[^\/][^>]*[^\/]>[^<>]*$/)) {
            // decrease indent when we write </ and prior line did not start a tag
            return prevIndent - indentWidth;
        }
    } else if (char == '>') {
        // increase indent width when we write <...> or <.../> but not </...>
        // and the prior line didn't close a tag
        if (line == 0) {
            return 0;
        } else if (prevLineString.match(/^<(\?xml|!DOCTYPE)/)) {
            return 0;
        } else if (lineString.match(/^\s*<\//)) {
            // closing tag, decrease indentation when previous didn't open a tag
            if (prevLineString.match(/<[^\/][^>]*[^\/]>[^<>]*$/)) {
                // keep indent when prev line opened a tag
                return prevIndent;
            } else {
                return prevIndent - indentWidth;
            }
        } else if (prevLineString.match(/<([\/!][^>]+|[^>]+\/)>\s*$/)) {
            // keep indent when prev line closed a tag or was empty or a comment
            return prevIndent;
        }
        return prevIndent + indentWidth;
    } else if (char == '\n') {
        if (prevLineString.match(/^<(\?xml|!DOCTYPE)/)) {
            return 0;
        }
        if (prevLineString.match(/<[^\/!][^>]*[^\/]>[^<>]*$/)) {
            // increase indent when prev line opened a tag (but not for comments)
            return prevIndent + indentWidth;
        }
    }

    return prevIndent;
}

// kate: space-indent on; indent-width 4; replace-tabs on;