find_package(Qt5Test ${QT_MIN_VERSION} QUIET REQUIRED)

add_library(latexcompletionplugin MODULE)
target_compile_definitions(latexcompletionplugin PRIVATE TRANSLATION_DOMAIN="latexcompletionplugin")

target_link_libraries(latexcompletionplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  latexcompletionplugin
  PRIVATE
    latexcompletionplugin.cpp
    completionmodel.cpp
)

# ensure we are able to load plugins pre-install, too, directories must match!
set_target_properties(latexcompletionplugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/ktexteditor")
install(TARGETS latexcompletionplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
