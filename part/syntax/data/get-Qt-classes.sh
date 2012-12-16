#!/bin/bash
#
# Copyright (c) 2012 by Alex Turbov
#
# Grab a documented (officially) class list from Qt project web site:
# http://qt-project.org/doc/qt-${version}/classes.html
#

version=$1
shift

if [ -n "$version" ]; then
  tmp=`mktemp`
  wget -O $tmp http://qt-project.org/doc/qt-${version}/classes.html
  cat $tmp | egrep '^<dd><a href=".*\.html">.*</a></dd>$' \
    | sed -e 's,<dd><a href=".*\.html">\(.*\)</a></dd>,<item> \1 </item>,'
  rm $tmp
else
  cat <<EOF
Usage:
  $0 Qt-version

Note: Only major and minor version required

Example:
  $0 4.8
EOF
fi
