include(ECMMarkAsTest)

add_executable(filetree_model_test "")
target_include_directories(filetree_model_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  filetree_model_test
  PRIVATE
    kateprivate
    KF6::GuiAddons
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)

target_sources(
  filetree_model_test
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../katefiletreemodel.cpp
    filetree_model_test.cpp
    document_dummy.cpp
)

add_test(NAME kateapp-filetree_model_test COMMAND filetree_model_test ${OFFSCREEN_QPA})
ecm_mark_as_test(filetree_model_test)
