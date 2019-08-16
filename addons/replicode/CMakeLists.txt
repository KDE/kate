find_package(KF5IconThemes QUIET)
set_package_properties(KF5IconThemes PROPERTIES PURPOSE "Required to build the replicode addon")

if(NOT KF5IconThemes_FOUND)
  return()
endif()

add_library(katereplicodeplugin MODULE "")
target_compile_definitions(katereplicodeplugin PRIVATE TRANSLATION_DOMAIN="kate-replicode-plugin")

target_link_libraries(
  katereplicodeplugin 
  PRIVATE 
    KF5::TextEditor
    KF5::IconThemes
)

ki18n_wrap_ui(UI_SOURCES config.ui)
target_sources(katereplicodeplugin PRIVATE ${UI_SOURCES})

target_sources(
  katereplicodeplugin
  PRIVATE
    replicodeplugin.cpp
    replicodeconfigpage.cpp
    replicodeconfig.cpp
    replicodesettings.cpp
    replicodeview.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(katereplicodeplugin katereplicodeplugin.desktop)
install(TARGETS katereplicodeplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
