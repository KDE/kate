var test = 1;

var start = document.numLines ()

document.editBegin();
for (var t = 0; t < 10000; t++)
{
  document.insertLine (start+t, document.lineLength(0));
  document.insertText (start+t, 0, " muh");
  document.insertText (start+t, 0, view.cursorLine());
}
document.editEnd();
