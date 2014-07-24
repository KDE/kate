
include(ECMMarkAsTest)

include_directories(
  ${CMAKE_SOURCE_DIR}/kate/src
  ${CMAKE_SOURCE_DIR}/kate/src/session
)

macro(kate_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    add_test(kateapp-${_testname} ${_testname})
    target_link_libraries(${_testname} kdeinit_kate Qt5::Test)
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro(kate_executable_tests)

kate_executable_tests(
  session_test
  session_manager_test
  sessions_action_test
)
