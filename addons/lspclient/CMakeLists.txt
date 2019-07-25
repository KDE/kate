include(ECMQtDeclareLoggingCategory)

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
    KF5::TextEditor
    KF5::XmlGui
    KF5::ItemViews
    KF5::ItemModels
)

kcoreaddons_desktop_to_json(lspclientplugin lspclientplugin.desktop)
install(TARGETS lspclientplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
