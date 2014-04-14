/** kate-script
 * name: Pascal
 * license: LGPL
 * author: Trevor Blight <trevor-b@ovi.com>
 * revision: 1
 * kate-version: 3.4
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

/**************************  quick usage notes  **************************

Automatically handle Pascal indenting while editing code.

Whenever the return key is pressed,  the indenter correctly indents the 
just finished line, and determines where to start the new line.

Moderately sensible code is assumed:
   comments are expected to be on their own line, or at the end of the line
   keywords that start or end a statement should start a line

The begin keyword can be on the same line as its parent or it can be at the 
start the following line.
If a begin keyword starts a line, by default this indenter aligns it with its 
parent, so your code looks like this for example:
                   if x<0 then
                   begin           // begin is lined up with the if keyword
                       x := 0;
You can indent the begin by setting the document variable cfgIndentBegin.

Some effort is expended to recognise certain special cases, 
eg there should be no indent after statements like this
     if x>0 then begin f(x); writeln end; 

An if keyword that follows an else on the same line does not cause another 
indent, so multiple if and else conditions line up like this:
           if x<0 then begin
              ...
           end
           else if x>0 then begin
              ...
           end
           else begin
              ...
           end;

The indenter recognises arguments entered inside brackets over multiple lines. 
The arguments are lined up, and the closing bracket is lined up with either
the arguments or the opening bracket, whichever is appropriate.  
Even nested brackets are handled like this.

Labels are forced to the left margin and on a line of their own, apart from 
an optional comment.

These document variables are recognised (default value in parens):
  cfgIndentCase (true)     indents elements in a case statement
  cfgIndentBegin (0)       indents begin ... end pair the specified nr spaces
  cfgAutoInsertStar (true) auto insert '*' at start of new line in (* .. *)
  cfgSnapParen (true)      snap '* )' to '*)' at end of comment
  debugMode (false)        show debug output in terminal

See the USER CONFIGURATION below, and code near the start of function indent().

If, say, the nesting level of a code block changes, the affected code can be 
selected and realigned. 

You can manually adjust the line following a record, open bracket or 
unfinished condition (ie if or while statement spread over 2 or more lines).
When these lines are realigned your manual adjustment is repected.
In much the same way, comments keep the same relative indent when the code 
around them is realigned as part of the same block.

The following bugs are known.
Fixing them is possible, but would make the indenter too big, slow and clever.
 - 'else' keyword in 'case' statement not recognised.   Use 'otherwise' instead
 - comments keep alignment relative to preceding line, but naturally 
   should keep alignment relative to following line.   
 - procedure/function declarations in 'interface' should be indented
*/

"use strict";

// required katepart js libraries
require ("range.js");
require ("cursor.js");
require ("string.js");

//BEGIN USER CONFIGURATION
var cfgIndentCase = true;         // indent elements in a case statement
var cfgIndentBegin = 0;           // indent begin
var cfgAutoInsertStar = true;     // auto insert '*' in comments
var cfgSnapParen = true;          // snap ')' to '*)' in comments
var debugMode = false;            // send debug output to terminal
//END USER CONFIGURATION


const patTrailingSemi = /;\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/;
const patMatchEnd     = /\b(begin|case|record)\b/i;
const patDeclaration  = /^\s*(program|module|unit|uses|import|implementation|interface|label|const|type|var|function|procedure|operator)\b/i;
const patTrailingBegin = /\bbegin\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/i;
const patTrailingEnd = /\bend;?\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/i;
const patCaseValue = /^(\s*[^,:;]+\s*(,\s*[^,:;]+\s*)*):(?!=)/;
const patEndOfStatement = /(;|end)\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/i;

function dbg() {
    if (debugMode) {
        debug.apply(this, arguments);
    }
}


//BEGIN global variables and functions

// checking if a line starts with a case value is costly and 
// occurs multiple times per line
// use this to make it faster
var gCaseValue = -1;

var gIndentWidth = 4;

// when indenting selected code, 
// we might need to respect relative indents of successsive lines
// gShift remebers how far the previous line shifted
var gShift = 0;

//END global variables and functions


/**
 * recursively search for a corresponding 'repeat'
 * line contains keyword 'until', and its code
 * line is the start of the search.
 * return line number of corresponding 'repeat'
 *        -1 if not found 
 */
function findMatchingRepeat( line )
{
    var pLine = line;

    //dbg("\t starting from >>>>> " + document.line(pLine) + "<<<<<<<<<<");
    do {
        pLine = lastCodeLine(pLine);
        if( pLine < 0 ) return -1;
        var pLineStr = document.line( pLine );
        if( /^begin\b/i.test( pLineStr ) 
            && document.isCode(pLine, 0)) {
            dbg("\tfindMatchingRepeat: can't find 'repeat'");
            return -1;
        }
        //dbg("\tfindMatchingRepeat: backing up >>> " + pLineStr + " <<<");

        if( !/\brepeat\b.*\buntil\b/i.test(pLineStr) ) {
            var kpos = pLineStr.search( /\brepeat\b/i );
            if( kpos != -1 && document.isCode( pLine, kpos ) ) {
                //dbg("\tfindMatchingRepeat: keyword 'repeat' found on line", pLine);
                return pLine;
            }
            kpos = pLineStr.search( /\buntil\b/i );
            if( kpos != -1 && document.isCode( pLine, kpos ) ) {
                //dbg("\tfindMatchingRepeat: keyword 'until' found on line", pLine);
                pLine = findMatchingRepeat( pLine );
            }
        }
    } while( true );

} // findMatchingRepeat()


/**
 * recursively search for a corresponding 'if'
 * line contains keyword 'else', and its code
 * line is the start of the search.
 * return line number of corresponding 'if'
 *        -1 if not found 
 */
