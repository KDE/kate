include(ECMMarkAsTest)

add_executable(test_formatapply "")
target_include_directories(test_formatapply PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  test_formatapply
  PRIVATE
    kateprivate
    KF${KF_MAJOR_VERSION}::TextEditor
    Qt::Test
)

target_sources(
  test_formatapply
  PRIVATE
    test_formatapply.cpp
)

add_test(NAME kateapp-test_formatapply COMMAND test_formatapply)
ecm_mark_as_test(test_formatapply)
