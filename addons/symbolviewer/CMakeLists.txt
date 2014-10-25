########### next target ###############
set(katesymbolviewerplugin_PART_SRCS cpp_parser.cpp tcl_parser.cpp fortran_parser.cpp perl_parser.cpp 
php_parser.cpp xslt_parser.cpp ruby_parser.cpp python_parser.cpp bash_parser.cpp ecma_parser.cpp plugin_katesymbolviewer.cpp )

add_library(katesymbolviewerplugin MODULE ${katesymbolviewerplugin_PART_SRCS})

target_link_libraries(katesymbolviewerplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

########### install files ###############
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/katesymbolviewer)
install(TARGETS katesymbolviewerplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
install(FILES katesymbolviewerplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
kcoreaddons_desktop_to_json (katesymbolviewerplugin katesymbolviewerplugin.desktop)
