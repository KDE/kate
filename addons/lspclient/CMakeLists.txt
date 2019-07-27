find_package(KF5ItemModels QUIET)
set_package_properties(KF5ItemModels PROPERTIES PURPOSE "Required to build the lspclient addon")

if(NOT KF5ItemModels_FOUND)
  return()
endif()

set(lspclientplugin_SRCS
  lspclientcompletion.cpp
  lspclientconfigpage.cpp
  lspclienthover.cpp
  lspclientplugin.cpp
  lspclientpluginview.cpp
  lspclientserver.cpp
  lspclientservermanager.cpp
  lspclientsymbolview.cpp
  plugin.qrc
)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  lspclientplugin_debug_SRCS
  HEADER lspclient_debug.h
  IDENTIFIER LSPCLIENT
  CATEGORY_NAME "katelspclientplugin"
)

add_library(lspclientplugin MODULE ${lspclientplugin_SRCS} ${lspclientplugin_debug_SRCS})
target_compile_definitions(lspclientplugin PRIVATE TRANSLATION_DOMAIN="lspclient")

target_link_libraries(
  lspclientplugin 
  PRIVATE 
    KF5::ItemModels
    KF5::TextEditor
)

kcoreaddons_desktop_to_json(lspclientplugin lspclientplugin.desktop)
install(TARGETS lspclientplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
