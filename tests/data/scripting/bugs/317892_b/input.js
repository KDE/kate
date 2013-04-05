view.setCursorPosition(1, document.lineLength(1));
if (document.isComment(1, document.lineLength(1)))
  view.type("PASS");
else
  view.type("FAIL");
