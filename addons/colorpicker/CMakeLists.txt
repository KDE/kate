kate_add_plugin(katecolorpickerplugin)
target_compile_definitions(katecolorpickerplugin PRIVATE TRANSLATION_DOMAIN="katecolorpickerplugin")
target_link_libraries(katecolorpickerplugin PRIVATE KF6::I18n KF6::TextEditor)

target_sources(
  katecolorpickerplugin
  PRIVATE
    katecolorpickerplugin.cpp
    colorpickerconfigpage.cpp
)

if (BUILD_PCH)
    target_precompile_headers(katecolorpickerplugin REUSE_FROM katepch)
endif()
