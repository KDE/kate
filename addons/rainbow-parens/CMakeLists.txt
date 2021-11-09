add_library(rainbowparens MODULE "")
target_compile_definitions(rainbowparens PRIVATE TRANSLATION_DOMAIN="rainbowparens")
target_link_libraries(rainbowparens PRIVATE KF5::TextEditor)

target_sources(
  rainbowparens
  PRIVATE
    rainbowparens_plugin.cpp
)

# ensure we are able to load plugins pre-install, too, directories must match!
set_target_properties(rainbowparens PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/ktexteditor")
install(TARGETS rainbowparens DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
