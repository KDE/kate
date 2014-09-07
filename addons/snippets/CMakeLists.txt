project (katesnippets)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS NewStuff)

########### next target ###############

set(katesnippetsplugin_PART_SRCS katesnippets.cpp
katesnippetglobal.cpp
snippetview.cpp
snippetstore.cpp
snippetrepository.cpp
snippetfilterproxymodel.cpp
snippetcompletionmodel.cpp
snippetcompletionitem.cpp
snippet.cpp
editrepository.cpp
editsnippet.cpp )

ki18n_wrap_ui(katesnippetsplugin_PART_SRCS 
snippetview.ui
editrepository.ui
editsnippet.ui)

add_library (katesnippetsplugin MODULE ${katesnippetsplugin_PART_SRCS})

target_link_libraries(katesnippetsplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n
    KF5::NewStuff Qt5::Script KF5::ItemViews KF5::IconThemes)

install(TARGETS katesnippetsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )


########### install files ###############

kcoreaddons_desktop_to_json (katesnippetsplugin katesnippetsplugin.desktop)
install( FILES katesnippetsplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

# install the snippet stuff
install( FILES ktexteditor_codesnippets_core.knsrc DESTINATION  ${CONFIG_INSTALL_DIR} )

install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/katesnippets)
