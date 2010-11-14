/** kate-script
 * name: LISP
 * license: LGPL
 * author: Dominik Haumann <dhdev@gmx.de>
 * revision: 2
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

triggerCharacters = ";";

/**
 * Process a newline character.
 * This function is called whenever the user hits <return/enter>.
 */
function indent(line, indentWidth, ch)
{
    // special rules: ;;; -> indent 0
    //                ;;  -> align with next line, if possible
    //                ;   -> usually on the same line as code -> ignore

    textLine = document.line(line);
    if (textLine.search(/^\s*;;;/) != -1) {
        return 0;
    } else if (textLine.search(/^\s*;;/) != -1) {
        // try to align with the next line
        nextLine = document.nextNonEmptyLine(line + 1);
        if (nextLine != -1) {
            return document.firstVirtualColumn(nextLine);
        }
    }

    cursor = document.anchor(line, 0, '(');
    if (cursor.isValid()) {
        return document.toVirtualColumn(cursor) + indentWidth;
    } else {
        return 0;
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
