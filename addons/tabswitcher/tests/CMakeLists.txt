add_executable(tstestapp "")

target_link_libraries(
  tstestapp 
  PRIVATE KF6::TextEditor KF6::I18n kateprivate $<TARGET_OBJECTS:tabswitcherplugin>
)

target_sources(
  tstestapp 
  PRIVATE 
    tstestapp.cpp
)

if(ENABLE_PCH)
    # plugin objects were compiled with katepch, MSVC needs its PCH object
    target_link_libraries(tstestapp PRIVATE katepch)
    target_precompile_headers(tstestapp REUSE_FROM katepch_tests)
endif()
