#! /usr/bin/env bash

# we create one kate.pot for the shared lib + the applications, the addons have extra ones
$EXTRACTRC `find lib apps -name \*.rc -o -name \*.ui` >> lib/rc.cpp || exit 11
$XGETTEXT `find lib apps -name "*.cpp" -o -name "*.h"` -o $podir/kate.pot
