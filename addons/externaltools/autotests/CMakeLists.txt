include(ECMMarkAsTest)

# Project Plugin
add_executable(externaltools_test
    externaltooltest.cpp
    ../kateexternaltool.cpp
    ../katetoolrunner.cpp
)
add_test(NAME plugin-externaltools_test COMMAND externaltools_test -platform offscreen)
target_link_libraries(externaltools_test PRIVATE kateprivate Qt::Test KF6::I18n KF6::ConfigCore KF6::CoreAddons KF6::TextEditor)
ecm_mark_as_test(externaltools_test)
