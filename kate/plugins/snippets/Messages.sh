#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp
$XGETTEXT *.cpp -o $podir/katesnippets.pot
rm -f rc.cpp
