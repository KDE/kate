var tabWidth = 4;
var spaceIndent = true;
var indentWidth = 4;

var line = view.cursorLine();
var col = view.cursorColumn();

var textLine = document.textLine( line );
var prevLine = document.textLine( line - 1 );

var prevIndent = prevLine.match(/^\s*/);
var addIndent = "";

function unindent()
{
//     if (
}

// unindent } and {, if not in a comment
if ( textLine.search( /^\s*\/\// ) == -1 )
{
    if ( textLine.charAt( col-1 ) == '}' || textLine.charAt( col-1 ) == '{')
    {
        if ( textLine.search(/^\s\s\s\s/) != -1)
        {
            document.removeText( line, 0, line, tabWidth );
            view.setCursorPositionReal( line, col - tabWidth );
        }
    }
}
