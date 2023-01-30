kate_add_plugin(katecolorpickerplugin)
target_compile_definitions(katecolorpickerplugin PRIVATE TRANSLATION_DOMAIN="katecolorpickerplugin")
target_link_libraries(katecolorpickerplugin PRIVATE KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

target_sources(
  katecolorpickerplugin
  PRIVATE
    katecolorpickerplugin.cpp
    colorpickerconfigpage.cpp
)

