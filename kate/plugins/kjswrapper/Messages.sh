#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp
$XGETTEXT *.cpp -o $podir/katekjswrapper.pot
rm -f rc.cpp
