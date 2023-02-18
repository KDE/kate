add_executable(tstestapp "")

target_link_libraries(
  tstestapp 
  PRIVATE KF${KF_MAJOR_VERSION}::TextEditor kateprivate
)

target_sources(
  tstestapp 
  PRIVATE 
    tstestapp.cpp 
    ../tabswitcherfilesmodel.cpp
)
