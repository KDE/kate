view.setSelection(new Range(2, 0, 3, 0));
moveLinesUp();
moveLinesUp();
var s = view.selection();
if (s.start.line == 0 && s.start.column == 0 && s.end.line == 1 && s.end.column == 0) {
  view.clearSelection();
  view.setCursorPosition(3, 0);
  view.type("PASS");
}