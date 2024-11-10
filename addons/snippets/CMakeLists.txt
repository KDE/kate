find_package(KF6NewStuff ${KF6_DEP_VERSION} QUIET)
set_package_properties(KF6NewStuff PROPERTIES PURPOSE "Required to build the snippets addon")

if(NOT KF6NewStuff_FOUND)
  return()
endif()

kate_add_plugin(katesnippetsplugin)
target_compile_definitions(katesnippetsplugin PRIVATE TRANSLATION_DOMAIN="katesnippetsplugin")

target_link_libraries(
  katesnippetsplugin 
  PRIVATE
    KF6::I18n
    KF6::NewStuffWidgets
    KF6::TextEditor
)

ki18n_wrap_ui(katesnippetsplugin snippetview.ui editrepository.ui editsnippet.ui)

target_sources(
  katesnippetsplugin
  PRIVATE
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

if (BUILD_PCH)
    target_precompile_headers(katesnippetsplugin REUSE_FROM katepch)
endif()
