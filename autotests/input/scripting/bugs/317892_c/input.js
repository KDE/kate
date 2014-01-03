view.setCursorPosition(2, 0);
if (document.isComment(2, 0))
  view.type("PASS");
else
  view.type("FAIL");
