$test = 1;

$start = document.numLines ()

for ($t = 0; $t < 10; $t++)
{
  document.insertLine ($start+$t, document.lineLength(0));
  document.insertText ($start+$t, 1, "muh");
}
