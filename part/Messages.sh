#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui | egrep -v "/tests" | egrep -v "/plugins"` >> rc.cpp || exit 11
$EXTRACTATTR --attr=language,name,Language --attr="language,section,Language Section" syntax/data/*.xml >> rc.cpp || exit 12
$XGETTEXT `find . -name "*.cpp" -o -name "*.h" -o -name '*.js' | egrep -v "/tests" | egrep -v "/plugins" ` -o $podir/katepart4.pot
