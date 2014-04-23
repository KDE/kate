// BUG: #333696

v.setCursorPosition(0,10);
v.type(".");
v.type(".");
v.type("/");
v.type("foo.hh");

v.setCursorPosition(1,21);
v.type(".");
v.type(".");
v.type(".");

v.setCursorPosition(2,10);
v.type(".");
v.type(".");
v.type(".");
