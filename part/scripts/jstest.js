// kate-script
// help: Test the selection interface.
// If you select some text before running the script, this line will be overwritten.
if ( view.hasSelection() )
{
    document.removeText( 0, 0, 0, document.lineLength(0) );
    document.insertText( 0, 0, "// initial selection: " + view.selectionStartLine + "," + view.selectionStartColumn + " - " + view.selectionEndLine + "," + view.selectionEndColumn );
}
// test selection interface
view.selectAll();
view.setSelection( 0, 0, 0, 20 );
view.clearSelection();

// insert some text and select that
var line = document.lines();
document.insertLine( line, "foo" );
document.insertText( line, 0, "// This is a new line" );
view.setSelection( line, 0, line, document.lineLength(line) );
// Delete that text
view.removeSelectedText();

// insert some text and select that
var line = document.lines();
document.insertLine( line, "bar");
document.insertText( line, 0, "// This is a new line" );
view.setSelection( line, 0, line, document.lineLength(line) );

document.insertText( line, document.lineLength( line ), " Selection is " + view.selectionStartLine + "," + view.selectionStartColumn + " - " + view.selectionEndLine + "," + view.selectionEndColumn );
