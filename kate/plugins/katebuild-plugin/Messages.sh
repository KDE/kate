#! /bin/sh
$EXTRACTRC `find . -name \*.ui` >>  rc.cpp
$XGETTEXT *.cpp -o $podir/katebuild-plugin.pot
rm -f rc.cpp

