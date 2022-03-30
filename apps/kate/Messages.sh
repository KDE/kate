#! /usr/bin/env bash
$EXTRACTRC `find . ../lib -name \*.rc -o -name \*.ui` >> rc.cpp || exit 11
$XGETTEXT `find . ../lib -name "*.cpp" -o -name "*.h"` -o $podir/kate.pot
