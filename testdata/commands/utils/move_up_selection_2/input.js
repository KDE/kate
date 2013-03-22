view.setSelection(new Range(2, 1, 2, 2));
moveLinesUp();
moveLinesUp();
if (view.selectedText() == "S") {
  view.clearSelection();
  view.setCursorPosition(3, 0);
  view.type("PASS");
}