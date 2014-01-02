# Kate Part

## Licensing

Contributions to Kate Part shall be licensed under LGPLv2+.

At the moment the following source files still have licensing issues:

### *.h

cullmann@altair:/local/cullmann/kf5/src/kate> find part -name "*.h" -exec ../ninka/ninka.pl {} \; | grep -v LibraryGPLv2\+
part/dialogs/katedialogs.h;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
part/document/katebuffer.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/document/katedocument.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/mode/katewildcardmatcher.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/render/katerenderer.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/schema/katestyletreewidget.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/script/kateindentscript.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescript.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescriptdocument.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescriptmanager.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescriptview.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/syntax/kateextendedattribute.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/katehighlight.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/katesyntaxdocument.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/katesyntaxmanager.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/undo/kateundo.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/undo/kateundomanager.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/utils/kateautoindent.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/utils/katebookmarks.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/utils/kateconfig.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/view/kateview.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/view/kateviewhelpers.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/view/kateviewinternal.h;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
part/katepartdebug.h;LibraryGPLv2;1;1;4;1;0;,1,-1,-1,-1,-1

### *.cpp

cullmann@altair:/local/cullmann/kf5/src/kate> find part -name "*.cpp" -exec ../ninka/ninka.pl {} \; | grep -v LibraryGPLv2\+
part/dialogs/katedialogs.cpp;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
part/document/katebuffer.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/document/katedocument.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/render/katerenderer.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/script/katescriptview.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/kateindentscript.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescript.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescriptdocument.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
part/script/katescriptmanager.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/search/katesearchbar.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/kateextendedattribute.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/katehighlight.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/katesyntaxdocument.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/syntax/katesyntaxmanager.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/utils/kateautoindent.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/utils/katebookmarks.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/utils/kateconfig.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/view/kateviewhelpers.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
part/view/kateviewinternal.cpp;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
part/view/kateview.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
