find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)

kate_add_plugin(latexcompletionplugin)
target_compile_definitions(latexcompletionplugin PRIVATE TRANSLATION_DOMAIN="latexcompletionplugin")

target_link_libraries(latexcompletionplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  latexcompletionplugin
  PRIVATE
    latexcompletionplugin.cpp
    completionmodel.cpp
)


if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
