#! /usr/bin/env bash
$EXTRACTRC ./*/*.rc >> rc.cpp || exit 11
$EXTRACTATTR --attr=language,name,Language --attr="language,section,Language Section" syntax/data/*.xml >> rc.cpp || exit 12
$XGETTEXT `find . -name "*.cpp"` -o $podir/katepart.pot
