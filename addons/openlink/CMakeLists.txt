kate_add_plugin(openlinkplugin)
target_compile_definitions(openlinkplugin
                           PRIVATE TRANSLATION_DOMAIN="openlinkplugin")
target_link_libraries(openlinkplugin PRIVATE KF6::TextEditor KF6::I18n)

target_sources(openlinkplugin PRIVATE OpenLinkPlugin.cpp)

if(ENABLE_PCH)
  target_precompile_headers(openlinkplugin REUSE_FROM katepch)
endif()

if (BUILD_TESTING)
    include(ECMMarkAsTest)

    find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
    add_executable(linkmatchtest linkmatchtest.cpp)
    add_test(NAME linkmatchtest COMMAND linkmatchtest ${OFFSCREEN_QPA})
    target_link_libraries(linkmatchtest PRIVATE kateprivate Qt::Test Qt::Core)
    ecm_mark_as_test(linkmatchtest)

    if(ENABLE_PCH)
        target_precompile_headers(linkmatchtest REUSE_FROM katepch_tests)
    endif()
endif()
