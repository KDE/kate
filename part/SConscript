Import('env')

sources = """
part/katesearch.cpp
part/katebuffer.cpp
part/katecmds.cpp
part/kateundo.cpp
part/katecursor.cpp
part/katedialogs.cpp
part/katedocument.cpp
part/katefactory.cpp
part/katehighlight.cpp
part/katesyntaxdocument.cpp
part/katetextline.cpp
part/kateview.cpp
part/kateconfig.cpp
part/kateviewhelpers.cpp
part/katecodecompletion.cpp
part/katedocumenthelpers.cpp
part/katecodefoldinghelpers.cpp
part/kateviewinternal.cpp
part/katebookmarks.cpp
part/katefont.cpp
part/katelinerange.cpp
part/katesmartcursor.cpp
part/katerenderer.cpp
part/kateautoindent.cpp
part/katefiletype.cpp
part/kateschema.cpp
part/katedocument.skel
part/katetemplatehandler.cpp
part/katejscript.cpp
part/katespell.cpp
part/kateprinter.cpp
part/kateindentscriptabstracts.cpp
part/kateluaindentscript.cpp
part/katerange.cpp
part/katesmartrange.cpp
part/kateglobal.cpp
part/katecmd.cpp
part/katelayoutcache.cpp
part/katetextlayout.cpp
part/katesmartmanager.cpp
part/kateedit.cpp
part/katesmartcursornotifier.cpp
part/katerenderrange.cpp
part/katesmartregion.cpp
part/katedynamicanimation.cpp
part/kateextendedattribute.cpp
"""

includes = """
part
tests
../interfaces
../interfaces/kregexpeditor
../kdeprint
../kdefx
../kutils
../kjs"""

obj = env.kdeobj('module')
obj.target = 'katepart'
obj.source = sources
obj.includes = includes
obj.libs = 'kdeui kparts '
obj.libpaths = '../interfaces/ktexteditor ../interfaces/kdocument ../kdecore ../kdeui ../kparts ../kjs ../kio ../dcop'
obj.libs = 'ktexteditor kdocument kdecore kdeui kparts kjs kio DCOP'
obj.uselib = 'QT QTCORE QTGUI QTXML QT3SUPPORT KDE4'


obj = env.kdeobj('program')
obj.target = 'katetest'
obj.source = 'tests/katetest.cpp tests/arbitraryhighlighttest.cpp'
obj.includes = includes
obj.libs = 'ktexteditor kdocument kparts'
obj.libpaths = '../interfaces/ktexteditor ../interfaces/kdocument ../kparts'
obj.uselib = 'QT QTCORE QTGUI QT3SUPPORT KDE4'


# TODO move this to a more generic place
env['BUILDERS']['Hash']    = Builder(action="perl kjs/create_hash_table $SOURCE -i > $TARGET" ,suffix='.lut.h',src_suffix='.cpp')

for i in ["part/katejscript.cpp"]:
    env.Hash(i)
