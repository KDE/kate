if( view.hasSelection() )
{
    start = view.selectionStartLine;
    end = view.selectionEndLine;

    txt = document.textRange( start, 0, end, document.lineLength( end ) );

    repl = txt.split("\n");
    repl.sort();
    txt = repl.join("\n");

    view.clearSelection();

    document.editBegin();
    document.removeText( start, 0, end, document.lineLength( end ) );
    document.insertText( start, 0, txt );
    document.editEnd();
}