view.setSelection(new Range(1, 0, 3, 1));
duplicateLinesDown();
var s = view.selection();
if (s.start.line == 4 && s.start.column == 0 && s.end.line == 6 && s.end.column == 1) {
  view.clearSelection();
  view.setCursorPosition(8, 0);
  view.type("PASS");
}