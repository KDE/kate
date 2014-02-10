project(katefilebrowserplugin)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katefilebrowserplugin_PART_SRCS
    katefilebrowserplugin.cpp
    katefilebrowserconfig.cpp
    katefilebrowser.cpp
    katebookmarkhandler.cpp
)

add_library(katefilebrowserplugin MODULE ${katefilebrowserplugin_PART_SRCS})

target_link_libraries(katefilebrowserplugin KF5::TextEditor KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::I18n)

install(TARGETS katefilebrowserplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

kservice_desktop_to_json (katefilebrowserplugin katefilebrowserplugin.desktop)
install( FILES katefilebrowserplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

