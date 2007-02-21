#! /bin/sh
(cd data && $PREPARETIPS > ../tips.cpp)
$EXTRACTRC ./*/*.rc >> ./rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp"` -o $podir/kate.pot
rm -f tips.cpp
