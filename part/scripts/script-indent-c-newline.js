var tabWidth = 4;
var spaceIndent = true;
var indentWidth = 4;

var line = view.cursorLine();
var col = view.cursorColumn();

var textLine = document.textLine( line );
var prevLine = document.textLine( line - 1 );

var prevIndent = prevLine.match(/^\s*/);
var addIndent = "";


// if previous line ends with a '{' increase indent level
if ( prevLine.search( /[{)]\s*$/ ) != -1 )
{
    if ( spaceIndent )
        addIndent = "    ";
    else
        addIndent = "\t";
}

//document.insertText(0, 0, line + ":" + col);
document.insertText( line, col, prevIndent + addIndent );
view.setCursorPositionReal( line, document.textLine( line ).length )

// document.insertText( 0, 0, view.cursorLine() + ":" + view.cursorColumnReal() );
// document.insertText( 0, 0, view.cursorLine() + ":" + view.cursorColumn() );
