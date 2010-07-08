# define project
project(katepart)

# these subdirs have their own CMakeLists
add_subdirectory( data )

# syntax highlighting data files
add_subdirectory( syntax/data )

# jscripts for the part
add_subdirectory( script/data )

add_subdirectory( plugins )

# tests
add_subdirectory( tests )

# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/buffer
  ${CMAKE_CURRENT_SOURCE_DIR}/completion
  ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/document
  ${CMAKE_CURRENT_SOURCE_DIR}/script
  ${CMAKE_CURRENT_SOURCE_DIR}/mode
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/search
  ${CMAKE_CURRENT_SOURCE_DIR}/smart
  ${CMAKE_CURRENT_SOURCE_DIR}/syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/undo
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/vimode
  ${CMAKE_CURRENT_SOURCE_DIR}/view
  ${CMAKE_CURRENT_SOURCE_DIR}/swapfile
  ${CMAKE_SOURCE_DIR}/kutils
  ${KDE4_KIO_INCLUDES}
)

# our sources
set(katepart_PART_SRCS

# text buffer
buffer/katetextbuffer.cpp
buffer/katetextblock.cpp
buffer/katetextline.cpp
buffer/katetextcursor.cpp
buffer/katetextrange.cpp
buffer/katetexthistory.cpp

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

# simple internal word completion
completion/katewordcompletion.cpp

# dialogs
dialogs/katedialogs.cpp

# document (THE document, buffer, lines/cursors/..., CORE STUFF)
document/katedocument.cpp
document/katedocumenthelpers.cpp
document/katebuffer.cpp
document/kateedit.cpp

# undo
undo/kateundo.cpp
undo/kateundomanager.cpp

# scripting
script/katescript.cpp
script/kateindentscript.cpp
script/katecommandlinescript.cpp
script/katetemplatescript.cpp
script/katescriptmanager.cpp
script/katescriptaction.cpp

# scripting wrappers
script/katescriptdocument.cpp
script/katescriptview.cpp
script/katescripthelpers.cpp

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

# search stuff
search/kateregexp.cpp
search/kateplaintextsearch.cpp
search/kateregexpsearch.cpp
search/katematch.cpp
search/katesearchbar.cpp

# smart stuff (cursors, ranges, ...)
smart/katesmartcursor.cpp
smart/katesmartrange.cpp
smart/katesmartmanager.cpp
smart/katesmartgroup.cpp

# syntax related stuff (highlighting, xml file parsing, folding, ...)
syntax/katesyntaxmanager.cpp
syntax/katehighlight.cpp
syntax/katehighlighthelpers.cpp
syntax/katehighlightmenu.cpp
syntax/katesyntaxdocument.cpp
syntax/katecodefolding.cpp
syntax/kateextendedattribute.cpp

# view stuff (THE view and its helpers)
view/kateview.cpp
view/kateviewinternal.cpp
view/kateviewhelpers.cpp

# vi input mode
vimode/kateviinputmodemanager.cpp
vimode/katevimodebase.cpp
vimode/katevinormalmode.cpp
vimode/kateviinsertmode.cpp
vimode/katevicommand.cpp
vimode/katevimotion.cpp
vimode/katevirange.cpp
vimode/katevikeyparser.cpp
vimode/kateviglobal.cpp
vimode/katevivisualmode.cpp
vimode/katevireplacemode.cpp
vimode/katevimodebar.cpp

# spell checking
spellcheck/prefixstore.h
spellcheck/prefixstore.cpp
spellcheck/ontheflycheck.h
spellcheck/ontheflycheck.cpp
spellcheck/spellcheck.h
spellcheck/spellcheck.cpp
spellcheck/spellcheckdialog.h
spellcheck/spellcheckdialog.cpp
spellcheck/spellingmenu.h
spellcheck/spellingmenu.cpp

# generic stuff, unsorted...
utils/katecmds.cpp
utils/kateconfig.cpp
utils/katebookmarks.cpp
utils/kateautoindent.cpp
utils/kateschema.cpp
utils/katetemplatehandler.cpp
utils/kateprinter.cpp
utils/kateglobal.cpp
utils/katecmd.cpp
utils/katestyletreewidget.cpp
utils/katepartpluginmanager.cpp

# swapfile
swapfile/katerecoverbar.cpp
swapfile/kateswapfile.cpp

)

set( katepart_PART_UI
dialogs/textareaappearanceconfigwidget.ui
dialogs/bordersappearanceconfigwidget.ui
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
search/searchbarincremental.ui
search/searchbarpower.ui
dialogs/viinputmodeconfigwidget.ui
dialogs/spellcheckconfigwidget.ui
swapfile/recoverwidget.ui
)

add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13000 )

kde4_add_ui_files(katepart_PART_SRCS ${katepart_PART_UI} )

kde4_add_library (katepartinterfaces SHARED ${katepart_PART_SRCS} )

target_link_libraries (
  katepartinterfaces ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS}
  ${KDE4_KCMUTILS_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${QT_QTSCRIPT_LIBRARY} ${KDE_4_4_LIBS_NEEDED}
)
target_link_libraries(katepartinterfaces LINK_INTERFACE_LIBRARIES "${KDE4_KPARTS_LIBS}" )

set_target_properties(
  katepartinterfaces PROPERTIES
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
)

# kde 4 final support
if(KDE4_ENABLE_FINAL)
   macro_add_file_dependencies(${CMAKE_CURRENT_BINARY_DIR}/katepartinterfaces_final_cpp.cpp )
else(KDE4_ENABLE_FINAL)
   macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/script/katescript.cpp )
endif(KDE4_ENABLE_FINAL)

# install kate part interfaces
install (TARGETS katepartinterfaces EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

# kate part itself just is interfaces + the factory
kde4_add_plugin (katepart utils/katefactory.cpp)

# link the part, use kate part interfaces + kde stuff
target_link_libraries (katepart
  ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS}
  ${KDE4_KCMUTILS_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${QT_QTSCRIPT_LIBRARY} katepartinterfaces)

# install the part
install (TARGETS katepart DESTINATION ${PLUGIN_INSTALL_DIR})
