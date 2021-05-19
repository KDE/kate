kcoreaddons_add_plugin(katereplicodeplugin INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(katereplicodeplugin PRIVATE TRANSLATION_DOMAIN="kate-replicode-plugin")

target_link_libraries(
  katereplicodeplugin
  PRIVATE
    KF5::I18n
    KF5::TextEditor
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

