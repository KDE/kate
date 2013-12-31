#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp || exit 11
$EXTRACTATTR --attr=language,name,Language --attr="language,section,Language Section" syntax/data/*.xml >> rc.cpp || exit 12
grep -n '^ *\* *name:' script/data/indentation/*.js | sed 's!^\(.*\):.*\bname: *\(.*\)$!// i18n: file: \1\ni18nc("Autoindent mode", "\2");!' | sed 's/ \+")/")/' >>rc.cpp || exit 13
$XGETTEXT `find . -name "*.cpp" -o -name "*.h" -o -name '*.js'` -o $podir/katepart4.pot
