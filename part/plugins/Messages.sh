#! /usr/bin/env bash
find -name "*.cpp" -print > files
find -name "*.cc" -print >> files
find -name "*.h" -print >> files
$XGETTEXT --files-from=files -o $podir/ktexteditor_plugins.pot
rm -f files