function findMatchingIf( line, hasEnd )
{
    var pLine = line;

    //dbg("\tstarting from >>>>> " + document.line(pLine) + "<<<<<<<<<<");
    do {
        // expect line to have one of
        //   - end else if
        //   - else if
        //   - if
        if( hasEnd ) {
            do {
                pLine = findMatchingKeyword(pLine, /begin/i);
                if( pLine < 0 ) return -1;
                var pLineStr = document.line( pLine );
                hasEnd = /^\s*end\b/i.test(pLineStr) 
                    && document.isCode(line, document.firstColumn(pLine) );
                //dbg("\tfinding 'begin' >>>>> " + document.line(pLine));
                //dbg("\thasEnd is", hasEnd ? "true" : "false");
            } while( hasEnd )

            // expect begin on its own, or
            // begin to trail else line
            if( !/^\s*(else|if)\b/i.test(pLineStr) ) {
                pLine = findMatchingKeyword(pLine, /\b(else|if)\b/i );
            }
        } else {
            pLine = findMatchingKeyword(pLine, /\b(else|if)\b/i );
        }
        if( pLine < 0 ) return -1;
        pLineStr = document.line( pLine );

        //dbg("\tfindMatchingIf: finding 'else' >>> " + pLineStr + " <<<");

        hasEnd = /^\s*end\b/i.test( pLineStr );
        if( ! /\belse\s+if\b/i.test(pLineStr) ) {
            var kpos = pLineStr.search( /\bif\b/i );
            if( kpos != -1 && document.isCode( pLine, kpos ) ) {
                //dbg("\tfindMatchingIf: 'if' found in " + pLineStr);
                return pLine;
            }
            kpos = pLineStr.search( /\belse\b/i );
            if( kpos != -1 && document.isCode( pLine, kpos ) ) {
                //dbg("\tfindMatchingIf: 'else' found in " + pLineStr);
                pLine = findMatchingIf( pLine, hasEnd );
            }
        }
    } while( true );

} // findMatchingIf()


/**
 * current line contains some end pattern, eg 'end'
 * recursively search for a corresponding starting pattern, 
 * eg 'begin'/'case'/'record', and return its line number.
 * matching keyword is assumed to be on some previous line, not current line
 * If not found return -1; 
 * line is the start of the search, and is not included
 */
function findMatchingKeyword(line, pattern )
{
    var testLine = lastCodeLine(line);
    if( testLine < 0 ) return -1;
    var testStr = document.line( testLine );

    //dbg("\t starting from >>>>> " + document.line(testLine) + "<<<<<<<<<<");
     while( true ) {

        //dbg("\tfindMatchingKeyword: backing up >>>> " + testStr + " <<<");

        // if /pattern/ found return
        // if end of searchable code return error
        // if end found, recursively skip block
        // need to consider:
        // if c then begin s1; s2 end; // strip these lines
        // begin   // normal case
        // end     // normal case
        // end else if begin // should be OK since /end/ tested last

        var kpos = testStr.search(/\bbegin\b.*\bend\b/i);
        if( kpos != -1 && document.isCode( testLine, kpos ) ) {
            // need to consider ...
            //                  if c1 then begin s1; s2 end
            //                  else s3; <-- align this line
            testStr = testStr.substr(0,kpos);
            //dbg("findMatchingKeyword: begin...end stripped in line " + testStr );
        } 

        kpos = testStr.search( pattern );
        if( kpos != -1 && document.isCode( testLine, kpos ) ) {
            //dbg("\tfindMatchingKeyword: keyword '" + document.wordAt( testLine, kpos == 0? document.firstColumn(testLine): kpos) + "' found on line " + testLine);
            return testLine;
        }

        if( /^(begin|var|type|function|procedure|operator)\b/i.test(testStr)  
            && document.isCode(testLine, 0) ) {
            dbg("\tfindMatchingKeyword: stopped searching at", RegExp.$1, "line", testLine );
            return -1;
        }

        kpos = testStr.search( /\bend\b/i );
        if( kpos != -1 && document.isCode( testLine, kpos ) ) {
            //dbg("\tfindMatchingKeyword: keyword 'end' found on line " + testLine);
            testLine = matchEnd( testLine );
            if( testLine == -1 ) return -1;

            var testStr = document.line( testLine );

            // line contains 'begin' (or case or record)
            // /pattern/ might be in the first part of the line, eg
            // if c then begin  <-- we might be looking for the 'if'
            kpos = testStr.search(patMatchEnd);
            if(kpos != -1) testStr = testStr.substr(0,kpos);
            //dbg("findMatchingKeyword: teststring is:" + testStr );
        } else {
            testLine = lastCodeLine(testLine);
            if( testLine < 0 ) return -1;
            testStr = document.line( testLine );
        }
    } // while
} // findMatchingKeyword()


