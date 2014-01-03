// dbg("before indentation:");
// for (var col = 0; col <= document.lineLength(3); ++col) {
//   dbg("isComment(" + 3 + ", " + col + "): " + document.isComment(3, col));
// }
// dbg("indentation:");
v.setCursorPosition(3, document.lineLength(3));
v.type(">");
// dbg("after indentation:");
// for (var col = 0; col <= document.lineLength(3); ++col) {
//   dbg("isComment(" + 3 + ", " + col + "): " + document.isComment(3, col));
// }
