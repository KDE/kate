# Plasma Data Engine
set(plasma_engine_katesessions_SRCS katesessionsengine.cpp katesessionsmodel.cpp katesessionsjob.cpp katesessionsservice.cpp)
add_library(plasma_engine_katesessions MODULE ${plasma_engine_katesessions_SRCS})
kcoreaddons_desktop_to_json(plasma_engine_katesessions plasma-dataengine-katesessions.desktop)
target_link_libraries(plasma_engine_katesessions
    Qt5::Widgets       # QAction
    KF5::IconThemes
    KF5::Plasma
    KF5::Service
    KF5::I18n
)

install(TARGETS plasma_engine_katesessions DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-dataengine-katesessions.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES org.kde.plasma.katesessions.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

