/**KATE
 *NAME: C style indenter
 *LICENSE: short name of the license
 *COPYRIGHT:
 *Based on work Copyright 2005 by Dominik Haumann
 *Copyright 2005 by Joseph Wenninger
 *Here will be the license text, Dominik has to choose
 * The following line is not empty
 * 
 *An empty line ends this block
 *
 *VERSION: 0.1
 *ANUNKNOWNKEYWORD: Version has to be in the format major.minor (both numbers)
 *IGNOREALSO: All keywords, except COPYRIGHT are expected to have their data on one line
 *UNKNOWN: unknown keywords are simply ignored from the information parser
 *CURRENTLY_KNOWN_KEYWORDS: NAME,VERSION, COPYRIGHT, LICENSE
 *INFORMATION: This block has to begin in the first line at the first character position
 *INFORMATION: It is optional, but at least all files within the kde cvs,
 *INFORMATION: which are ment for publishing are supposed to have at least the
 *INFORMATION: COPYRIGHT block
 *INFORMATION: These files have to be stored as UTF8
 *INFORMATION: The copyright text should be in english
 *INFORMATION: A localiced copyright statement could be put into a blah.desktop file
 **/

/*
function indentChar() // also possible
{*/

function indentChar(c)
{
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
        if ( /*textLine.charAt( col-1 )*/ c == '}' || /*textLine.c( col-1 )*/ c == '{')
        {
            if ( textLine.search(/^\s\s\s\s/) != -1)
            {
               document.removeText( line, 0, line, tabWidth );
               view.setCursorPositionReal( line, col - tabWidth );
          }
       }
    }

}


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

function indentNewLine()
{
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
}

indenter.onchar=indentChar
indenter.onnewline=indentNewLine
