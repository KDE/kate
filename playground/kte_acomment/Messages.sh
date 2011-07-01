#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc` >> rc.cpp || exit 11
$EXTRACTRC `find . -name \*.ui` >> rc.cpp || exit 12
$XGETTEXT -kaliasLocal `find . -name \*.cc -o -name \*.cpp -o -name \*.h` rc.cpp -o $podir/kte_acomment.pot
rm -f rc.cpp