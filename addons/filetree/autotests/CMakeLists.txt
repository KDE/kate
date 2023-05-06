include(ECMMarkAsTest)

add_executable(filetree_model_test "")
target_include_directories(filetree_model_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  filetree_model_test
  PRIVATE
    kateprivate
    KF${KF_MAJOR_VERSION}::GuiAddons
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
    Qt::Test
)

target_sources(
  filetree_model_test
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../katefiletreemodel.cpp
    filetree_model_test.cpp
    document_dummy.cpp
)

add_test(NAME kateapp-filetree_model_test COMMAND filetree_model_test -platform offscreen)
ecm_mark_as_test(filetree_model_test)
