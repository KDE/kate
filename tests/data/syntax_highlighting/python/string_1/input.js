var pass = true;
var line = 2;
for (column = 4; column <= 8; ++column) {
  if (!document.isString(line,column) || document.isComment(line, column)) {
    pass = false;
    v.setCursorPosition(line, document.lineLength(line));
    v.type("|fail" + column);
  }
}
v.setCursorPosition(document.lines() - 1, 0);
if (pass) {
  v.type("pass");
} else {
  v.type("fail (the position of the failure is marked at the end of the affected line(s))");
}
v.enter();
