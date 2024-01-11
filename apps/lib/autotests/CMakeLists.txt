include(ECMMarkAsTest)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

# add given tests to run in offscreen rendering
macro(kate_executable_tests)
    foreach(_testname ${ARGN})
        add_executable(${_testname} ${_testname}.cpp tests.qrc)
        target_link_libraries(${_testname}
            PRIVATE
                kateprivate Qt::Test)
        add_test(NAME kateapp-${_testname} COMMAND ${_testname} ${OFFSCREEN_QPA})
        ecm_mark_as_test(${_testname})
    endforeach()
endmacro()

# some test ATM timeout with Qt 6 in the CI
if (WIN32)
    kate_executable_tests(
        session_test
        # session_manager_test
        # sessions_action_test
        urlinfo_test
        json_utils_test
        kate_doc_manager_tests
        # location_history_test
        # kate_view_mgmt_tests
        bytearraysplitter_tests
        diffwidget_tests)
else()
    kate_executable_tests(
        session_test
        session_manager_test
        sessions_action_test
        urlinfo_test
        json_utils_test
        kate_doc_manager_tests
        location_history_test
        kate_view_mgmt_tests
        bytearraysplitter_tests
        diffwidget_tests)
endif()
