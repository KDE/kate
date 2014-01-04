# KTextEditor

## Licensing

Contributions to KTextEditor shall be licensed under LGPLv2+.

At the moment the following source files still have licensing issues:

### *.h

cullmann@altair:/local/cullmann/kf5/src/kate> find part -name "*.h" -exec ../ninka/ninka.pl {} \; | grep -v LibraryGPLv2\+
ktexteditor/dialogs/katedialogs.h;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
ktexteditor/document/katebuffer.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/document/katedocument.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/render/katerenderer.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/script/kateindentscript.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescript.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescriptdocument.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescriptmanager.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescriptview.h;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/syntax/katehighlight.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/syntax/katesyntaxdocument.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/syntax/katesyntaxmanager.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/utils/kateautoindent.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/view/kateview.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/view/kateviewhelpers.h;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/view/kateviewinternal.h;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1

### *.cpp

cullmann@altair:/local/cullmann/kf5/src/kate> find part -name "*.cpp" -exec ../ninka/ninka.pl {} \; | grep -v LibraryGPLv2\+
ktexteditor/dialogs/katedialogs.cpp;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
ktexteditor/document/katedocument.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/render/katerenderer.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/script/katescriptview.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/kateindentscript.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescript.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescriptdocument.cpp;UNKNOWN;0;0;0;5;1;Copyright,UNKNOWN,FSFwarranty,LibraryGPLseeDetailsVer0,LibraryGPLcopyVer0,GPLwrite
ktexteditor/script/katescriptmanager.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/search/katesearchbar.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/syntax/katehighlight.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/syntax/katesyntaxdocument.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/syntax/katesyntaxmanager.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/utils/kateautoindent.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/utils/katebookmarks.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/utils/kateconfig.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/view/kateviewhelpers.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
ktexteditor/view/kateviewinternal.cpp;LibraryGPLv2;1;1;4;3;0;Copyright,Copyright,,1,-1,-1,-1,-1
ktexteditor/view/kateview.cpp;LibraryGPLv2;1;1;4;2;0;Copyright,,1,-1,-1,-1,-1
