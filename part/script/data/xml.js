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
    var alignOnly = (char == "");
    if (alignOnly) {
        var lineString = document.line(line);
        if (lineString.match(/^\s*<\//)) {
            char = '/';
        } else if (lineString.match(/\>[^<>]*$/)) {
            char = '>';
        }
    }

    if (char == '/' || char == '>') {
        return processChar(line, char, indentWidth);
    } else if(char == '\n' ) {
        var prefLineString = line ? document.line(line-1) : "";
        if (prefLineString.match(/<[^\/>]+>[^<>]*$/)) {
            // increase indent when pref line opened a tag
            return document.firstVirtualColumn(line-1) + indentWidth;
        }
    }

    // keep indent width
    return document.firstVirtualColumn(line-1);
}

function processChar(line, char, indentWidth)
{
    var lineString = document.line(line);
    var prefLineString = line ? document.line(line-1) : "";

    if (char == '/') {
        if (lineString.match(/^\s*<\//) && !prefLineString.match(/<[^/>]+>[^<>]*$/)) {
            // decrease indent when we write </ and prior line did not start a tag
            return document.firstVirtualColumn(line-1) - indentWidth;
        }
    } else if (char == '>') {
        // increase indent width when we write <...> or <.../> but not </...>
        // and the prior line didn't close a tag
        var prefIndent = document.firstVirtualColumn(line-1);
        if (line == 0) {
            return 0;
        } else if (prefLineString.match(/^<\?xml/)) {
            return 0;
        } else if (lineString.match(/^\s*<\//)) {
            // closing tag, decrease indentation when previous didn't open tag
            if (prefLineString.match(/<[^\/>]+>[^<>]*$/)) {
                // keep indent
                return prefIndent;
            } else {
                return prefIndent - indentWidth;
            }
        } else if (prefLineString.match(/<\/[^\/>]+>\s*$/)) {
            // keep indent when pref line closed a tag
            return prefIndent;
        }
        return prefIndent + indentWidth;
    }

    return -1;
}

// kate: space-indent on; indent-width 4; replace-tabs on;