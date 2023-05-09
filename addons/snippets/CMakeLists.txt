find_package(KF${KF_MAJOR_VERSION}NewStuff ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF${KF_MAJOR_VERSION}NewStuff PROPERTIES PURPOSE "Required to build the snippets addon")

if(NOT KF${KF_MAJOR_VERSION}NewStuff_FOUND)
  return()
endif()

kate_add_plugin(katesnippetsplugin)
target_compile_definitions(katesnippetsplugin PRIVATE TRANSLATION_DOMAIN="katesnippetsplugin")

target_link_libraries(
  katesnippetsplugin 
  PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::NewStuffWidgets
    KF${KF_MAJOR_VERSION}::TextEditor
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
