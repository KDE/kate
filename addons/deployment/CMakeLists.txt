kate_add_plugin(katedeploymentplugin)
target_compile_definitions(katedeploymentplugin PRIVATE TRANSLATION_DOMAIN="katedeployment")

target_link_libraries(
    katedeploymentplugin
    PRIVATE
        kateprivate
        KF6::I18n
        KF6::KIOFileWidgets
        KF6::KIOWidgets
        KF6::TextEditor
        KF6::WidgetsAddons
)

target_sources(
    katedeploymentplugin
    PRIVATE
        deploymentconfig.h
        deploymentconfig.cpp
        deploymentplugin.h
        deploymentplugin.cpp
        plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(katedeploymentplugin REUSE_FROM katepch)
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
