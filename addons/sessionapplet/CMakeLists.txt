find_package(KF5Plasma QUIET)
set_package_properties(KF5Plasma PROPERTIES PURPOSE "Required to build the sessionapplet addon")

find_package(Qt5Widgets QUIET)
set_package_properties(Qt5Widgets PROPERTIES PURPOSE "Required to build the sessionapplet addon")

if(NOT KF5Plasma_FOUND OR NOT Qt5Widgets_FOUND)
  return()
endif()

add_library(plasma_engine_katesessions MODULE "")
target_compile_definitions(plasma_engine_katesessions PRIVATE TRANSLATION_DOMAIN="plasma_applet_org.kde.plasma.katesessions")

target_link_libraries(
  plasma_engine_katesessions 
  PRIVATE
    KF5::I18n
    KF5::Plasma
    Qt5::Widgets
)

target_sources(
  plasma_engine_katesessions 
  PRIVATE
    katesessionsengine.cpp 
    katesessionsmodel.cpp 
    katesessionsjob.cpp 
    katesessionsservice.cpp
)

plasma_install_package(applet org.kde.plasma.katesessions)
kcoreaddons_desktop_to_json(plasma_engine_katesessions plasma-dataengine-katesessions.desktop)
install(TARGETS plasma_engine_katesessions DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-katesessions.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES org.kde.plasma.katesessions.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

