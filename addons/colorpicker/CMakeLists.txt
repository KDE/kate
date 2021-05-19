kcoreaddons_add_plugin(katecolorpickerplugin INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(katecolorpickerplugin PRIVATE TRANSLATION_DOMAIN="katecolorpickerplugin")
target_link_libraries(katecolorpickerplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  katecolorpickerplugin
  PRIVATE
    katecolorpickerplugin.cpp
    colorpickerconfigpage.cpp
)

