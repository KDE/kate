view.setSelection(new Range(1, 0, 2, 0));
duplicateLinesUp();
var s = view.selection();
if (s.start.line == 1 && s.start.column == 0 && s.end.line == 2 && s.end.column == 0) {
  view.clearSelection();
  view.setCursorPosition(4, 0);
  view.type("PASS");
}