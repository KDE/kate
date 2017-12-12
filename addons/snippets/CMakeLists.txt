project (katesnippets)
add_definitions(-DTRANSLATION_DOMAIN=\"katesnippetsplugin\")

########### next target ###############

set(katesnippetsplugin_PART_SRCS katesnippets.cpp
katesnippetglobal.cpp
snippetview.cpp
snippetstore.cpp
snippetrepository.cpp
snippetcompletionmodel.cpp
snippetcompletionitem.cpp
snippet.cpp
editrepository.cpp
editsnippet.cpp )

ki18n_wrap_ui(katesnippetsplugin_PART_SRCS 
snippetview.ui
editrepository.ui
editsnippet.ui)

# resource for ui file and stuff
qt5_add_resources(katesnippetsplugin_PART_SRCS plugin.qrc)

add_library (katesnippetsplugin MODULE ${katesnippetsplugin_PART_SRCS})

kcoreaddons_desktop_to_json (katesnippetsplugin katesnippetsplugin.desktop)

target_link_libraries(katesnippetsplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n
    KF5::NewStuff KF5::ItemViews KF5::IconThemes)

install(TARGETS katesnippetsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
