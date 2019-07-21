include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

include(ECMQtDeclareLoggingCategory)

add_definitions(-DTRANSLATION_DOMAIN=\"lspclient\")

set(lspclientplugin_SRCS
  lspclientcompletion.cpp
  lspclientconfigpage.cpp
  lspclienthover.cpp
  lspclientplugin.cpp
  lspclientpluginview.cpp
  lspclientserver.cpp
  lspclientservermanager.cpp
  lspclientsymbolview.cpp
)

ecm_qt_declare_logging_category(lspclientplugin_debug_SRCS
    HEADER lspclient_debug.h
    IDENTIFIER LSPCLIENT
    CATEGORY_NAME "katelspclientplugin"
)
# resource for ui file and stuff
qt5_add_resources(lspclientplugin_SRCS plugin.qrc)

add_library(lspclientplugin MODULE ${lspclientplugin_SRCS} ${lspclientplugin_debug_SRCS})

kcoreaddons_desktop_to_json(lspclientplugin lspclientplugin.desktop)

target_link_libraries(lspclientplugin
  KF5::TextEditor
  KF5::XmlGui
  KF5::ItemViews
  KF5::ItemModels
)

install(TARGETS lspclientplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

############# unit tests ################
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
