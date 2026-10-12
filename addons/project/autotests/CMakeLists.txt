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
    KF6::GuiAddons
    KF6::WidgetsAddons
    Qt::Concurrent
    Qt::Network
    Qt::Test
    $<TARGET_OBJECTS:kateprojectplugin>
)

target_sources(
  projectplugin_test
  PRIVATE
    test1.cpp
)

if(ENABLE_PCH)
    # plugin objects were compiled with katepch, MSVC needs its PCH object
    target_link_libraries(projectplugin_test PRIVATE katepch)
    target_precompile_headers(projectplugin_test REUSE_FROM katepch_tests)
endif()

add_test(NAME plugin-project_test COMMAND projectplugin_test ${OFFSCREEN_QPA})
ecm_mark_as_test(projectplugin_test)

add_executable(gitforgeurltest gitforgeurltest.cpp)
target_include_directories(gitforgeurltest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
  gitforgeurltest
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::TextEditor
    KF6::GuiAddons
    KF6::WidgetsAddons
    Qt::Concurrent
    Qt::Network
    Qt::Test
    $<TARGET_OBJECTS:kateprojectplugin>
)

add_test(NAME plugin-gitforgeurltest COMMAND gitforgeurltest)
ecm_mark_as_test(gitforgeurltest)

if(ENABLE_PCH)
    # plugin objects were compiled with katepch, MSVC needs its PCH object
    target_link_libraries(gitforgeurltest PRIVATE katepch)
    target_precompile_headers(gitforgeurltest REUSE_FROM katepch_tests)
endif()
