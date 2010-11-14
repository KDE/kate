#! /bin/sh
$EXTRACTRC *.rc >> rc.cpp
$XGETTEXT *.cpp *.h -o $podir/kateinsertcommand.pot
