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

for fname in `find -iname '*.katetemplate'`; do
  for compound in template@item:inmenu \
                  group@item:inmenu \
                  description@info:whatsthis \
                  author@info:credit \
               ; do
    field=`echo $compound | sed 's/\(.*\)@.*/\1/'`
    ctxmark=`echo $compound | sed 's/.*\(@.*\)/\1/'`
      grep -inH "^katetemplate:.*$field=" $fname \
    | sed "s/\\([^:]*\\):\\([^:]*\\):.*$field= *\\([^=]*\\)\\( .*\\)\\?\$/\
           \/\/i18n: file \1 line \2\n\
           i18nc(\"$ctxmark\", \"\3\");\
           /i" \
    >> rc.cpp
  done
done

echo "rc.cpp" > files
find $dirs -maxdepth 1 -name "*.cpp" -print >> files
find $dirs -maxdepth 1 -name "*.cc" -print >> files
find $dirs -maxdepth 1 -name "*.h" -print >> files
$EXTRACTRC `find $dirs -maxdepth 1 \( -name \*.rc -o -name \*.ui -o -name \*.kcfg \) ` >> rc.cpp || exit 11
$XGETTEXT --files-from=files -o $podir/kate.pot
rm -f tips.cpp dirs files rc.cpp
