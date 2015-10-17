project(katefilebrowserplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"katefilebrowserplugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katefilebrowserplugin_PART_SRCS
    katefilebrowserplugin.cpp
    katefilebrowserconfig.cpp
    katefilebrowser.cpp
    katebookmarkhandler.cpp
)

add_library(katefilebrowserplugin MODULE ${katefilebrowserplugin_PART_SRCS})

# we compile in the .desktop file
kcoreaddons_desktop_to_json (katefilebrowserplugin katefilebrowserplugin.desktop)

target_link_libraries(katefilebrowserplugin KF5::TextEditor KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::I18n)

install(TARGETS katefilebrowserplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
