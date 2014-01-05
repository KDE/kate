# KTextEditor interfaces
project (ktexteditor)

# ecm setup, will create version header & cmake file
ecm_setup_version (${KATE_VERSION} VARIABLE_PREFIX KTEXTEDITOR
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/ktexteditor/ktexteditor_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/data/KTextEditorConfigVersion.cmake")

# install version header
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ktexteditor/ktexteditor_version.h DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)

# handle include files, both normal ones and generated ones
add_subdirectory (include)

# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ktexteditor
  ${CMAKE_CURRENT_SOURCE_DIR}/buffer
  ${CMAKE_CURRENT_SOURCE_DIR}/completion
  ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/document
  ${CMAKE_CURRENT_SOURCE_DIR}/script
  ${CMAKE_CURRENT_SOURCE_DIR}/mode
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/search
  ${CMAKE_CURRENT_SOURCE_DIR}/syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/schema
  ${CMAKE_CURRENT_SOURCE_DIR}/undo
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/vimode
  ${CMAKE_CURRENT_SOURCE_DIR}/view
  ${CMAKE_CURRENT_SOURCE_DIR}/swapfile
  ${CMAKE_CURRENT_SOURCE_DIR}/variableeditor
  ${CMAKE_CURRENT_SOURCE_DIR}/kte5)

# KTextEditor interface sources
set(ktexteditor_LIB_SRCS
src/ktexteditor.cpp
src/document.cpp
src/range.cpp
src/documentcursor.cpp
src/templateinterface.cpp
src/templateinterface2.cpp
src/attribute.cpp
src/codecompletioninterface.cpp
src/codecompletionmodel.cpp
src/codecompletionmodelcontrollerinterface.cpp
src/configinterface.cpp
src/configpageinterface.cpp
src/highlightinterface.cpp
src/movinginterface.cpp
src/movingcursor.cpp
src/movingrange.cpp
src/movingrangefeedback.cpp
src/messageinterface.cpp

# application interfaces
src/application.cpp
src/mainwindow.cpp
   
# text buffer & buffer helpers
buffer/katetextbuffer.cpp
buffer/katetextblock.cpp
buffer/katetextline.cpp
buffer/katetextcursor.cpp
buffer/katetextrange.cpp
buffer/katetexthistory.cpp
buffer/katetextfolding.cpp

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

# undo
undo/kateundo.cpp
undo/katemodifiedundo.cpp
undo/kateundomanager.cpp

# scripting
script/katescript.cpp
script/kateindentscript.cpp
script/katecommandlinescript.cpp
script/katetemplatescript.cpp
script/katescriptmanager.cpp
script/katescriptaction.cpp
script/katescriptconsole.cpp

# scripting wrappers
script/katescriptdocument.cpp
script/katescriptview.cpp
script/katescripthelpers.cpp

# mode (modemanager and co)
mode/katemodemanager.cpp
mode/katemodeconfigpage.cpp
mode/katemodemenu.cpp
mode/katewildcardmatcher.cpp

# modeline variable editor
variableeditor/variablelineedit.cpp
variableeditor/variablelistview.cpp
variableeditor/variableeditor.cpp
variableeditor/variableitem.cpp
variableeditor/katehelpbutton.cpp

# printing classes
printing/kateprinter.cpp
printing/printpainter.cpp
printing/printconfigwidgets.cpp

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

# syntax related stuff (highlighting, xml file parsing, ...)
syntax/katesyntaxmanager.cpp
syntax/katehighlight.cpp
syntax/katehighlighthelpers.cpp
syntax/katehighlightmenu.cpp
syntax/katesyntaxdocument.cpp
syntax/kateextendedattribute.cpp

# view stuff (THE view and its helpers)
view/kateview.cpp
view/kateviewinternal.cpp
view/kateviewhelpers.cpp
view/katemessagewidget.cpp
view/katefadeeffect.cpp
view/kateanimation.cpp
view/katetextanimation.cpp

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
vimode/kateviemulatedcommandbar.cpp
vimode/katevicommandrangeexpressionparser.cpp
vimode/katevikeymapper.cpp

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
utils/katetemplatehandler.cpp
utils/kateglobal.cpp
utils/katecmd.cpp
utils/katepartpluginmanager.cpp

# schema
schema/kateschema.cpp
schema/kateschemaconfig.cpp
schema/katestyletreewidget.cpp
schema/katecolortreewidget.cpp
schema/katecategorydrawer.cpp

# swapfile
swapfile/kateswapdiffcreator.cpp
swapfile/kateswapfile.cpp
   )

set( katepart_PART_UI
)

qt5_wrap_ui(ktexteditor_LIB_SRCS 
dialogs/textareaappearanceconfigwidget.ui
dialogs/bordersappearanceconfigwidget.ui
dialogs/commandmenuconfigwidget.ui
dialogs/commandmenueditwidget.ui
dialogs/completionconfigtab.ui
dialogs/navigationconfigwidget.ui
dialogs/editconfigwidget.ui
dialogs/filetypeconfigwidget.ui
dialogs/indentationconfigwidget.ui
dialogs/opensaveconfigwidget.ui
dialogs/opensaveconfigadvwidget.ui
dialogs/modonhdwidget.ui
dialogs/completionconfigwidget.ui
search/searchbarincremental.ui
search/searchbarpower.ui
dialogs/viinputmodeconfigwidget.ui
dialogs/spellcheckconfigwidget.ui
schema/howtoimportschema.ui )
   
add_library (ktexteditor SHARED ${ktexteditor_LIB_SRCS} ${KTEXTEDITOR_PUBLIC_HEADERS})

target_link_libraries(ktexteditor LINK_PUBLIC KF5::Parts
                                  LINK_PRIVATE KF5::I18n
    Qt5::Script
    KF5::Archive
    KF5::GuiAddons
    KF5::I18n
    KF5::IconThemes
    KF5::ItemViews
    KF5::KCMUtils
    KF5::KIOFileWidgets
    KF5::Notifications
    KF5::Parts
    KF5::PrintUtils
    KF5::SonnetCore)
target_include_directories(ktexteditor INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/KTextEditor>")

set_target_properties(ktexteditor
   PROPERTIES VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)

# generate and install export header file
generate_export_header (ktexteditor EXPORT_FILE_NAME "include/KTextEditor/ktexteditor/ktexteditor_export.h")
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/include/KTextEditor/ktexteditor/ktexteditor_export.h DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)

install(TARGETS ktexteditor EXPORT KTextEditor ${INSTALL_TARGETS_DEFAULT_ARGS})

# handle data files, .desktop & .cmake
add_subdirectory (data)

# syntax highlighting data files
add_subdirectory( syntax/data )

# jscripts for the part
add_subdirectory( script/data )

# kate part itself just is core + the factory
add_library (katepart MODULE utils/katefactory.cpp)

# link the part, use kate part core + kde stuff
target_link_libraries (katepart ktexteditor Qt5::Script)

# install the part
install (TARGETS katepart DESTINATION ${PLUGIN_INSTALL_DIR})
