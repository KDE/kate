#! /bin/sh
$EXTRACTRC *.rc *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/katesnippets_tng.pot
