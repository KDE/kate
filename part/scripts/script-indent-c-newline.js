var tabWidth = 4;
var spaceIndent = true;
var indentWidth = 4;

var strIndentCharacters = "    ";
var strIndentFiller = "";

var intStartLine = view.cursorLine();
var intStartColumn = view.cursorColumn();

var strTextLine = document.textLine( intStartLine  );
var strPrevLine = document.textLine( intStartLine  - 1 );

var addIndent = "";

function firstNonSpace( _text )
{
    for( _i=0; _i < _text.length; _i++ )
    {
        _ch = _text.charAt( _i );
        if( _ch != ' ' && _ch != '\t' )
            return _i;
    }

    return -1;
}

function lastNonSpace( _text )
{
    for( _i=_text.length - 1; _i >= 0; _i-- )
    {
        _ch = _text.charAt( _i );
        if( _ch != ' ' && _ch != '\t' )
            return _i;
    }

    return -1;
}


// if previous line ends with a '{' increase indent level
// if ( prevLine.search( /{\s*$/ ) != -1 )
// {
//     if ( spaceIndent )
//         addIndent = "    ";
//     else
//         addIndent = "\t";
// }
// else
{
    var intCurrentLine = intStartLine;
    var openParenCount = 0;
    var openBraceCount = 0;

    label_while:
    while ( intCurrentLine > 0 )
    {
        intCurrentLine--;

        strCurrentLine = document.textLine( intCurrentLine );
        intLastChar = lastNonSpace( strCurrentLine );
        intFirstChar = firstNonSpace( strCurrentLine ) ;

        if ( strCurrentLine.search( /\/\// ) == -1 )
        {

            // look through line backwards for interesting characters
            for( intCurrentChar = intLastChar; intCurrentChar >= intFirstChar; --intCurrentChar )
            {
                ch = strCurrentLine.charAt( intCurrentChar );
                switch( ch )
                {
                case '(': case '[':
                    if( ++openParenCount > 0 )
                    break label_while; //return calcIndentInBracket( begin, cur, pos );
                    break;
                case ')': case ']': openParenCount--; break;
                case '{':
                    if( ++openBraceCount > 0 )
                    break label_while; //return calcIndentInBrace( begin, cur, pos );
                    break;
                case '}': openBraceCount--; lookingForScopeKeywords = false; break;
                case ';':
                    if( openParenCount == 0 )
                    lookingForScopeKeywords = false;
                    break;
                }
            }
        }
    }

    strIndentFiller += strCurrentLine.match(/^\s+/);
    if ( strIndentFiller == "null" )
        strIndentFiller = "";

    debug( "line: " + intCurrentLine);
    debug( openParenCount + ", " + openBraceCount);

    while( openParenCount > 0 )
    {
        openParenCount--;
        strIndentFiller += strIndentCharacters;
    }

    while( openBraceCount > 0 )
    {
        openBraceCount--;
        strIndentFiller += strIndentCharacters;
    }
}

document.insertText( intStartLine, 0, strIndentFiller );
view.setCursorPositionReal( intStartLine, document.textLine( intStartLine ).length );
