#! /bin/sh
$EXTRACTRC *.rc >> rc.cpp

for fname in `find ../templates -iname '*.katetemplate'`; do
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

$XGETTEXT *.cpp -o $podir/katefiletemplates.pot
