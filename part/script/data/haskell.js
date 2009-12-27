/** kate-script
 * name: Haskell
 * license: LGPL
 * author: Erlend Hamberg <ehamberg@gmail.com>
 * revision: 2
 * kate-version: 3.4
 * type: indentation
 */

// based on Paul Giannaro's Python indenter

var debugMode = false;

// words for which space character-triggered indentation should be done
var re_spaceIndent = /\bwhere\b|\bin\b|\belse\b/

// escapes text w.r.t. regex special chars
function escape(text) {
    return text.replace(/(\/|\.|,|\+|\?|\||\*|\(|\)|\[|\]|\{|\}|\\)/g, "\\$1");
}

String.prototype.startsWith = function(prefix) {
    return this.search(RegExp("^"+escape(prefix)+"(\\b|\\s|$)")) != -1;
}

String.prototype.endsWith = function(suffix) {
    return this.search(RegExp("(\\b|\\s)"+escape(suffix)+"\\s*$")) != -1;
}

String.prototype.lastCharacter = function() {
    var l = this.length;
    if (l == 0)
        return '';
    else
        return this.charAt(l - 1);
}

String.prototype.stripWhiteSpace = function() {
    return this.replace(/^[ \t\n\r]+/, '').replace(/[ \t\n\r]+$/, '');
}


function dbg(s) {
    // debug to the term in blue so that it's easier to make out amongst all
    // of Kate's other debug output.
    if (debugMode)
        debug("\u001B[34m" + s + "\u001B[0m");
}

var triggerCharacters = "| ";

// General notes:
// indent() returns the amount of characters (in spaces) to be indented.
// Special indent() return values:
//   -2 = no indent
//   -1 = keep last indent

