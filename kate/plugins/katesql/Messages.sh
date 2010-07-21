#! /bin/sh
$EXTRACTRC `find . -name \*.ui` >>  rc.cpp
$XGETTEXT *.cpp -o $podir/katesql.pot
rm -f rc.cpp

