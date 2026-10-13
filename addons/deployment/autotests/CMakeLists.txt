include(ECMMarkAsTest)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

add_executable(deploymentconfigtest deploymentconfigtest.cpp)
target_include_directories(deploymentconfigtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
  deploymentconfigtest
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::KIOFileWidgets
    KF6::KIOWidgets
    KF6::TextEditor
    KF6::WidgetsAddons
    Qt::Test
    $<TARGET_OBJECTS:katedeploymentplugin>
)

add_test(NAME plugin-deploymentconfigtest COMMAND deploymentconfigtest)
ecm_mark_as_test(deploymentconfigtest)

add_executable(deploymenttransfertest deploymenttransfertest.cpp)
target_link_libraries(deploymenttransfertest PRIVATE KF6::KIOCore Qt::Core Qt::Test)

add_test(NAME plugin-deploymenttransfertest COMMAND deploymenttransfertest)
ecm_mark_as_test(deploymenttransfertest)

if (ENABLE_PCH)
    # plugin objects were compiled with katepch, MSVC needs its PCH object
    target_link_libraries(deploymentconfigtest PRIVATE katepch)
    target_precompile_headers(deploymentconfigtest REUSE_FROM katepch_tests)
    target_precompile_headers(deploymenttransfertest REUSE_FROM katepch_tests)
endif()
