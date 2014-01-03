view.setSelection(new Range(1, 0, 4, 0));
duplicateLinesDown();
var s = view.selection();
if (s.start.line == 4 && s.start.column == 0 && s.end.line == 7 && s.end.column == 0) {
  view.clearSelection();
  view.setCursorPosition(8, 0);
  view.type("PASS");
}