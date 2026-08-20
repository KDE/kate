#! /bin/sh
$EXTRACTRC *.rc *.ui >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` -o $podir/kategdbplugin.pot
