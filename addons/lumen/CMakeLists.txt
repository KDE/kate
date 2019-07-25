add_library(ktexteditor_lumen MODULE "")
target_compile_definitions(ktexteditor_lumen PRIVATE TRANSLATION_DOMAIN="ktexteditor_lumen")

target_link_libraries(
  ktexteditor_lumen 
  PRIVATE KF5::TextEditor
)

target_sources(
  ktexteditor_lumen 
  PRIVATE
    lumen.cpp
    dcd.cpp
    completion.cpp
)

kcoreaddons_desktop_to_json(ktexteditor_lumen ktexteditor_lumen.desktop)
install(TARGETS ktexteditor_lumen DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
