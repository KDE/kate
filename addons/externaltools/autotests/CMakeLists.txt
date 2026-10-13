include(ECMMarkAsTest)

# Project Plugin
add_executable(externaltools_test
    externaltooltest.cpp
)
add_test(NAME plugin-externaltools_test COMMAND externaltools_test ${OFFSCREEN_QPA})
target_link_libraries(externaltools_test PRIVATE kateprivate Qt::Test KF6::I18n KF6::ConfigCore KF6::CoreAddons KF6::IconThemes KF6::IconWidgets KF6::TextEditor $<TARGET_OBJECTS:externaltoolsplugin>)
ecm_mark_as_test(externaltools_test)

if(ENABLE_PCH)
    # plugin objects were compiled with katepch, MSVC needs its PCH object
    target_link_libraries(externaltools_test PRIVATE katepch)
    target_precompile_headers(externaltools_test REUSE_FROM katepch_tests)
endif()
