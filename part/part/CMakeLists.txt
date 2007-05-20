# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/completion
  ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/document
  ${CMAKE_CURRENT_SOURCE_DIR}/mode
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/smart
  ${CMAKE_CURRENT_SOURCE_DIR}/syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/view
  ${CMAKE_SOURCE_DIR}/kdeprint
  ${CMAKE_SOURCE_DIR}/kde3support
  ${CMAKE_SOURCE_DIR}/kde3support/kdeui
  ${CMAKE_SOURCE_DIR}/kjsembed )


# kjs hash stuff
set(CREATE_HASH_TABLE ${CMAKE_SOURCE_DIR}/kjs/create_hash_table )

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h
   COMMAND ${PERL_EXECUTABLE} ${CREATE_HASH_TABLE} ${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp >
   ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp
)

# our sources
set(katepart_PART_SRCS

# completion (widget, model, delegate, ...)
completion/katecompletionwidget.cpp
completion/katecompletionmodel.cpp
completion/katecompletiondelegate.cpp
completion/katecompletiontree.cpp
completion/katecompletionconfig.cpp

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

# mode (modemanager and co)
mode/katemodemanager.cpp
mode/katemodeconfigpage.cpp
mode/katemodemenu.cpp

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
syntax/katesyntaxdocument.cpp
syntax/katecodefolding.cpp
syntax/kateextendedattribute.cpp

# view stuff (THE view and it's helpers)
view/kateview.cpp
view/kateviewinternal.cpp
view/kateviewhelpers.cpp

# generic stuff, unsorted...
katesearchbar.cpp
katecmds.cpp
katefactory.cpp
kateconfig.cpp
katebookmarks.cpp
kateautoindent.cpp
kateschema.cpp
katetemplatehandler.cpp
katespell.cpp
kateprinter.cpp
kateglobal.cpp
katecmd.cpp
katedynamicanimation.cpp
katejscript.cpp
katestyletreewidget.cpp
)

kde4_automoc(${katepart_PART_SRCS})

set( katepart_PART_UI
dialogs/appearanceconfigwidget.ui
dialogs/commandmenuconfigwidget.ui
dialogs/commandmenueditwidget.ui
dialogs/cursorconfigwidget.ui
dialogs/editconfigwidget.ui
dialogs/filetypeconfigwidget.ui
dialogs/indentationconfigwidget.ui
dialogs/opensaveconfigwidget.ui
dialogs/modonhdwidget.ui
dialogs/schemaconfigcolortab.ui
dialogs/completionconfigwidget.ui
)

kde4_add_ui_files(katepart_PART_SRCS ${katepart_PART_UI} )

kde4_add_plugin(katepart ${katepart_PART_SRCS})

# linking
target_link_libraries(katepart ${KDE4_KDECORE_LIBS} ktexteditor kdeprint kjs kde3support )

if(KDE4_ENABLE_FINAL)
	macro_add_file_dependencies(${CMAKE_CURRENT_BINARY_DIR}/katepart_final_cpp.cpp ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h )
else(KDE4_ENABLE_FINAL)		
	macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h )
endif(KDE4_ENABLE_FINAL)


# install the stuff
install(TARGETS katepart  DESTINATION ${PLUGIN_INSTALL_DIR} )
