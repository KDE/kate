# define project
project(kate)

# this subdirs have own CMakeLists
add_subdirectory( data )

# syntax highlighting data files
add_subdirectory( syntax/data )

# jscripts for the part
add_subdirectory( script/data )

add_subdirectory( plugins )

# tests
# add_subdirectory( tests )

# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/completion
  ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/document
  ${CMAKE_CURRENT_SOURCE_DIR}/script
  ${CMAKE_CURRENT_SOURCE_DIR}/mode
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/smart
  ${CMAKE_CURRENT_SOURCE_DIR}/syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/view
  ${CMAKE_SOURCE_DIR}/kde3support
  ${CMAKE_SOURCE_DIR}/kde3support/kdeui
  ${CMAKE_SOURCE_DIR}/kjsembed
  ${CMAKE_SOURCE_DIR}/kutils
  ${KDE4_KIO_INCLUDES})

# our sources
set(katepart_PART_SRCS

# completion (widget, model, delegate, ...)
completion/katecompletionwidget.cpp
completion/katecompletionmodel.cpp
completion/katecompletiontree.cpp
completion/katecompletionconfig.cpp
completion/kateargumenthinttree.cpp
completion/kateargumenthintmodel.cpp
completion/katecompletiondelegate.cpp
completion/expandingtree/expandingwidgetmodel.cpp
completion/expandingtree/expandingdelegate.cpp
completion/expandingtree/expandingtree.cpp
# dialogs
dialogs/katedialogs.cpp

# document (THE document, buffer, lines/cursors/..., CORE STUFF)
document/katedocument.cpp
document/katedocumenthelpers.cpp
document/katebuffer.cpp
document/katetextline.cpp
document/katecursor.cpp
document/kateedit.cpp
document/kateundo.cpp

# scripting
script/katescript.cpp
script/kateindentscript.cpp
script/katescriptmanager.cpp
# scripting wrappers
script/katescriptdocument.cpp
script/katescriptview.cpp

# mode (modemanager and co)
mode/katemodemanager.cpp
mode/katemodeconfigpage.cpp
mode/katemodemenu.cpp
mode/katewildcardmatcher.cpp

# rendering stuff (katerenderer  and helpers)
render/katerenderer.cpp
render/katerenderrange.cpp
render/katelayoutcache.cpp
render/katetextlayout.cpp
render/katelinelayout.cpp

# smart stuff (cursors, ranges, ...)
smart/katesmartcursor.cpp
smart/katesmartrange.cpp
smart/katesmartmanager.cpp
smart/katesmartcursornotifier.cpp
smart/katesmartregion.cpp

# syntax related stuff (highlighting, xml file parsing, folding, ...)
syntax/katesyntaxmanager.cpp
syntax/katehighlight.cpp
syntax/katehighlighthelpers.cpp
syntax/katehighlightmenu.cpp
syntax/katesyntaxdocument.cpp
syntax/katecodefolding.cpp
syntax/kateextendedattribute.cpp

# view stuff (THE view and it's helpers)
view/kateview.cpp
view/kateviewinternal.cpp
view/kateviewhelpers.cpp

# generic stuff, unsorted...
utils/katesearchbar.cpp
utils/katecmds.cpp
utils/katefactory.cpp
utils/kateconfig.cpp
utils/katebookmarks.cpp
utils/kateautoindent.cpp
utils/kateschema.cpp
utils/katetemplatehandler.cpp
utils/katespell.cpp
utils/kateprinter.cpp
utils/kateglobal.cpp
utils/katecmd.cpp
utils/katedynamicanimation.cpp
utils/katestyletreewidget.cpp
utils/katepartpluginmanager.cpp
utils/katehistorymodel.cpp
utils/katevicommandparser.cpp

)

set( katepart_PART_UI
dialogs/appearanceconfigwidget.ui
dialogs/commandmenuconfigwidget.ui
dialogs/commandmenueditwidget.ui
dialogs/completionconfigtab.ui
dialogs/cursorconfigwidget.ui
dialogs/editconfigwidget.ui
dialogs/filetypeconfigwidget.ui
dialogs/indentationconfigwidget.ui
dialogs/opensaveconfigwidget.ui
dialogs/opensaveconfigadvwidget.ui
dialogs/modonhdwidget.ui
dialogs/schemaconfigcolortab.ui
dialogs/completionconfigwidget.ui
dialogs/searchbarincremental.ui
dialogs/searchbarpower.ui
)

add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13000 )

kde4_add_ui_files(katepart_PART_SRCS ${katepart_PART_UI} )

kde4_add_plugin(katepart ${katepart_PART_SRCS})

# linking
target_link_libraries(katepart ${KDE4_KDECORE_LIBS} kutils ktexteditor kde3support ${QT_QTSCRIPT_LIBRARY})

if(KDE4_ENABLE_FINAL)
	macro_add_file_dependencies(${CMAKE_CURRENT_BINARY_DIR}/katepart_final_cpp.cpp )
else(KDE4_ENABLE_FINAL)
	macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/script/katescript.cpp )
endif(KDE4_ENABLE_FINAL)

# install the stuff
install(TARGETS katepart  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

if(NOT MINGW)

# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# This can be used for finding data files in the source dir, without installing them
#add_definitions( -DKDESRCDIR=\\"${CMAKE_CURRENT_SOURCE_DIR}\\" )

#include_directories( ${KDE4_KDECORE_INCLUDES} )

set(testkateregression_SRCS ${katepart_PART_SRCS} tests/test_regression.cpp tests/test_regression.h )

kde4_add_executable(testkateregression NOGUI RUN_UNINSTALLED ${testkateregression_SRCS})

target_link_libraries(testkateregression ${KDE4_KDECORE_LIBS} ${QT_QTSCRIPT_LIBRARY} kutils ktexteditor kde3support )

#add_dependencies(check testkateregression)

add_custom_target(check ${CMAKE_CURRENT_BINARY_DIR}/testkateregression DEPENDS testkateregression WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

endif(NOT MINGW)
