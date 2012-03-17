# Copyright 2011 by Alex Turbov <i.zaufi@gmail.com>
#
# Integrate Boost unit tests into cmake infrastructure
#
# Usage:
#  add_boost_tests(EXECUTABLE EXTRA_ARGS SOURCES)
#   EXECUTABLE -- name of unit tests binary (unit_tests usualy)
#   EXTRA_ARGS -- aux args to be passed to Boost unit test binary
#   SOURCES    -- list of source files
#
# Source files will be canned for automatic test cases.
# Every found test will be added to ctest list.
#

function (add_boost_tests EXECUTABLE EXTRA_ARGS)
  # Prevent running w/o source files
  if (NOT ARGN)
    message(FATAL_ERROR "No source files given to `add_boost_tests'")
  endif()

  # Scan source files for BOOST_AUTO_TEST_CASE macros and add test by found name
  foreach(source ${ARGN})
    file(READ "${source}" contents)
    # Append BOOST_AUTO_TEST_CASEs
    string(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\(([A-Za-z_0-9]+)\\)" found_tests ${contents})
    foreach(hit ${found_tests})
        string(REGEX REPLACE ".*\\(([A-Za-z_0-9]+)\\).*" "\\1" test_name ${hit})
        add_test(${test_name} ${EXECUTABLE} --run_test=${test_name} ${EXTRA_ARGS})
    endforeach()
    # Append BOOST_FIXTURE_TEST_CASEs
    string(REGEX MATCHALL "BOOST_FIXTURE_TEST_SUITE\\(([A-Za-z_0-9]+), ([A-Za-z_0-9]+)\\)" found_tests ${contents})
    foreach(hit ${found_tests})
        string(REGEX REPLACE ".*\\(([A-Za-z_0-9]+), ([A-Za-z_0-9]+)\\).*" "\\1" test_name ${hit})
        add_test(${test_name} ${EXECUTABLE} --run_test=${test_name} ${EXTRA_ARGS})
    endforeach()
  endforeach()
endfunction(add_boost_tests)
