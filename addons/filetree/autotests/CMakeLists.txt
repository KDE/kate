include(ECMMarkAsTest)

add_executable(filetree_model_test "")

target_link_libraries(
  filetree_model_test 
  PRIVATE
    katefiletree 
    Qt5::Test
)

target_sources(
  filetree_model_test 
  PRIVATE
    filetree_model_test.cpp 
    document_dummy.cpp
)

add_test(NAME kateapp-filetree_model_test COMMAND filetree_model_test)
ecm_mark_as_test(filetree_model_test)
