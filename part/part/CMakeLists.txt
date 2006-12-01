# includes
include_directories( ${CMAKE_SOURCE_DIR}/kdeprint ${CMAKE_SOURCE_DIR}/interfaces/kregexpeditor ${CMAKE_SOURCE_DIR}/kutils ${CMAKE_SOURCE_DIR}/kde3support/kdeui)


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
katesearch.cpp
katesearchbar.cpp
katebuffer.cpp
katecmds.cpp
kateundo.cpp
katecursor.cpp
katedialogs.cpp
katedocument.cpp
katefactory.cpp
katehighlight.cpp
katesyntaxdocument.cpp
katetextline.cpp
kateview.cpp
kateconfig.cpp
kateviewhelpers.cpp
katedocumenthelpers.cpp
katecodefoldinghelpers.cpp
kateviewinternal.cpp
katebookmarks.cpp
katelinerange.cpp
katesmartcursor.cpp
katerenderer.cpp
kateautoindent.cpp
katefiletype.cpp
kateschema.cpp
katetemplatehandler.cpp
katespell.cpp
kateprinter.cpp
katesmartrange.cpp
kateglobal.cpp
katecmd.cpp
katelayoutcache.cpp
katetextlayout.cpp
katesmartmanager.cpp
kateedit.cpp
katesmartcursornotifier.cpp
katerenderrange.cpp
katesmartregion.cpp
katedynamicanimation.cpp
kateextendedattribute.cpp
katecompletionwidget.cpp
katecompletionmodel.cpp
katecompletiondelegate.cpp
katecompletiontree.cpp
katecompletionconfig.cpp
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
target_link_libraries(katepart ${KDE4_KDECORE_LIBS} ktexteditor kdeprint kutils kjs knewstuff kde3support )

# install the stuff
install(TARGETS katepart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### tests ###############

# Tests don't need to go into toplevel/bin, they are fine in the current dir.
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# This can be used for finding data files in the source dir, without installing them
#add_definitions( -DKDESRCDIR=\\"${CMAKE_CURRENT_SOURCE_DIR}\\" )

#include_directories( ${KDE4_KDECORE_INCLUDES} )

#set(testkateregression_SRCS ${katepart_PART_SRCS} test_regression.cpp test_regression.h )

#kde4_automoc(${testkateregression_SRCS})

#kde4_add_executable(testkateregression NOGUI RUN_UNINSTALLED ${testkateregression_SRCS})

#target_link_libraries(testkateregression  ${KDE4_KDECORE_LIBS} ktexteditor kdeprint kutils kjs knewstuff kde3support )

#add_dependencies(check testkateregression)

#add_custom_target(check ${CMAKE_CURRENT_BINARY_DIR}/testkateregression DEPENDS testkateregression WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
