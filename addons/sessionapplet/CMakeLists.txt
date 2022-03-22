find_package(KF5Plasma ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5Plasma PROPERTIES PURPOSE "Required to build the sessionapplet addon")

find_package(Qt${QT_MAJOR_VERSION}Widgets ${QT_MIN_VERSION} QUIET)
set_package_properties(Qt5Widgets PROPERTIES PURPOSE "Required to build the sessionapplet addon")

if(NOT KF5Plasma_FOUND OR NOT Qt5Widgets_FOUND)
  return()
endif()

kcoreaddons_add_plugin(plasma_engine_katesessions INSTALL_NAMESPACE "plasma/dataengine")
target_compile_definitions(plasma_engine_katesessions PRIVATE TRANSLATION_DOMAIN="plasma_applet_org.kde.plasma.katesessions")

target_link_libraries(
  plasma_engine_katesessions 
  PRIVATE
    KF5::I18n
    KF5::Plasma
    KF5::KIOGui
    Qt::Widgets
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

install(FILES plasma-dataengine-katesessions.desktop DESTINATION ${KDE_INSTALL_KSERVICESDIR})
install(FILES org.kde.plasma.katesessions.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

