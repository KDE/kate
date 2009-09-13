/* kate-script
 * author: Dominik Haumann <dhdev@gmx.de>
 * license: LGPL
 * revision: 1
 * kate-version: 3.4
 * type: commands
 * functions: sort
 */

function sort()
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
