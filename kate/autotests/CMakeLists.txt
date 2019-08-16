include(ECMMarkAsTest)

find_package(Qt5Test QUIET REQUIRED)

macro(kate_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    
    target_link_libraries(
      ${_testname} 
      PRIVATE
        kdeinit_kate
        Qt5::Test
    )

    add_test(NAME kateapp-${_testname} COMMAND ${_testname})
    ecm_mark_as_test(${_testname})
  endforeach()
endmacro()

kate_executable_tests(
  session_test
  session_manager_test
  sessions_action_test
)
