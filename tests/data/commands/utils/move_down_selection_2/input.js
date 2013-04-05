view.setSelection(new Range(0, 1, 0, 2));
moveLinesDown();
moveLinesDown();
if (view.selectedText() == "S") {
  view.clearSelection();
  view.setCursorPosition(3, 0);
  view.type("PASS");
}