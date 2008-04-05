/** kate-script
 * name: Python
 * license: LGPL
 * author: Paul Giannaros <paul@giannaros.org>
 * version: 1
 * kate-version: 3.0
 * type: indentation
 */


// niceties

String.prototype.startsWith = function(prefix) {
    return this.substr(0, prefix.length) == prefix;
}

String.prototype.endsWith = function(suffix) {
    var startPos = this.length - suffix.length;
    if (startPos < 0)
        return false;
    return (this.lastIndexOf(suffix, startPos) == startPos);
}

String.prototype.lastCharacter = function() {
    var l = this.length;
    if(l == 0)
        return '';
    else
        return this.charAt(l - 1);
}

String.prototype.sansWhiteSpace = function() {
    return this.replace(/[ \t\n\r]/g, '');
}
String.prototype.stripWhiteSpace = function() {
    return this.replace(/^[ \t\n\r]+/, '').replace(/[ \t\n\r]+$/, '');
}


function dbg(s) {
    // debug to the term in blue so that it's easier to make out amongst all
    // of Kate's other debug output.
    debug("\u001B[34m" + s + "\u001B[0m");
}

// XX forget for now
// var triggerCharacters = "}]";

// General notes:
// indent() returns the amount of characters (in spaces) to be indented.
// Special indent() return values:
//   -2 = no indent
//   -1 = keep last indent

function indent(line, indentWidth, character) {
//     dbg(document.attribute.toString());
//     dbg("character: " + character);
//     dbg("line text: " + document.line(line));
    var currentLine = document.line(line);
    var lastLine = document.line(line - 1);
    var lastCharacter = lastLine.lastCharacter();
    // we can't really indent line 0
    if(line == 0)
        return -2;
    // check what state the last character of the previous line is in -- if 
    // it's a string/comment, we don't want to go any further
    var lastLetterType = document.attribute(line - 1, document.lineLength(line - 1) - 1);
    // 19 = docstring or comment, 20 = string
    if((lastLetterType == 19 || lastLetterType == 20) && lastCharacter != "\"" && lastCharacter != "'") {
//         dbg("attributes that we don't want! Returning");
        return -1;
    }
    // otherwise, check the line contents
    // for :, we simply indent
    dbg('line without white space: ' + currentLine.sansWhiteSpace().length);
    if(lastLine.endsWith(':')) {
        dbg('indenting line for :');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }
    // generally, when a brace is on its own at the end of a regular line
    // (i.e a data structure is being started), indent is wanted.
    // For example:
    // dictionary = {
    //     'foo': 'bar',
    // }
    // etc..
    else if(lastCharacter == '{' || lastCharacter == '[') {
        dbg('indenting for { or [');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }
    // XX this introduces irritating bugs. Commenting it out for now
    //
//     // relatively simplistic heuristic for determining whether or not to unindent:
//     // if a }] has been typed and it's the sole letter on the line, unindent
//     else if((character == '}' || character == ']') && (currentLine.sansWhiteSpace().length == 1)) {
//         dbg('unindenting line for } or ]');
//         return Math.max(0, document.firstVirtualColumn(line - 1) - indentWidth);
//     }
    // finally, a raise, pass, and continue should unindent
    lastLine = lastLine.stripWhiteSpace();
    if(lastLine == 'continue' || lastLine == 'pass' || lastLine == 'raise' || lastLine.startsWith('raise ')) {
        dbg('unindenting line for keyword');
        return Math.max(0, document.firstVirtualColumn(line - 1) - indentWidth);
    }
    return -1;
}


// kate: space-indent on; indent-width 4; replace-tabs on;
