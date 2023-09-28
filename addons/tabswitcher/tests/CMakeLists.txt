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
