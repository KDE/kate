kate_add_app_test(SearchPluginTest
  SOURCES SearchPluginTest.cpp
  PLUGINS katesearchplugin
  LIBRARIES KF6::TextEditor
)
# for the MatchModel enums
target_include_directories(SearchPluginTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
