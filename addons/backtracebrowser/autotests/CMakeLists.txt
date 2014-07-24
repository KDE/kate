include(ECMMarkAsTest)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Plugin Kate Backtrace Browser
set(BtBrowserSrc btbrowsertest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../btparser.cpp)
add_executable(btbrowser_test ${BtBrowserSrc})
add_test(plugin-btbrowser_test btbrowser_test)
target_link_libraries(btbrowser_test kdeinit_kate Qt5::Test)
ecm_mark_as_test(btbrowser_test)
