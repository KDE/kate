add_executable(lsptestapp "")
target_include_directories(lsptestapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.. ../rapidjson/include)
target_include_directories(lsptestapp SYSTEM PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../rapidjson/include)
target_link_libraries(lsptestapp PRIVATE kateprivate KF6::TextEditor)

target_sources(
  lsptestapp
  PRIVATE
    lsptestapp.cpp
    ../lspclientserver.cpp
    ../lspsemantichighlighting.cpp
    ../semantic_tokens_legend.cpp
    ${DEBUG_SOURCES}
)
