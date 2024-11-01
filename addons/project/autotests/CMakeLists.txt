include(ECMMarkAsTest)

add_executable(projectplugin_test "")
target_include_directories(projectplugin_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  projectplugin_test 
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::TextEditor
    Qt::Test
)

target_sources(
  projectplugin_test 
  PRIVATE
    test1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../fileutil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kateprojectcodeanalysistool.cpp
)

if(BUILD_PCH)
    target_precompile_headers(projectplugin_test REUSE_FROM katepch_tests)
endif()

add_test(NAME plugin-project_test COMMAND projectplugin_test ${OFFSCREEN_QPA})
ecm_mark_as_test(projectplugin_test)
