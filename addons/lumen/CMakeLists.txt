include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DTRANSLATION_DOMAIN=\"ktexteditor_lumen\")

set(ktexteditor_lumen_SRCS
  lumen.cpp
  dcd.cpp
  completion.cpp
)

add_library(ktexteditor_lumen MODULE ${ktexteditor_lumen_SRCS})
kcoreaddons_desktop_to_json(ktexteditor_lumen ktexteditor_lumen.desktop)
target_link_libraries(ktexteditor_lumen
  KF5::TextEditor
)

install(TARGETS ktexteditor_lumen DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

