find_package(KF6 ${KF_MIN_VERSION} COMPONENTS
    Archive
    Config
)

kate_add_plugin(templateplugin)
target_compile_definitions(templateplugin PRIVATE TRANSLATION_DOMAIN="templateplugin")
if ((TARGET KF6::Archive) AND (TARGET KF6::ConfigGui))
    target_link_libraries(templateplugin PRIVATE KF6::I18n KF6::TextEditor KF6::Archive KF6::ConfigGui)
else ()
    target_link_libraries(templateplugin PRIVATE KF6::I18n KF6::TextEditor)
endif()

ki18n_wrap_ui(templateplugin template.ui importcategory.ui)

set(PROJECT_SOURCES
    template.cpp
    template.h
    template.ui
    abstractdatamodel.cpp
    abstractdatamodel.h
    importcategory.cpp
    importcategory.h
    plugin.qrc
    templates.qrc
)
if ((TARGET KF6::Archive) AND (TARGET KF6::ConfigGui))
    message(STATUS "Archive & ConfigGui found")
    set(PROJECT_SOURCES ${PROJECT_SOURCES} AppWizardReader.cpp)
    add_definitions(-DBUILD_APPWIZARD)
endif()

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
    DEBUG_SOURCES
    HEADER templateplugin_debug.h
    IDENTIFIER KTEBUILD
    CATEGORY_NAME "templateplugin"
)

target_sources(templateplugin  PRIVATE templateplugin.h templateplugin.cpp ${PROJECT_SOURCES} ${DEBUG_SOURCES})

# qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} main.cpp)
# target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

if (ENABLE_PCH)
    target_precompile_headers(templateplugin REUSE_FROM katepch)
endif()
