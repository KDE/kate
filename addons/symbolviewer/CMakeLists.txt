add_definitions(-DTRANSLATION_DOMAIN=\"katesymbolviewer\")
########### next target ###############
set(katesymbolviewerplugin_PART_SRCS cpp_parser.cpp tcl_parser.cpp fortran_parser.cpp perl_parser.cpp 
php_parser.cpp xslt_parser.cpp ruby_parser.cpp python_parser.cpp bash_parser.cpp ecma_parser.cpp plugin_katesymbolviewer.cpp )

# resource for ui file and stuff
qt5_add_resources(katesymbolviewerplugin_PART_SRCS plugin.qrc)

add_library(katesymbolviewerplugin MODULE ${katesymbolviewerplugin_PART_SRCS})

kcoreaddons_desktop_to_json (katesymbolviewerplugin katesymbolviewerplugin.desktop)

target_link_libraries(katesymbolviewerplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

install(TARGETS katesymbolviewerplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
