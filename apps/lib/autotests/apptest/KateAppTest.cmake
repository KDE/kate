# kate_add_app_test(<name> SOURCES <sources>... [PLUGINS <plugin targets>...] [LIBRARIES <libraries>...])
#
# Adds a whole app test using the kateapptest library (apps/lib/autotests/apptest/kateapptest.h).
# The test depends on the given plugin targets, so they are built first, and it loads them from
# the build directory instead of an installed Kate. It gets the CTest label "apptest".
#
#   kate_add_app_test(test_search_app
#       SOURCES test_search_app.cpp
#       PLUGINS katesearchplugin
#   )
include(ECMMarkAsTest)

function(kate_add_app_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;PLUGINS;LIBRARIES" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE kateapptest ${ARG_LIBRARIES})
    ecm_mark_as_test(${name})

    if(ARG_PLUGINS)
        # loaded at runtime, not linked, but must be built for the test
        add_dependencies(${name} ${ARG_PLUGINS})
    endif()

    add_test(NAME kateapp-${name} COMMAND ${name} ${OFFSCREEN_QPA})
    # Qt searches QT_PLUGIN_PATH first, so the plugins of the build directory win over installed ones
    set_tests_properties(kateapp-${name} PROPERTIES
        ENVIRONMENT "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/bin;KATE_APP_TEST_PLUGIN_DIR=${CMAKE_BINARY_DIR}/bin/kf6/ktexteditor"
        LABELS apptest
    )

    if(ENABLE_PCH)
        target_precompile_headers(${name} REUSE_FROM katepch_tests)
    endif()
endfunction()
