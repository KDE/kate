add_subdirectory(qtermwidget)

add_library(kateterminal)
install(TARGETS kateterminal ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

include(GenerateExportHeader)
generate_export_header(
  kateterminal
  EXPORT_FILE_NAME kateterm_export.h
  EXPORT_MACRO_NAME KATE_TERMINAL_EXPORT
)

target_sources(kateterminal PRIVATE KateTerminalWidget.cpp)

target_link_libraries(kateterminal PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
    Qt::Widgets
    qtermwidget5
)
