add_library(kategitblameplugin MODULE "")
target_compile_definitions(kategitblameplugin PRIVATE TRANSLATION_DOMAIN="kategitblameplugin")
target_link_libraries(kategitblameplugin PRIVATE KF5::TextEditor)

target_sources(
  kategitblameplugin
  PRIVATE
    kategitblameplugin.cpp
    gitblametooltip.cpp
)

kcoreaddons_desktop_to_json(kategitblameplugin kategitblameplugin.desktop)
install(TARGETS kategitblameplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
