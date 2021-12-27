if(WIN32)
  return()
endif()

kate_add_plugin(katekonsoleplugin)
target_compile_definitions(katekonsoleplugin PRIVATE TRANSLATION_DOMAIN="katekonsoleplugin")
target_link_libraries(katekonsoleplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  katekonsoleplugin 
  PRIVATE
    kateconsole.cpp 
    plugin.qrc
)