function indent(line, indentWidth, character) {
    dbg(document.attribute.toString());
    dbg("indent character: '" + character + "'");
    var currentLine = document.line(line);
    dbg("current line: " + currentLine);
    var lastLine = document.line(line - 1);
    var lastCharacter = lastLine.lastCharacter();

    // invocations triggered by a space character should be ignored unless the
    // line starts with one of the words in re_spaceIndent
    if (character == ' ') {
        if (currentLine.search(re_spaceIndent) == -1) {
            dbg("skipping...");
            return document.firstVirtualColumn(line);
        }
    }

    // we can't really indent line 0
    if (line == 0)
        return -2;

    // make sure the last line is code
    if (!document.isCode(line - 1, document.lineLength(line - 1) - 1)
            && lastCharacter != "\"" && lastCharacter != "'") {
        dbg("attributes that we don't want! Returning");
        return -1;
    }

    // check the line contents ...

    // indent lines following a line ending with '='
    if (lastLine.endsWith("=")) {
        dbg('indenting for =');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent lines following a line ending with '{'
    if (lastLine.endsWith("{")) {
        dbg('indenting for {');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent lines following a line ending with 'do'
    if (lastLine.endsWith("do")) {
        dbg('indenting for do');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent line after 'where' 6 characters for alignment:
    // ... where foo = 3
    //     >>>>>>bar = 4
    if (lastLine.stripWhiteSpace().startsWith('where')) {
        dbg('indenting line for where');
        return document.firstVirtualColumn(line - 1) + 6;
    }

    // indent 'where' to column 0 + indentWidth
    // fun x = y
    // >>>>where y = x+1
    if (currentLine.stripWhiteSpace().startsWith('where')) {
        dbg('indenting line for where');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent line after 'where' unless it starts with 'module'
    // instance Functor Tree where
    // >>>>fmap = treeMap
    if (lastLine.endsWith('where') && !lastLine.startsWith('module')) {
        dbg('indenting line for where');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent line after 'let' 4 characters for alignment:
    // ... let foo = 3
    //     >>>>bar = 4
    var letCol = lastLine.search(/\blet\b/);
    if (letCol != -1) {
        dbg('indenting line for let');
        return letCol + 4;
    }

    // deindent line starting with 'in' to the level of its corresponding 'let':
    // ... let foo = 3
    //         bar = 4
    //     in foo+bar
    if (currentLine.stripWhiteSpace().startsWith('in')) {
        dbg('indenting line for in');
        var t = line-1;
        var indent = -1;
        while (t >= 0 && line-t < 100) {
            var letCol = document.line(t).search(/\blet\b/);
            if (letCol != -1) {
                indent = letCol;
                break;
            }
            t--;
        }
        return indent;
    }

    // deindent line starting with 'else' to the level of 'then':
    // ... if foo
    //        then do
    //           bar baz
    //        else return []
    if (currentLine.stripWhiteSpace().startsWith('else')) {
        dbg('indenting line for else');
        var t = line-1;
        var indent = -1;
        while (t >= 0 && line-t < 100) {
            var thenCol = document.line(t).search(/\bthen\b/); // \s*\bthen\b
            if (thenCol != -1) {
                indent = thenCol;
                break;
            }
            t--;
        }
        return indent;
    }

    // indent line after a line with just 'in' one indent width:
    // ... let foo = 3
    //         bar = 4
    //     in
    //     >>>>foo+bar
    if (lastLine.stripWhiteSpace() == 'in') {
        dbg('indenting line after in');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent line after 'case' 5 characters for alignment:
    // case xs of
    // >>>>>[] -> ...
    // >>>>>(y:ys) -> ...
    var caseCol = lastLine.search(/\bcase\b/);
    if (caseCol != -1) {
        dbg('indenting line for case');
        return caseCol + 5;
    }

    // indent line after 'if/else' 3 characters for alignment:
    // if foo == bar
    // >>>then baz
    // >>>else vaff
    var ifCol = lastLine.search(/\bif\b/);
    if (ifCol != -1) {
        dbg('indenting line for if');
        return ifCol + 3;
    }

    // indent line starting with "deriving: ":
    // data Tree a = Node a (Tree a) (Tree a)
    //             | Empty
    // >>>>>>>>>>>>>>deriving (Show)
    //
    // - OR -
    //
    // data Bool = True | False
    // >>>>>deriving (Show)
    if (currentLine.stripWhiteSpace().startsWith('deriving')) {
        dbg('indenting line for deriving');

        var pipeCol = lastLine.search(/\|/);
        if (lastLine.stripWhiteSpace().startsWith('data')) {
            return document.firstVirtualColumn(line - 1) + indentWidth;
        }
        else if (pipeCol != -1) {
            var t = 1;
            while (lastLine[document.firstVirtualColumn(line - 1)+pipeCol+t] == ' ') {
                t++;
            }
            return pipeCol + t + 1;
        }
        else {
            return document.firstVirtualColumn(line - 1) + 2;
        }
    }

    // indent lines starting with '|' (guards or alternate constructors):
    // f x
    // >>| x == 0 = 1
    // >>| x == 0 = x
    //
    // OR
    //
    // data Bool = True
    // >>>>>>>>>>| False
    if (currentLine.stripWhiteSpace().startsWith("|")) {
        dbg('indenting line for |');
        var equalsCol = lastLine.search(/=/);
        var pipeCol = lastLine.search(/\|/);
        if (equalsCol != -1 && lastLine.stripWhiteSpace().startsWith('data')) {
            return equalsCol;
        }
        else if (pipeCol != -1) {
            return pipeCol;
        }
        else {
            return document.firstVirtualColumn(line - 1) + indentWidth;
        }
    }

    // line starting with !#$%&*+./<=>?@\^|~-
    if (document.isCode(line, document.lineLength(line) - 1)
            && currentLine.search(/^\s*[!$#%&*+.\/<=>?@\\^|~-]/) != -1) {
        dbg('indenting for operator');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    //if (lastLine.search(/^\s*$/) != -1) {
    //    dbg('indenting for empty line');
    //    return 0;
    //}

    dbg('continuing with regular indent');
    return -1;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
