add_executable(lsptestapp "")
target_include_directories(lsptestapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(lsptestapp PRIVATE kateprivate KF6::I18n KF6::TextEditor $<TARGET_OBJECTS:lspclientplugin> katepch)

target_sources(
  lsptestapp
  PRIVATE
    lsptestapp.cpp
)

if(ENABLE_PCH)
    target_precompile_headers(lsptestapp REUSE_FROM katepch_tests)
endif()
