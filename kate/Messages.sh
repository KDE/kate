#! /bin/sh
(cd data && $PREPARETIPS > ../tips.cpp)

find . -type d | fgrep -v '.svn' | sed -e 's,$,/,' > dirs
msh=`find . -name Messages.sh`
for dir in $msh; do
  dir=`dirname $dir`
  if test "$dir" != "."; then
    egrep -v "^$dir" dirs > dirs.new && mv dirs.new dirs
  fi
done
dirs=`cat dirs`

echo "rc.cpp" > files
find $dirs -maxdepth 1 -name "*.cpp" -print >> files
find $dirs -maxdepth 1 -name "*.cc" -print >> files
find $dirs -maxdepth 1 -name "*.h" -print >> files
$EXTRACTRC `find $dirs -maxdepth 1 \( -name \*.rc -o -name \*.ui -o -name \*.kcfg \) ` >> rc.cpp || exit 11
$XGETTEXT --files-from=files -o $podir/kate.pot
rm -f tips.cpp dirs files rc.cpp
