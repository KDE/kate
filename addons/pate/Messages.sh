#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp
$XGETTEXT `find . -name "*.cpp" -o -name "*.h"` -o $podir/pate.pot
$XGETTEXT --language=Python --join `find . -name "*.py"` -o $podir/pate.pot
