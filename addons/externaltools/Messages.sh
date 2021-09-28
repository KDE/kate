#! /bin/sh
$EXTRACTRC *.rc *.ui >> rc.cpp
for i in externaltools-config/*; do
    grep -n -e '^ *name=' $i | sed 's!^\(.*\):.*name= *\(.*\) *$!// i18n: file: \1\ni18nc("External tool name", "\2");!' | sed 's/ \+")/")/' >>rc.cpp || exit 13
    grep -n -e '^ *category=' $i | sed 's!^\(.*\):.*category= *\(.*\) *$!// i18n: file: \1\ni18nc("External tool category", "\2");!' | sed 's/ \+")/")/' >>rc.cpp || exit 13
;done
$XGETTEXT *.cpp  -o $podir/kateexternaltoolsplugin.pot
