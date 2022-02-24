add_library(kateshared OBJECT
    quickdialog.cpp
)

target_include_directories(kateshared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(
  KF5 ${KF5_DEP_VERSION}
  QUIET
  REQUIRED
  COMPONENTS
    TextEditor
    DBusAddons
    Crash
)

target_link_libraries(
  kateshared
  PUBLIC
    KF5::I18n
    KF5::TextEditor
    KF5::DBusAddons
    KF5::Crash
)
