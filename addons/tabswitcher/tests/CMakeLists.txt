add_executable(tstestapp "")

target_link_libraries(
  tstestapp 
  PRIVATE KF6::TextEditor kateprivate
)

target_sources(
  tstestapp 
  PRIVATE 
    tstestapp.cpp 
    ../tabswitcherfilesmodel.cpp
)

if(ENABLE_PCH)
    target_precompile_headers(tstestapp REUSE_FROM katepch_tests)
endif()
