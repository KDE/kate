/** kate-script
 * name: Haskell
 * license: LGPL
 * author: Erlend Hamberg <ehamberg@gmail.com>
 * version: 1
 * kate-version: 3.0
 * type: indentation
 */

// based on Paul Giannaro's Python indenter

String.prototype.startsWith = function(prefix) {
    return this.substring(0, prefix.length) == prefix;
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

var triggerCharacters = "";

// General notes:
// indent() returns the amount of characters (in spaces) to be indented.
// Special indent() return values:
//   -2 = no indent
//   -1 = keep last indent

function indent(line, indentWidth, character) {
     //dbg(document.attribute.toString());
     //dbg("indent character: " + character);
     //dbg("line text: " + document.line(line));
	 var currentLine = document.line(line);
     //dbg("current line: " + currentLine);
	 var lastLine = document.line(line - 1);
	 var lastCharacter = lastLine.lastCharacter();
	 //
	 // we can't really indent line 0
	 if(line == 0)
		 return -2;
	 //
	 // make sure the last line is code
	 if(!document.isCode(line - 1, document.lineLength(line - 1) - 1) && lastCharacter != "\"" && lastCharacter != "'") {
		 //dbg("attributes that we don't want! Returning");
		 return -1;
	 }
	 // otherwise, check the line contents

	 //dbg('line without white space: ' + currentLine.sansWhiteSpace().length);

	 // indent line after 'where' 6 characters for alignment:
	 // ... where foo = 3
	 //     >>>>>>bar = 4
	 if(lastLine.stripWhiteSpace().startsWith('where')) {
		 //dbg('indenting line for where');
		 return document.firstVirtualColumn(line - 1) + 6;
	 }

	 // indent line after 'case' 6 characters for alignment:
	 // case xs of
	 // >>>>>[] -> ...
	 // >>>>>(y:ys) -> ...
	 if(lastLine.stripWhiteSpace().startsWith('case')) {
		 //dbg('indenting line for case');
		 return document.firstVirtualColumn(line - 1) + 5;
	 }

	 // indent line after 'if/else' 3 characters for alignment:
	 // if foo == bar
	 // >>>then baz
	 // >>>else vaff
	 if(lastLine.stripWhiteSpace().startsWith('if')) {
		 //dbg('indenting line for if');
		 return document.firstVirtualColumn(line - 1) + 3;
	 }

	 // indent lines following a line ending with '='
	 if(lastCharacter == '=') {
		 //dbg('indenting for =');
		 return document.firstVirtualColumn(line - 1) + indentWidth;
	 }

	 // indent lines following a line ending with 'do'
	 if(lastLine.stripWhiteSpace().endsWith('do')) {
		 //dbg('indenting for do');
		 return document.firstVirtualColumn(line - 1) + indentWidth;
	 }

	 // !#$%&*+./<=>?@\\^|~-
	 if (lastLine.search(/[!$#%&*+.\/<=>?@\\^|~-]$/) != -1) {
		 //dbg('indenting for operator');
		 return document.firstVirtualColumn(line - 1) + indentWidth;
	 }

	 if (lastLine.search(/^\s*$/) != -1) {
		 dbg('indenting for empty line');
		 return 0;
	 }
			 

	 //dbg('continuing with regular indent');
	 return -1;
}


// kate: space-indent on; indent-width 4; replace-tabs on;
