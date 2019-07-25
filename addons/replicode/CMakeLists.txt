set(katereplicode_SRCS
  replicodeplugin.cpp
  replicodeconfigpage.cpp
  replicodeconfig.cpp
  replicodesettings.cpp
  replicodeview.cpp
  plugin.qrc
)

ki18n_wrap_ui(katereplicode_SRCS config.ui)

add_library(katereplicodeplugin MODULE ${katereplicode_SRCS})
target_compile_definitions(katereplicodeplugin PRIVATE TRANSLATION_DOMAIN="kate-replicode-plugin")

target_link_libraries(
  katereplicodeplugin 
  PRIVATE 
    KF5::TextEditor
    KF5::IconThemes
    KF5::I18n
)

kcoreaddons_desktop_to_json(katereplicodeplugin katereplicodeplugin.desktop)
install(TARGETS katereplicodeplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
