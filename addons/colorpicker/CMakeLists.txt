add_library(katecolorpickerplugin MODULE "")
target_compile_definitions(katecolorpickerplugin PRIVATE TRANSLATION_DOMAIN="katecolorpickerplugin")
target_link_libraries(katecolorpickerplugin PRIVATE KF5::TextEditor)

target_sources(
  katecolorpickerplugin
  PRIVATE
    katecolorpickerplugin.cpp
    colorpickerconfigpage.cpp
)

kcoreaddons_desktop_to_json(katecolorpickerplugin katecolorpickerplugin.desktop)
install(TARGETS katecolorpickerplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
