// kate-script
// help: Stupid test script.
var test = 1;
var start = document.lines();

document.editBegin();

for (var t = 0; t < 100; t++) {
    document.insertLine(start+t, document.lineLength(0));
    document.insertText(start+t, 0, " muh");
    document.insertText(start+t, 0, view.cursorLine());
    document.insertText(start+t, 0, "// ");
}

document.editEnd();

