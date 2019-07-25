set(katesnippetsplugin_PART_SRCS 
  katesnippets.cpp
  katesnippetglobal.cpp
  snippetview.cpp
  snippetstore.cpp
  snippetrepository.cpp
  snippetcompletionmodel.cpp
  snippetcompletionitem.cpp
  snippet.cpp
  editrepository.cpp
  editsnippet.cpp
  plugin.qrc
)

ki18n_wrap_ui(katesnippetsplugin_PART_SRCS snippetview.ui editrepository.ui editsnippet.ui)

add_library(katesnippetsplugin MODULE ${katesnippetsplugin_PART_SRCS})
target_compile_definitions(katesnippetsplugin PRIVATE TRANSLATION_DOMAIN="katesnippetsplugin")

target_link_libraries(
  katesnippetsplugin 
  PRIVATE
    KF5::TextEditor
    KF5::Parts 
    KF5::I18n
    KF5::NewStuff 
    KF5::ItemViews 
    KF5::IconThemes
)

kcoreaddons_desktop_to_json(katesnippetsplugin katesnippetsplugin.desktop)
install(TARGETS katesnippetsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
