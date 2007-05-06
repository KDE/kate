# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/completion
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

if(KDE4_ENABLE_FINAL)
	macro_add_file_dependencies(${CMAKE_CURRENT_BINARY_DIR}/katepart_final_cpp.cpp ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h )
else(KDE4_ENABLE_FINAL)		
	macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h )
endif(KDE4_ENABLE_FINAL)

# our sources
set(katepart_PART_SRCS

# completion (widget, model, delegate, ...)
completion/katecompletionwidget.cpp
completion/katecompletionmodel.cpp
completion/katecompletiondelegate.cpp
completion/katecompletiontree.cpp
completion/katecompletionconfig.cpp

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
syntax/katehighlight.cpp
syntax/katesyntaxdocument.cpp
syntax/katecodefolding.cpp
syntax/kateextendedattribute.cpp

# view stuff (THE view and it's helpers)
view/kateview.cpp
view/kateviewinternal.cpp
view/kateviewhelpers.cpp


# generic stuff, unsorted...
katesearchbar.cpp
katebuffer.cpp
katecmds.cpp
kateundo.cpp
katecursor.cpp
katedialogs.cpp
katedocument.cpp
katefactory.cpp
katetextline.cpp
kateconfig.cpp
katedocumenthelpers.cpp
katebookmarks.cpp
kateautoindent.cpp
katefiletype.cpp
kateschema.cpp
katetemplatehandler.cpp
katespell.cpp
kateprinter.cpp
kateglobal.cpp
katecmd.cpp
kateedit.cpp
katedynamicanimation.cpp
katejscript.cpp
katestyletreewidget.cpp
)

kde4_automoc(${katepart_PART_SRCS})

set( katepart_PART_UI
ui//appearanceconfigwidget.ui
ui//commandmenuconfigwidget.ui
ui//commandmenueditwidget.ui
ui//cursorconfigwidget.ui
ui//editconfigwidget.ui
ui//filetypeconfigwidget.ui
ui//hlconfigwidget.ui
ui//indentationconfigwidget.ui
ui//opensaveconfigwidget.ui
ui//modonhdwidget.ui
ui//schemaconfigcolortab.ui
ui//completionconfigwidget.ui
)

kde4_add_ui_files(katepart_PART_SRCS ${katepart_PART_UI} )

kde4_add_plugin(katepart ${katepart_PART_SRCS})

# linking
target_link_libraries(katepart ${KDE4_KDECORE_LIBS} ktexteditor kdeprint kjs kde3support )

# install the stuff
install(TARGETS katepart  DESTINATION ${PLUGIN_INSTALL_DIR} )
