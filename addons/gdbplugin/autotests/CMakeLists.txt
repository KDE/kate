include(ECMMarkAsTest)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

add_executable(gdbplugin_test_gdbmi "")
target_include_directories(gdbplugin_test_gdbmi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
  gdbplugin_test_gdbmi
  PRIVATE
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)
target_sources(
  gdbplugin_test_gdbmi
  PRIVATE
    tst_gdbmiitems.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbmi/tokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbmi/parser.cpp
)
add_test(NAME plugin-gdb_test_gdbmi COMMAND gdbplugin_test_gdbmi ${OFFSCREEN_QPA})
ecm_mark_as_test(gdbplugin_test_gdbmi)


add_executable(gdbplugin_test_gdbbackend "")
target_include_directories(gdbplugin_test_gdbbackend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
  gdbplugin_test_gdbbackend
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)
ki18n_wrap_ui(gdbplugin_test_gdbbackend ${CMAKE_CURRENT_SOURCE_DIR}/../advanced_settings.ui)
target_sources(
  gdbplugin_test_gdbbackend
  PRIVATE
    tst_gdb_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../debugview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../debugview_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbvariableparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../advanced_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../dap/entities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbmi/tokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gdbmi/parser.cpp
)
add_test(NAME plugin-gdb_test_gdbbackend COMMAND gdbplugin_test_gdbbackend ${OFFSCREEN_QPA})
ecm_mark_as_test(gdbplugin_test_gdbbackend)
