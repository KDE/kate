#! /bin/sh
$EXTRACTRC *.ui *.rc >> rc.cpp
$XGETTEXT *.cpp -o $podir/acpclient.pot
