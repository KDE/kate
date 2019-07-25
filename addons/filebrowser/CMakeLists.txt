set(katefilebrowserplugin_PART_SRCS
  katefilebrowserplugin.cpp
  katefilebrowserconfig.cpp
  katefilebrowser.cpp
  katebookmarkhandler.cpp
)

add_library(katefilebrowserplugin MODULE ${katefilebrowserplugin_PART_SRCS})
target_compile_definitions(katefilebrowserplugin PRIVATE TRANSLATION_DOMAIN="katefilebrowserplugin")

target_link_libraries(
  katefilebrowserplugin 
  PRIVATE
    KF5::TextEditor 
    KF5::KIOCore 
    KF5::KIOFileWidgets 
    KF5::KIOWidgets 
    KF5::I18n
)

kcoreaddons_desktop_to_json(katefilebrowserplugin katefilebrowserplugin.desktop)
install(TARGETS katefilebrowserplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
