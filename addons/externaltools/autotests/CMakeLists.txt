include(ECMMarkAsTest)

# Project Plugin
add_executable(externaltools_test
    externaltooltest.cpp
    ../kateexternaltool.cpp
    ../katetoolrunner.cpp
)
add_test(plugin-externaltools_test externaltools_test)
target_link_libraries(externaltools_test kdeinit_kate Qt5::Test)
ecm_mark_as_test(externaltools_test)