/**
* Search for a start of block, ie corresponding 'begin'/'case'/record'
* line contains 'end'
* return line nr of start of block
* return -1 if not found
* account for variant records
*/
function matchEnd( line )
{
    var pLine = findMatchingKeyword( line, patMatchEnd );
    if( pLine < 0 ) return -1;

    var pLineStr = document.line(pLine);

    if( /\b(record\s+)?case\b/i.test(pLineStr)
        && RegExp.$1.length == 0 ) {  // findMatchingKeyword checks this is code
        // variant case doesn't count, ignore it
        // if 'case' found, go to next statement, look for ':\s*('
        // (possibly over multi lines)

        // dbg("matchEnd: found keyword 'case' " + pLineStr );
        var testLine = pLine;
        do {
            testLine = document.nextNonEmptyLine(testLine+1);
            if( testLine < 0 ) return -1;
        } while( !document.isCode(testLine, document.firstColumn(testLine) ) );
        var testStr = document.line(testLine);
        //dbg("matchEnd: found case, checking " + testStr);
        // variant records have caseValue: (...)
        var kpos = testStr.search(/:\s*\((?!\*)/g);
        if( kpos != -1 && document.isCode(testLine, kpos) ) {
            //dbg("matchEnd: variant case found in line " + testStr );
            pLine = findMatchingKeyword( pLine, /\brecord\b/i );
            // current line had better contain 'record'
            //dbg("matchEnd: parent record is", pLine );
        } else {
            // case tagType of
            //    something:
            //        ( field list ....
            do {
                testLine = document.nextNonEmptyLine(testLine+1);
                if( testLine < 0 ) return -1;
            } while( !document.isCode(testLine, document.firstColumn(testLine) ) );
            if( document.firstChar(testLine) == '(' ) {
                pLine = findMatchingKeyword( pLine, /\brecord\b/i );
                // current line had better contain 'record'
                //dbg("matchEnd: parent record is " + pLine );
            }
        }
    }
    return pLine;
} // matchEnd()


/**
 * Find last non-empty line that is code
 */
function lastCodeLine(line)
{
    do {
        line = document.prevNonEmptyLine(--line);
        if ( line == -1 ) {
            return -1;
        }
  
        // keep looking until we reach code
        // skip labels & preprocessor lines
    } while( document.firstChar(line) == '#'
             || !( document.isCode(line, document.firstColumn(line) )
                || document.isString(line, document.firstColumn(line) )
                || document.isChar(line, document.firstColumn(line) ) )
             || /^\w+?\s*:(?!=)/.test(document.line(line)) );

    return line;
} // lastCodeLine()


/**
 * 'then/do' and/or 'begin' found on the line, 
 * (this must be guaranteed by the caller)
 * scan back to find the corresponding if/while, etc, 
 * allowing it to be on a previous line
 * try to find the right line for indent for constructs like:
 * if a = b
 *     and c = d then begin <- check for 'then', and find 'if',
 * then return its line number
 * Returns -1, if no success
 */
function findStartOfStatement(line)
{
    if( line < 0 ) return -1;

    var pLine = line;
    var pLineStr = document.line(pLine);
    //dbg("\tfindStartOfStatement: starting at " + pLineStr );

    if( /^\s*begin\b/i.test( pLineStr )) {
        //dbg( "\tfindStartOfStatement: leading begin found" );
    } else {

        // looking for something like 'caseValue: begin'
        if( hasCaseValue( pLine) ) return pLine;

        // if there's a 'then' or 'do' 'begin',
        // the corresponding 'if', etc could be on a previous line.
        var kpos = -1;
        while( ((kpos = pLineStr.search(
                /\b(if|while|for|with|else)\b/i )) == -1)
                ||  !document.isCode(pLine, kpos) ) {
            pLine = document.prevNonEmptyLine(pLine-1);
            if( pLine < 0 ) return -1;
            pLineStr = document.line(pLine);
            //dbg("\tfindStartOfStatement: going back ... ", pLineStr);
            if( /^begin\b/i.test( pLineStr ) 
                 && document.isCode(pLine, 0) ) {
                dbg("\tfindStartOfStatement: can't find start of statement");
                return -1;
            }
        }
        //dbg("\tfindStartOfStatement: Found statement: " + pLineStr.trim() );
    }
    return pLine;
} // findStartOfStatement()


/* 
 * line is valid and contains a label
 * split label part & statement part (if any) into separate lines
 * align label on left margin
 * no return value 
 */
function doLabel( labelLine )
{
    if( /^(\s*\w+\s*:)\s*(.*?)?\s*((\/\/|\{|\(\*).*)?$/.test( document.line(labelLine) ) ) {
        dbg("doLabel: label is '" + RegExp.$1 + "', statement is '" + RegExp.$2 + "', comment is '" + RegExp.$3 + "'" );

        if( RegExp.$2.length != 0 ) {
            document.wrapLine( labelLine, RegExp.$1.length );
        }
    }
    //else dbg("doLabel: couldn't decode line", document.line(labelLine));

} // doLabel()


function hasCaseValue( line )
{
    if(line < 0) return false;
    if( gCaseValue == line ) {
        //dbg("hasCaseValue: line already decoded");
        return true;
    }

    var lineStr = document.line(line);

    if( /^\s*otherwise\b/i.test(lineStr)
        && document.isCode(line, document.firstColumn(line)) ) {
        //dbg( "hasCaseValue: 'otherwise' found ", lineStr );
        return true;
    }
    return (decodeColon(line) > 0);
} // hasCaseValue()


/*
* check if the line starts with a case value
* return cursor position of 'case' statement if it exists
*        0 when a label is found
*       -1 otherwise
* called when there's a ':' on the current line, and it's code
*
* need to test that the : is a case value, not
*  - inside writeln parens, or 
*  - a var declaration, or 
*  - a function procedure heading
*/
function decodeColon( line )
{
    if( !document.isCode(line, 0 ) )
        return -1;

    var kpos;
    var pLine = line;
    var pLineStr = document.line(pLine);

    if( !patCaseValue.test(pLineStr)
        || !document.isCode(pLine, RegExp.$1.length) ) {
        //dbg( "decodeColon: uninteresting line ", pLineStr );
        return -1;
    }

    //dbg("decodeColon: value is " + RegExp.$1 );
    pLineStr = RegExp.$1;
    
    /* check ':' in statements.   are they case values?
     * special cases are ':' inside '(...)'
     * eg
     *  writeln( a:3 );
     *  function( x: real );
     * parens over multiple lines already filtered out by tryParens
     */

    var Paren = document.anchor(pLine, pLineStr.length, '(' );
    if( Paren.isValid() ) {
        //dbg("decodeColon: ':' found inside '(...)', not case value");
        return -1;
    }
    
    do {
        //dbg("decodeColon: scanning back >" + pLineStr);
        // we know that pLine is code
        if( /(^\s*var|\brecord)\b/i.test(pLineStr) ) {
            //dbg("decodeColon: ':' is part of", RegExp.$1, "declaration");
            return -1;
        } 
        if( /^\s*type\b/i.test(pLineStr)  ) {
            dbg("decodeColon: ':' can't find parent, giving up after finding keyword 'type' at line", pLine);
            return -1;
        } 

        // jump over 'end' to ensure we don't find an inner 'case' statement
        if( /^\s*end\b/i.test(pLineStr) ) {
            //dbg("decodeColon: skipping inner block");
            pLine = matchEnd( pLine );
            if(pLine < 0) return -1;
            //dbg("decodeColon: back to previous block");
            // line contains 'begin' (or case)
            // /case/ might be in the first part of the line, eg
            //  c then begin  <-- we might be looking for the 'if'
            pLineStr = document.line( pLine );
            kpos = pLineStr.search(patMatchEnd);
            if(kpos != -1)
                pLineStr = pLineStr.substr(0,kpos);
            //dbg("decodeColon: retesting line:" + pLineStr );
        } else if( (kpos = pLineStr.search( /\)\s*;/ )) != -1 
                   && document.isCode( pLine, kpos )  ) {
            //dbg("\tdecodeColon: close paren found on line " + pLineStr);
            var openParen = document.anchor( pLine, kpos, ')' );
            if( !openParen.isValid() ) {
                dbg("decodeColon: mismatched parens");
                return -1;
            }
            // go to open paren, get substr
            pLine = openParen.line;
            pLineStr = document.line( pLine ).substr(0,openParen.column);
            //dbg("\tdecodeColon: skipping over parens to line " + pLine + pLineStr);
        } else {
            if( /^\s*until\b/i.test(pLineStr) ) {
                pLine = findMatchingRepeat( pLine );
            }
            pLine = lastCodeLine(pLine);
            if( pLine < 0 ) {
                dbg("decodeColon: no more lines");
                return -1;
            }
            pLineStr = document.line(pLine);
        }

        // no more code to search
        // we are looking at a label if we reach start of a block
        if( /^\s*(begin\b(?!.*\bend)|repeat\b(?!.*\buntil)|otherwise)\b/i.test(pLineStr) 
            || patTrailingBegin.test(pLineStr) ) {
            //dbg("decodeColon: found a label");
            return 0;
        }

        kpos = pLineStr.search(/\bcase\b/i); 
        if( kpos != -1 && document.isCode(pLine,kpos) ) {
            //dbg("decodeColon: found a case value");
            gCaseValue = line;

            var indent = document.toVirtualColumn(pLine, kpos);
            if (indent != -1) {
                if (cfgIndentCase)  indent += gIndentWidth;
            }
            return indent;
        } 
    } while( pLine > 0 );

    dbg("decodeColon: unexpected return");
    return -1;
} // decodeColon()


/**
 * Comment checking.
 * If the previous line begins with a "(*" or a "* ", then
 * return its leading white spaces + ' *' + the white spaces after the *
 * return: filler string or null, if not in a comment
 */
function tryComment(line, newLine)
{
    var p = document.firstColumn(line);
    if( !document.isComment(line, p<0? 0: p) ) {
        //dbg( "tryComment: line " + line + " is not a comment" );
        return -1;
    }

    var startComment = Cursor.invalid();
    var adjust = 0;
    if( document.startsWith(line, "*)", true ) ) {
        //dbg("find the opening (* and line up with it");
        startComment = document.rfind(line, 0, "(*");
        adjust = 1;
    }
    else if( document.startsWith(line,"}", true ) ) {
        //dbg("find the opening { and line up with it");
        startComment = document.rfind(line, 0, "{");
    }

    if( startComment.isValid() ) {
        indent = document.toVirtualColumn(startComment);
        if (indent != -1) { 
            dbg("tryComment: aligned to start of comment at line", startComment.line);
            return indent+adjust;
        }
    }

    var pline = document.prevNonEmptyLine(line - 1);
    if( pline < 0 )  {
        dbg("tryComment: keep existing indent");
        return -2;
    }

    var indent = -1;

    if( pline == line - 1 ) {   // no empty line in between

        var plineStr = document.line(pline);
        var startPos = plineStr.indexOf("(*");
        if( startPos != -1 
            && !plineStr.contains("*)")
            && document.isComment(pline, startPos) ) {

            indent = document.toVirtualColumn(pline,startPos); // line up with '(*'
            indent += 1; // line up with '*'
            dbg("tryComment: " + line + " follows start of comment");
        } else if( document.firstChar(pline) == '*' 
                   && ! plineStr.contains("*)") ) {
            indent = document.firstVirtualColumn(pline);
        }
    }

    // now indent != -1 if there is a '*' to follow

    if( indent != -1 && newLine && cfgAutoInsertStar ) {
        dbg("tryComment: leading '*' inserted in comment line");
        document.insertText(line, view.cursorPosition().column, '*');
        if (!document.isSpace(line, document.firstColumn(line) + 1))
            document.insertText(line, document.firstColumn(line) + 1, ' ');
    } else if( indent == -1 || document.firstChar(line) != '*' ) {
        indent = document.firstVirtualColumn(line);
        if( indent >= 0 ) {
            dbg("tryComment: keeping relative indent of existing comment");
            if( indent + gShift >= 0) indent += gShift;
        } else {
            dbg("tryComment: new comment line follows previous one");
            indent = document.firstVirtualColumn(pline);
        }
    } else {
        dbg("tryComment: aligning leading '*' in comments");
    }

    return indent;

} // tryComment()


/**
 * Check for a keyword that determines the indent of the current line
 * some keywords directly determine the indent of the current line
 *  (eg 'program' => zero indent)
 * some keywords need to align with another keyword further up the code
 *  (eg until needs to line up with its matching repeat)
 * for some lines,  the indent depends on a keyword on the previous line
 *  (eg a line that follws if, else, while, const, var, case etc...) 
 */
function tryKeywords(line)
{
    var kpos;

    if(document.firstChar(line) == '#') {
        dbg("tryKeywords: preprocessor line found: zero indent");
        return 0;
    }

    var lineStr = document.line(line);
    var isBegin = false;

    if( document.isCode( line, 0 ) ) {

        // these declarations always go on the left margin
        if( patDeclaration.test(lineStr) ) {
            dbg("tryKeywords: zero indent for '" + RegExp.$1 + "'" );
            return 0;
        }

        // when begin starts the line, determine indent later
        isBegin = /^\s*begin\b/i.test(lineStr);

        var indent = -1;
        var refLine = -1;

        /**
         * Check if the current line is end of a code block
         * (ie it contains 'else', 'end' or 'until')
         * return indent of corresponding start of block
         * If not found return -1; 
         * start of the search at line.
         */
        var found = true;
        if( /^\s*end\b/i.test(lineStr) ) {
            //dbg("tryKeywords: 'end' found, hunting for start of block");
            refLine = matchEnd( line );
            var refLineStr = document.line(refLine);
            // dbg("refLine is ", refLineStr );
            // if end matched 'begin', return indent of corresponding statement
            kpos = refLineStr.search(/\bbegin\b/i);
            if( kpos != -1 && document.isCode(refLine, kpos) ) {
                refLine = findStartOfStatement(refLine);
                //dbg("tryKeywords: keyword 'begin' found, start of statement is", refLineStr);
            } else if( /\brecord\b/i.test(refLineStr) ) {
                //align to record, not start of line
                refLine = document.nextNonEmptyLine(refLine+1);
                dbg("tryKeywords: outdenting 'end' relative to", document.line(refLine));
                return document.firstVirtualColumn(refLine) - gIndentWidth;
            }
        } else if( /^\s*until\b/i.test(lineStr) ) {
            //dbg( "tryKeywords: found keyword 'until' in " + lineStr );
            refLine = findMatchingRepeat( line );
        } else if( /^\s*(end)?\s*else\b/i.test(lineStr) ) {
            //dbg("tryKeywords: found 'else', hunting back for matching 'if'");
            refLine = findMatchingIf( line, RegExp.$1 );
        } else {
            found = false;
        }

        if( found ) {
            if( refLine < 0 ) return -1;
            // we must line up with statement at refLine
            dbg("tryKeywords: align to start of block at", document.line(refLine));
            indent = document.firstVirtualColumn(refLine);
            if( hasCaseValue( refLine ) ) indent += gIndentWidth;
            return indent;
        }

        if( /^\s*otherwise/i.test(lineStr) ) {
            refLine = findMatchingKeyword( line, /\bcase\b/i );
            if( refLine != -1 ) {
                indent = document.firstVirtualColumn(refLine);
                if (indent != -1) {
                    if (cfgIndentCase)  indent += gIndentWidth;
                    dbg("tryKeywords: 'otherwise' aligned to 'case' in line " + refLine);
                }
                return indent;
            }
        } else if( /^\s*(then|do)\b/i.test(lineStr) ) {
            dbg("tryKeywords: '" + RegExp.$1 + "' aligned with start of statement" );
            refLine = findStartOfStatement( line );
            if( refLine >= 0 )
                return document.firstVirtualColumn(refLine);
        }
        else if( /^\s*of/i.test(lineStr) ) {
            // check leading 'of' in array declaration
            //dbg("tryKeywords: found leading 'of'");
            refline = lastCodeLine(line);
            if(refline >= 0) {
                reflineStr = document.line(refline);
                kpos = reflineStr.search(/\]\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/);
                if( kpos != -1 ) {
                    //dbg("tryKeywords: found trailing ']'");
                    var c = document.anchor(refline, kpos, ']');
                    if( c.isValid() ) {
                        reflineStr = document.line(c.line).substring(0,c.column);
                        //dbg("tryKeywords: start of array is", reflineStr.trim());
                        kpos = reflineStr.search(/\barray\s*$/i);
                        if( kpos != -1 ) {
                            dbg("tryKeywords: indenting leading 'of' in array declaration");
                            return document.toVirtualColumn(c.line, kpos) + gIndentWidth;
                        }
                    }
                }
            }
        }
        else {
            //dbg("tryKeywords: checking for a case value: " + lineStr);
            indent = decodeColon( line );
            if( indent != -1 ) {
                if(indent == 0) 
                    doLabel(line);
                else 
                    dbg("tryKeywords: case value at line", line, "aligned to earlier 'case'");
                return indent;  // maybe it's a label
            }
        }
    } // is code

    //** now see if previous line determines how to indent line

    var pline = lastCodeLine(line);
    if (pline < 0)  return -1;
    var plineStr = document.line(pline);
    //dbg("tryKeywords: test prev line", plineStr);

    //dbg("tryKeywords: checking main begin ...");
    if( isBegin && patTrailingSemi.test( plineStr ) ) {
        // the previous line has trailing ';' this is the main begin
        // it goes on the left margin
        dbg("tryKeywords: main begin at line " + line);
        return 0;
    }

    // check array declarations
    //dbg("tryKeywords: checking arrays ....");
    kpos = plineStr.search(/\]\s*of\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/i) ;
    if( kpos != -1 && document.isCode(pline, kpos) ) {
        var c = document.anchor(pline, kpos, ']');
        if( c.isValid() ) {
            reflineStr = document.line(c.line);
            kpos = reflineStr.search(/\barray\s*\[/i);
            if( kpos != -1 ) {
                dbg("tryKeywords: indenting from unfinished array declaration");
                return document.toVirtualColumn(c.line, kpos) + gIndentWidth;
            }
        }
    }

    //dbg("tryKeywords: checking record ...");
    kpos = plineStr.search(/\brecord\b/i);
    if( kpos != -1 && document.isCode(pline, kpos) ) {
        // need to skip single line records
        if( !patTrailingEnd.test(plineStr) ) {
            indent = document.firstVirtualColumn(line);
            if( indent + gShift > document.firstVirtualColumn(pline) ) {
                dbg("tryKeywords: keeping indent following 'record'");
                return indent + gShift;
            } else {
                dbg("tryKeywords: indenting after 'record'");
                return document.toVirtualColumn(pline,kpos) + gIndentWidth;
            }
        }
    }

    //dbg("tryKeywords: checking unfinished type decs ...");
    if( /^(type)?\s*\b\w+\s*=\s*(\/\/.*|\{.*|\(\*.*)?$/.test(plineStr) ) {
        indent = document.toVirtualColumn(pline,
                       document.nextNonSpaceColumn(pline, RegExp.$1.length) );
        dbg("tryKeywords: indenting after 'type'" );
        return indent + gIndentWidth;        
    }

    if( /^(uses|import|label|type|const|var|case)\b/i.test(plineStr) ) {
        // indent after these
        dbg("tryKeywords: indent after", RegExp.$1);
        return gIndentWidth;
    }

    // ignore repeat s1 until c;
    if( /^\s*repeat\b/i.test(plineStr) 
        && document.isCode(pline, document.firstColumn(line)) ) {
        kpos = plineStr.search(/\buntil\b/i);
        if( kpos == -1 || !document.isCode(pline, kpos) ) {
            dbg( "tryKeywords: indenting after 'repeat'" );
            return document.firstVirtualColumn(pline) + gIndentWidth;
        }
    }
        
    if( /^\s*otherwise\b/i.test(plineStr) 
        && document.isCode(pline, document.firstColumn(line)) ) {
        dbg( "tryKeywords: indenting after 'otherwise'" );
        return document.firstVirtualColumn(pline) + gIndentWidth;
    }
        
    // after case keyword indent sb ready for a case value
    kpos = plineStr.search(/^\s*case\b/i);
    if( kpos != -1 && document.isCode(pline, kpos) ) {
        indent = document.firstVirtualColumn(pline);
        dbg("tryKeywords: new case value");
        return (cfgIndentCase)?  indent + gIndentWidth: indent;
    }

    //dbg("tryKeywords: checking multiline condition ...");
    if( /^\s*(while|for|with|(else\s*)?if)\b/i.test(plineStr)
        && document.isCode(pline, document.firstColumn(pline)) ) {
        // found if, etc.
        // if there's no then/do it's a multiline condition
        // if the line is empty indent by one,
        // otherwise keep existing indent
        var keyword = RegExp.$1; // only used for debug
        kpos = plineStr.search(/\b(then|do)\b/i);
        if( kpos == -1 || ! document.isCode(pline, kpos) ) {
            indent = document.firstVirtualColumn(line);
            if( indent + gShift > document.firstVirtualColumn(pline) ) {
                dbg("tryKeywords: keeping relative indent following '" +keyword+ "'");
                return indent + gShift;
            } else {
                dbg("tryKeywords: indenting after '" + keyword + "'");
                return document.firstVirtualColumn(pline)+2*gIndentWidth;
            }
        }
    } else {
        // there's no 'if', 'while', etc => test for 'then' or 'do'
        // then go back to the corresponding 'if' while'/'for'/'with'
        // assume case/with do not spread over multiple lines
        //dbg("tryKeywords: checking then/do ...");
        kpos = plineStr.search( /\b(then|do)(\s*begin)?\s*(\/\/.*|\{.*|\(\*.*)?$/i );
        if ( kpos != -1 && document.isCode( pline, kpos)) {
            //dbg("tryKeywords: found '" + RegExp.$1 + "', look for 'if/while/for/with'");
            pline = findStartOfStatement( pline );
            plineStr = document.line(pline);
        }
    }
    
    var xind = 0;   // extra indent
    //dbg("tryKeywords: checking case value ...");
    if( hasCaseValue( pline ) ) {
        plineStr = plineStr.substr( plineStr.indexOf(":")+1 );
        //dbg("tryKeywords: plineStr is '" + plineStr +"'" );
        kpos = plineStr.search(patEndOfStatement);
        if( kpos == -1 || !document.isCode(pline, kpos) ) {
            dbg("tryKeywords: extra indent after case value");
            xind = gIndentWidth;
        }
    }

    // link the line to 'begin' on the line above
    if( /^\s*begin\b/i.test(plineStr)  && document.isCode(pline, 0) ) {
        if( document.firstColumn(pline) == 0 ) {
            dbg( "tryKeywords: indent after main 'begin'" );
            return document.firstVirtualColumn(pline) + gIndentWidth;
        }
        kpos = plineStr.search(/\bend\b/i);
        if( kpos == -1 || !document.isCode(pline, kpos) ) {
            dbg( "tryKeywords: indent after leading 'begin'" );
            return document.firstVirtualColumn(pline) + xind + gIndentWidth - cfgIndentBegin;
        }
    }

    //dbg("tryKeywords: checking control statements ...");
    if( /^\s*(if|for|while|with|(end\s*)?\else)\b/i.test(plineStr) 
        || ((xind > 0) && (/^\s*begin\b/i.test(plineStr)) ) ) { // xind>0 ==> case value 
        var keyword = RegExp.$1; // only used for debug
        // indent after any of these, 
        // unless it's a single line statement
        if( !patEndOfStatement.test(plineStr) ) {
            kpos = lineStr.search(/end\s*;?\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/i) ;
            if( isBegin && (kpos == -1 || !document.isCode(line, kpos)) ) {
                //dbg("begin found - not cancelled by trailing end");
                dbg("tryKeywords: indent", cfgIndentBegin, "extra spaces for begin after " + keyword );
                xind += cfgIndentBegin;
            } else {
                dbg("tryKeywords: indent after", keyword);
                xind += gIndentWidth;
            }
            return  xind + document.firstVirtualColumn(pline);
       } 
    }

    //casevalue or  if/while/etc
    if( xind > 0 ) {
        return  xind + document.firstVirtualColumn(pline);
    }
    
    //dbg("tryKeywords: check unfinished := ...");
    kpos = plineStr.search(/:=[^;]*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/i);
    if( kpos != -1 && document.isCode(pline, kpos) ) {
        indent = document.firstVirtualColumn(line);
        if( indent + gShift > document.firstVirtualColumn(pline) ) {
            dbg("tryKeywords: keeping indent following ':='");
            return indent + gShift;
        } else {
            var pos = document.nextNonSpaceColumn(pline, kpos+2);
            dbg( "tryKeywords: indent after unfinished assignment" );
            if( pos != -1 )
                return document.toVirtualColumn(pline, pos);
            else
                return document.toVirtualColumn(pline, kpos) + gIndentWidth;
        }
    }

    return -1;
} // tryKeywords()


/* test if line has an unclosed opening paren '('
 * line is document line
 * n is nr chars to test, starting at pos 0
 * return index of unmatched '(', or
 *        -1 if no unmatched '(' 
 */
function checkOpenParen( line, n )
{
    var nest = 0;
    var i = n;
    while( nest <= 0 && --i >= 0 ) {
        if( document.isCode(line, i) ) {
            var c = document.charAt( line, i);
            if( c == ')' )      nest--;
            else if( c == '(' ) nest++;
            else if( c == ']' ) nest--;
            else if( c == '[' ) nest++;
        }
    }
    //dbg("string " + document.line(line) + (i==-1? " has matched open parens": " has unclosed paren at pos " + i ));
    return i;
}

/* test if line has an unmatched close paren
 * line is document line
 * n is nr chars to test, starting at pos 0
 * return index of rightmost unclosed paren
 *        -1 if no unmatched paren 
 */
function checkCloseParen( line, n )
{
    var nest = 0;
    var i = -1;
    var bpos = -1;
    while( ++i < n ) {
        if( document.isCode(line, i) ) {
            var c = document.charAt( line, i);
            if( c == ')' )      nest++;
            else if( c == '(' ) nest--;
            else if( c == ']' ) nest++;
            else if( c == '[' ) nest--;
        }
        if( nest > 0 ) {
            nest = 0;
            bpos = i;
        }
    }
    //dbg("string " + document.line(line) + (bpos==-1? " has matched close parens": " has unbalanced paren at pos " + bpos) );
    return bpos;
}


/**
 * line up args inside parens
 * if the opening line has no args, line up with first arg on next line
 * ignore trailing comments.
 * opening line ==> figure out indent based on pos of first arg
 * subsequent args ==> maintain indent
 *       ');' ==> align closing paren
 */
    //  check the line above
    //  if it has unclosed paren ==> indent to first arg
    //  other lines, keep indent

function tryParens(line, newLine)
{
    var pline = lastCodeLine(line);    // note: pline is code
    if (pline < 0) return -1;
    var plineStr = document.line(pline);
 
    /**
     * first, handle the case where the new line starts with ')' or ']'
     * find out whether we pressed return in something like () or [] and 
     * indent properly, so
     * []
     * becomes:
     * [
     *   |
     * ]
     *
     */
    var char = document.firstChar(line);
    if ( (char == ')' || char == ']')
            &&  document.isCode( line, document.firstColumn(line) ) ) {
        //dbg("tryParens - leading close " + char + " on current line");

        var openParen = document.anchor(line, 0, char);
        if( !openParen.isValid() ) {
            // nothing found, continue with other cases
            dbg("tryParens: open paren not found");
            return -1;
        }
        var indent = document.toVirtualColumn(openParen);

        //dbg("tryParens: found left anchor '" + document.charAt(openParen) + "' at " + openParen.toString() );

        // look for something like
        //        argn     <--- pline
        //        );       <--- line, trailing close paren
        // the trailing close paren lines up with the args or the open paren,
        // whichever is the leftmost
        if( pline > openParen.line ) {
            var argIndent = document.firstVirtualColumn(pline);
            if( argIndent > 0 && argIndent < indent) { 
                indent = argIndent;
                dbg("tryParens: align trailing '" + char + "' with arg list");
            }
            else {
                dbg("tryParens: align trailing '" + char + "' with open paren");
            }
        }

        // look for previous line ending with unclosed paren like "func(" {comment}
        // and no args following open paren
        var bpos = plineStr.search(/[,(\[]\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/ );
        // open line for a new arg if following a trailing ','
        // or if this is the first arg, ie parens match
        var openLine = false;
        if( bpos != -1 && document.isCode(pline, bpos)) {
            var lastChar = plineStr[bpos];
            openLine =   ( lastChar == ',' ) || 
                ( lastChar == '(' && char == ')' ) ||
                ( lastChar == '[' && char == ']' );
            //dbg("lastChar is '" + lastChar + "', openLine is " + (openLine? "true": "false") );
        }

        // just align brackets, not args
        if( !newLine || !openLine ) {
            return indent;
        }

        dbg("tryParens: opening line for new argument");
        //dbg(lastChar == ','? "following trailing ',' ": "parens match");
        /* on entry, we have one of 2 scenarios:
         *    arg,  <--- pline, , ==> new arg to be inserted
         *    )     <--- line
         * or
         *    xxx(  <--- pline
         *    )     <--- line
         *
         * in both cases, we need to open a line for the new arg and 
         * place right anchor in same column as the opening anchor
         * we leave cursor on the blank line
         */
        document.insertText(line, document.firstColumn(line), "\n");
        var anchorLine = line+1;
        // indent closing anchor 
        view.setCursorPosition(line, indent);
        document.indent(new Range(anchorLine, 0, anchorLine, 1), indent / gIndentWidth);
        // make sure we add spaces to align perfectly on left anchor
        var padding =  indent % gIndentWidth;
        if ( padding > 0 ) {
            document.insertText(anchorLine, 
                                document.fromVirtualColumn(anchorLine, indent - padding),
                                String().fill(' ', padding));
        }
    } // leading close paren


    // now look for line ending with unclosed paren like "func(" {comment}
    // and no args following open paren
    var bpos = plineStr.search(/[(\[]\s*(\/\/.*|\{.*\}|\(\*.*\*\))?\s*$/ );
    if( bpos != -1 && document.isCode(pline, bpos) ) {
        // we have something like
        //        func(     <--- pline
        //                  <--- line, first arg goes here
        // line contains the first arg in a multi line list
        // other args should be lined up against this
        // if line is not empty, assume user has set it - leave it as it is
        var bChar = plineStr[bpos];
        //dbg("tryParens: - trailing open " + char + " on previous line");
        indent = document.firstVirtualColumn(line);
            if( indent + gShift > document.firstVirtualColumn(pline) ) {
            dbg("tryParens: keep relative indent following '" + bChar + "'");
            indent += gShift;
        } else {
            dbg( "tryParens: args indented from opening paren" );
            indent = document.toVirtualColumn(pline, bpos) + gIndentWidth;
        }
        return indent;
    } // trailing open paren

    // next, look for something like
    //        func( arg1,     <--- pline
    //                        <--- line
    // note: if open paren is last char we would not be here
    bpos = checkOpenParen(pline, document.lineLength(pline));
    if( bpos != -1 ) {
        //dbg( "tryParens: found open paren '" + plineStr[bpos] + "' and trailing arg" );
        bpos = document.nextNonSpaceColumn(pline, bpos+1);
        dbg("tryParens: aligning arg to", document.wordAt(pline, bpos), "in", plineStr);
        return document.toVirtualColumn(pline, bpos );
    } // trailing opening arg

    // next step, check for close paren,  but nested inside outer paren
    bpos = checkCloseParen(pline, plineStr.length);
    if( bpos != -1 ) {
        // we have something like this:
        //     )  <-- pline
        //        <-- line

        // assert( indent == -1 );
        //dbg( "arg list terminated with '" + plineStr[bpos] + "'", " at pos", bpos );
        var openParen = document.anchor( pline, bpos, plineStr[bpos] );
        if (openParen.isValid() ) {
            // it's more complicated than it appears:
            // we might still be inside another arglist!
            // check back from openParen, 
            // if another open paren is found then
            //      indent to next non white space after that
            var testLine = openParen.line;
            //dbg("tryParens: testing " + document.line(testLine).substr(0,openParen.column) );
            bpos = checkOpenParen(testLine, openParen.column);
            if( bpos != -1 ) {
                // line above closes parens, still inside another paren level
                //dbg("tryParens: found yet another open paren @ pos", bpos);
                bpos = document.nextNonSpaceColumn(testLine, bpos+1);
                //dbg("tryParens: aligning with open paren in line", openParen.line);
                indent = document.toVirtualColumn(testLine, bpos );
                return indent;
            } else if( document.anchor(line,0,'(').isValid() 
                       || document.anchor(line,0,'[').isValid() ) {
                dbg("tryParens: aligning to args at next outer paren level");
                return document.firstVirtualColumn(testLine );
            }
        } else {
            // line above closes list, can't find open paren!
        }
    } else {
        // line above doesn't close parens, we may or may not be inside parens
    }

    // finally, check if inside parens, 
    // if so, align with previous line
    // eliminates a few checks later on
    openParen = document.anchor(line,0,'(');
    if( openParen.isValid() && document.isCode(openParen) ) {
        dbg("tryParens: inside '(...)', aligning with previous line" );
        return document.firstVirtualColumn(pline );
    }

    openParen = document.anchor(line,0,'[');
    if( openParen.isValid() && document.isCode(openParen) ) {
        dbg("tryParens: inside '[...]', aligning with previous line" );
        return document.firstVirtualColumn(pline );
    }

    return -1;

} // tryParens()


/**
 * hunt back to find statement at the same level, or parent
 * align to same as sibling, or add one indent from parent
 * line is start of the search
 * return indent of line
 *        never return -1, follow previous line if indent not determined
 * note:
 * the first child of a keyword has alreday been indented by tryKeywords()
 */
function tryStatement(line)
{
    var plineStr;
    var indent = -1;
    var pline = lastCodeLine(line);
    if(pline<0) {
        //dbg("tryStatement: no code to align to");
        return 0;
    }

    var defaultIndent = document.firstVirtualColumn(pline);
    plineStr = document.line(pline);

    var kpos = plineStr.search(patTrailingSemi);

    if( kpos == -1 || !document.isCode(pline, kpos) ) {
        dbg("tryStatement: following previous line");
        return defaultIndent;
    }

    var prevInd = defaultIndent;
    //dbg("tryStatement: plineStr is " + plineStr );

    do {
        if( /^\s*end\b/i.test(plineStr) ) {
            //dbg("tryStatement: end found, skipping block");
            pline = matchEnd( pline );
            if( pline < 0 ) return defaultIndent;
            plineStr = document.line(pline);
            //dbg("tryStatement: start of block is " + plineStr);

            kpos = plineStr.search(patMatchEnd);
            if(kpos != -1)
                plineStr = plineStr.substr(0,kpos);
            //dbg("tryStatement: start of block is " + plineStr );
            // prevInd for 'begin' must be same as for 'end'
        } else if( /^\s*until\b/i.test(plineStr) ) {
            //dbg("tryStatement: aligning after 'until'" );
            pline = findMatchingRepeat(pline);
            if( pline < 0 ) return defaultIndent;
            plineStr = document.line(pline);

            kpos = plineStr.search(/\brepeat\b/i);
            if( kpos != -1 )
                plineStr = plineStr.substr(0,kpos);
            //dbg("tryStatement(r): plineStr is " + plineStr );
            // prevInd for 'repeat' must be same as for 'until'
        } else {
            // keep indent of old line in case we need to line up with it
            prevInd = document.firstVirtualColumn(pline);
            pline = lastCodeLine( pline );
            if(pline<0) return defaultIndent;
            plineStr = document.line(pline);
            kpos = plineStr.search(patTrailingSemi);
            if( kpos != -1 ) {
                var c = document.anchor(pline, kpos, ')');
                if( !c.isValid() ) {
                    dbg("tryStatement: aligning with statement after ';'");
                    return prevInd;
                }
            }
        }

        //dbg("tryStatement: plineStr is " + plineStr );

        // need record to have priority over type & var, so test it first
        kpos = plineStr.search(/\brecord\b/i);
        if( kpos != -1 
            && document.isCode(pline, kpos) ) {
            if( patTrailingEnd.test(plineStr) ) {
                //dbg( "tryStatement: skipping single line record " );
            } else {
                dbg( "tryStatement: indenting after record " );
                return prevInd;  // line up with first member
            }
        }

        // patDeclaration has leading whitespace
        if( /^(uses|import|label|const|type|var)\b/i.test(plineStr) 
            && document.isCode(pline, 0) ) {
            // align after the keyword
            dbg("tryStatement: indenting after " + RegExp.$1 );
            return gIndentWidth;
        } 

        // ignore repeat s1 until c;
         if( /^\s*(repeat\b(?!.*\buntil\b)|otherwise)\b/i.test(plineStr) ) {
            dbg("tryStatement: indent after '"+ RegExp.$1 + "' in line", pline);
            return prevInd;
        }

        // statements like 'if c then begin s1 end' should just pass thru
        kpos = plineStr.search(patTrailingBegin);
        if( kpos != -1 && document.isCode(pline, kpos) ) {
            dbg( "tryStatement: indenting from begin" );
            return prevInd;
        }

        // after case keyword indent sb ready for a case value
        kpos = plineStr.search(/^\s*case\b/i);
        if( kpos != -1 && document.isCode(pline, kpos) ) {
            dbg("tryStatement: new case value");
            return prevInd;
        }

    } while( true );

} // tryStatement()


// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = " \t)]}#;";

// possible outdent for lines that match this regexp
var PascalReIndent = /^\s*((end|const|type|var|begin|until|function|procedure|operator|else|otherwise|\w+\s*:)\s+|[#\)\]\}]|end;)(.*)$/;

// check if the trigger characters are in the right context,
// otherwise running the indenter might be annoying to the user
function reindentTrigger(line)
{
    var res = PascalReIndent.exec(document.line(line));
    dbg("reindentTrigger: checking line, found", ((res && res[3] == "" )? "'"+ (res[2]?res[2]:res[1])+"'": "nothing") );
    return ( res && (res[3] == "" ));
} // reindentTrigger


function processChar(line, c)
{
   var cursor = view.cursorPosition();
    if (!cursor)
        return -2;

    var column = cursor.column;
    var lastPos = document.lastColumn(line);

    //dbg("processChar: lastPos:", lastPos, ", column..:", column);

    if (cfgSnapParen 
               && c == ')'
               && lastPos == column - 1) {
        // try to snap the string "* )" to "*)"
        var plineStr = document.line(line);
        if( /^(\s*\*)\s+\)\s*$/.test(plineStr) ) {
            plineStr = RegExp.$1 + c;
            document.editBegin();
            document.removeLine(line);
            document.insertLine(line, plineStr);
            view.setCursorPosition(line, plineStr.length);
            document.editEnd();
        }
    }
    return -2;
} // processChar()


// indent() returns the amount of characters (in spaces) to be indented.
// arguments: line, indentwidth in spaces, typed character indent
// Special indent() return values:
//   -2 = no indent
//   -1 = keep previous indent

function indent(line, indentWidth, ch)
{
    // all functions assume valid line nr on entry 
    if( line < 0 ) return -1;

    var t = document.variable("debugMode");
    if(t) debugMode = /^(true|on|enabled|1)$/i.test( t );

    dbg("\n------------------------------------ (" + line + ")");

    t = document.variable("cfgIndentCase");
    if(t) cfgIndentCase = /^(true|on|enabled|1)$/i.test( t );
    t = document.variable("cfgIndentBegin");
    if(/^[0-9]+$/.test(t)) cfgIndentBegin = parseInt(t);

    t = document.variable("cfgAutoInsertStar");
    if(t) cfgAutoInsertStar = /^(true|on|enabled|1)$/i.test( t );
    t = document.variable("cfgSnapParen");
    if(t) cfgSnapParen = /^(true|on|enabled|1)$/i.test( t );

    gIndentWidth = indentWidth;
    gCaseValue = -99;

    var newLine = (ch == "\n");

    if( ch == "" ) {
        // align Only
        if( document.firstVirtualColumn(line) == -1 ) {
            dbg("empty line,  zero indent");
            return 0;
        }
    } else if (ch == '\n') {
        // cr entered - align the just completed line
        document.align(new Range(line-1, 0, line-1, 1));
    } else if( !reindentTrigger(line) ) {
        return processChar(line, ch);
    }

    var oldIndent = document.firstVirtualColumn(line);
    var sel = view.selection();
    if( !sel.isValid() || line == 0 || !sel.containsLine(line-1) ) {
        gShift = 0;
    }

    var filler = tryComment(line, newLine);

    if (filler == -1)
        filler = tryParens(line, newLine);

    if (filler == -1)
        filler = tryKeywords(line);

    if (filler == -1)
        filler = tryStatement(line);

    if( sel.isValid() ) {
        //dbg("shifting this line", gShift, "places." );
        gShift = filler - oldIndent;
   }

    return filler;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
