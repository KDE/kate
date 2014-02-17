
include(ECMMarkAsTest)

macro(filetree_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp document_dummy.cpp)
    add_test(kateapp-${_testname} ${_testname})
    target_link_libraries(${_testname} katefiletree Qt5::Test)
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro(filetree_executable_tests)

filetree_executable_tests(
  filetree_model_test
)
