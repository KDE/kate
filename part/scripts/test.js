$test = 1;

$start = document.numLines ()

document.editBegin();
for ($t = 0; $t < 1000; $t++)
{
  document.insertLine ($start+$t, document.lineLength(0));
  document.insertText ($start+$t, 0, " muh");
  document.insertText ($start+$t, 0, view.cursorLine());
}
document.editEnd();

