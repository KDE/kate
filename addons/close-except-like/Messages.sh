#! /bin/sh
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/katecloseexceptplugin.pot
rm -f rc.cpp
