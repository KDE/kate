include(ECMMarkAsTest)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

add_executable(deploymentconfigtest deploymentconfigtest.cpp ../deploymentconfig.cpp)
target_include_directories(deploymentconfigtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(deploymentconfigtest PRIVATE TRANSLATION_DOMAIN="katedeployment")
target_link_libraries(deploymentconfigtest PRIVATE KF6::I18n Qt::Core Qt::Test)

add_test(NAME plugin-deploymentconfigtest COMMAND deploymentconfigtest)
ecm_mark_as_test(deploymentconfigtest)

add_executable(deploymenttransfertest deploymenttransfertest.cpp)
target_link_libraries(deploymenttransfertest PRIVATE KF6::KIOCore Qt::Core Qt::Test)

add_test(NAME plugin-deploymenttransfertest COMMAND deploymenttransfertest)
ecm_mark_as_test(deploymenttransfertest)

if (ENABLE_PCH)
    target_precompile_headers(deploymentconfigtest REUSE_FROM katepch_tests)
    target_precompile_headers(deploymenttransfertest REUSE_FROM katepch_tests)
endif()
