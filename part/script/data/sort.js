/* kate-script
 * name: C++ Indenter
 * author: Dominik Haumann <dhdev@gmx.de>
 * license: LGPL
 * version: 1
 * kate-version: 3.0
 * functions: sorter
 */

function sorter ()
{
    if (view.hasSelection()) {
        var start = view.startOfSelection().line;
        var end = view.endOfSelection().line;

        var text = document.textRange(start, 0, end, document.lineLength(end));

        var lines = text.split("\n");
        lines.sort();
        text = lines.join("\n");

        view.clearSelection();

        document.editBegin();
        document.removeText(start, 0, end, document.lineLength(end));
        document.insertText(start, 0, text);
        document.editEnd();
    }
}