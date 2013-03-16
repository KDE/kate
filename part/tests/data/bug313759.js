view.setCursorPosition(0, 0);
var count = document.lines() - 3;
for (var i = 0; i < count; ++i)
  moveLinesDown();
for (var i = 0; i < count; ++i)
  moveLinesUp();
for (var i = 0; i < count; ++i)
  moveLinesDown();
//for (var i = 0; i < count; ++i)
//  moveLinesUp();
