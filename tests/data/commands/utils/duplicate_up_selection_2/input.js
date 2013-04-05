view.setSelection(new Range(1, 1, 1, 2));
duplicateLinesUp();
var s = view.selection();
if (s.start.line == 1 && s.start.column == 1 && s.end.line == 1 && s.end.column == 2) {
  view.clearSelection();
  view.setCursorPosition(4, 0);
  view.type("PASS");
}