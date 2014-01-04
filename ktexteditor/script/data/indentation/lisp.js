/** kate-script
 * name: LISP
 * license: LGPL
 * author: Matteo Sasso <matteo.sasso@gmail.com>
 * version: 1.1
 * kate-version: 3.0
 *
 */

// required katepart js libraries
require ("range.js");

triggerCharacters = ";";

function cursors_eq(a, b)
{
    return (a.line == b.line && a.column == b.column);
}

function cursors_compare(a, b)
{
    if (!a || !b)
        return false;
    if (a.line == b.line)
        return (a.column - b.column);
    return (a.line - b.line);
}


function nearest_anchor(line, column)
{
    anchors = new Array(3);
    anchors[0] = document.anchor(line, column, '(');
    anchors[0].char = '('
    anchors[1] = document.anchor(line, column, '[');
    anchors[1].char = '['
    anchors[2] = document.anchor(line, column, '{');
    anchors[2].char = '{'

    anchors.sort(cursors_compare);

    return (anchors[2].line >= 0) ? anchors[2] : null;
}

function indent(line, indentWidth, ch)
{
    currentLineText = document.line(line);

    /* Handle single-line comments. */
    if (currentLineText.match(/^\s*;;;/)) {
        return 0;
    } else if (currentLineText.match(/^\s*;;/)) {
        // try to align with the next line
        nextLine = document.nextNonEmptyLine(line + 1);
        if (nextLine != -1)
            return document.firstVirtualColumn(nextLine);
        return -1;
    }
    if (ch != '\n') return -2;

    newlineAnchor = nearest_anchor(line, 0);
    if (!newlineAnchor)
        /* Toplevel */
        return 0;

    for (ln = line-1; ln > newlineAnchor.line; ln--) {
        prevlineAnchor = nearest_anchor(ln, 0);
        if (prevlineAnchor && cursors_eq(prevlineAnchor, newlineAnchor))
            /* IF the previous line's form is at the same nesting level of the current line...
            ...indent the current line to the same column of the previous line. */
            return document.firstVirtualColumn(ln);
    }
    nextlineAnchor = nearest_anchor(line + 1, 0);
    nextLineText = document.line(line + 1);
    if (nextlineAnchor && cursors_eq(nextlineAnchor, newlineAnchor) &&
            nextLineText && nextLineText.search(/\S/) != -1 &&
            nextlineAnchor.column < document.firstVirtualColumn(line+1))
        /* IF the next line's form is at the same nesting level of the current line,
           it's not empty and its indentation column makes sense for the current form...
           ...indent the current line to the same column of the next line. */
        return document.firstVirtualColumn(line+1);
    if (!newlineAnchor && !nearest_anchor(line-1, 0))
        /* If the user introduces a blank line the indentation level is not changed. */
        return -1;

    if (newlineAnchor.char == '(') {
        /* We are inside a form introduced previously, so we find the beginning of that
           form and determine the correct indentation column, assuming it's a function. */
        anchorText = document.line(newlineAnchor.line).substring(newlineAnchor.column+1);
        if (anchorText.match(/^[{[(]/))
           /* If the first character of the form is a bracket we assume it's something
              like Common Lisp's (let ...) special form (or just data) and we indent by 1. */
           return (newlineAnchor.column + 1);
        argument_match = anchorText.match(/^\S+\s+./);
        indentation = argument_match ? argument_match[0].length : indentWidth;
        column = document.toVirtualColumn(newlineAnchor.line, newlineAnchor.column);
        return (column + indentation);
    } else {
        /* This is probably some kind of data. Indent by 1. */
        return (newlineAnchor.column + 1);
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
