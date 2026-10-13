include(ECMMarkAsTest)

add_executable(test_importdata "")
target_include_directories(test_importdata PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  test_importdata
  PRIVATE
    Qt::Test
    Qt::Sql
    KF6::I18n
    KF6::TextEditor
    KF6::KIOWidgets
    KF6::WindowSystem
    qt6keychain
    $<TARGET_OBJECTS:katesqlplugin>
)

target_sources(
  test_importdata
  PRIVATE
    test_importdata.cpp
)

add_test(NAME katesql-test_importdata COMMAND test_importdata)
ecm_mark_as_test(test_importdata)

if(ENABLE_PCH)
  # plugin objects were compiled with katepch, MSVC needs its PCH object
  target_link_libraries(test_importdata PRIVATE katepch)
endif()

# ── Export data tests ─────────────────────────────────────────────────────────

add_executable(test_exportdata "")
target_include_directories(test_exportdata PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
  test_exportdata
  PRIVATE
    Qt::Test
    Qt::Gui
    Qt::Sql
    KF6::I18n
    KF6::TextEditor
    KF6::KIOWidgets
    KF6::WindowSystem
    qt6keychain
    $<TARGET_OBJECTS:katesqlplugin>
)

target_sources(
  test_exportdata
  PRIVATE
    test_exportdata.cpp
)

add_test(NAME katesql-test_exportdata COMMAND test_exportdata)
ecm_mark_as_test(test_exportdata)

if(ENABLE_PCH)
  # plugin objects were compiled with katepch, MSVC needs its PCH object
  target_link_libraries(test_exportdata PRIVATE katepch)
endif()
