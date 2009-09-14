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
    if (view.hasSelection()) {
        var start = view.startOfSelection().line;
        var end = view.endOfSelection().line;

        var text = document.text(start, 0, end, document.lineLength(end));

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
