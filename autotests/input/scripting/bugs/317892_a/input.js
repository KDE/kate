// test closed comment
view.setCursorPosition(1, document.lineLength(1));
if (document.isComment(1, 10) && !document.isCode(1, 10))
  view.type("PASS");
else
  view.type("FAIL");

// test open comment
view.setCursorPosition(2, document.lineLength(2));
if (document.isComment(2, document.lineLength(2) - 1))
  view.type("PASS");
else
  view.type("FAIL");
