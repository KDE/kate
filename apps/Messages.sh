#! /usr/bin/env bash

# we create one kate.pot for the shared lib + the applications, the addons have extra ones
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp" -o -name "*.h"` -o $podir/kate.pot
