include(ECMMarkAsTest)

add_executable(gdbplugin_test "")
target_include_directories(gdbplugin_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  gdbplugin_test
  PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
    Qt::Test
)

target_sources(
  gdbplugin_test
  PRIVATE
    tst_gdbmiitems.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbmi/tokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbmi/parser.cpp
)

add_test(NAME plugin-gdb_test COMMAND gdbplugin_test)
ecm_mark_as_test(gdbplugin_test)
