find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

kate_add_plugin(latexcompletionplugin)
target_compile_definitions(latexcompletionplugin PRIVATE TRANSLATION_DOMAIN="latexcompletionplugin")

target_link_libraries(latexcompletionplugin PRIVATE KF6::I18n KF6::TextEditor)

target_sources(
  latexcompletionplugin
  PRIVATE
    latexcompletionplugin.cpp
    completionmodel.cpp
)

if (BUILD_PCH)
    target_precompile_headers(latexcompletionplugin REUSE_FROM katepch)
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
