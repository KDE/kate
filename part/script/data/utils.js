/* kate-script
 * author: Dominik Haumann <dhdev@gmx.de>
 * license: LGPL
 * revision: 1
 * kate-version: 3.4
 * type: commands
 * functions: sort, test
 */

function sort()
{
    var selection = view.selection();
    if (selection.isValid()) {
        selection.start.column = 0;
        selection.end.column = document.lineLength(selection.end.line);

        var text = document.text(selection);

        var lines = text.split("\n");
        lines.sort();
        text = lines.join("\n");

        view.clearSelection();

        document.editBegin();
        document.removeText(selection);
        document.insertText(selection.start, text);
        document.editEnd();
    }
}

function test()
{
    var start = new Cursor(3, 6);
    var end = new Cursor(start);
    end.column = 12;

    var range = new Range(start, end);
    debug("line: " + end.line);
    debug("column: " + end.column);
    debug("range:" + range);
    debug("range:" + range.start.line);
    debug("range:" + range.start.column);
    debug("range:" + range.end.line);
    debug("range:" + range.end.column);

    view.setCursorPosition(start);
}

function help(cmd)
{
    if (cmd == "sort") {
        return "Sort the selected text.";
    }
}
