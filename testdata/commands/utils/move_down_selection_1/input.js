view.setSelection(new Range(1, 0, 2, 0));
moveLinesDown();
moveLinesDown();
var s = view.selection();
if (s.start.line == 3 && s.start.column == 0 && s.end.line == 4 && s.end.column == 0) {
  view.clearSelection();
  view.setCursorPosition(4, 0);
  view.type("PASS");
}