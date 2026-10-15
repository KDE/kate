include(ECMMarkAsTest)

add_executable(test_formatapply "")
target_include_directories(test_formatapply PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(test_formatapply PRIVATE TRANSLATION_DOMAIN="formatplugin")

# fake formatter process for the tests, a binary to not depend on a shell
add_executable(fake_formatter fake_formatter.cpp)
target_link_libraries(fake_formatter PRIVATE Qt6::Core)
add_dependencies(test_formatapply fake_formatter)
target_compile_definitions(test_formatapply PRIVATE FAKE_FORMATTER="$<TARGET_FILE:fake_formatter>")

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  test_formatapply
  PRIVATE
    kateprivate
    KF6::TextEditor
    Qt::Test
)

target_sources(
  test_formatapply
  PRIVATE
    test_formatapply.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Formatters.cpp
)

if(ENABLE_PCH)
    target_precompile_headers(test_formatapply REUSE_FROM katepch_tests)
endif()

add_test(NAME kateapp-test_formatapply COMMAND test_formatapply ${OFFSCREEN_QPA})
ecm_mark_as_test(test_formatapply)
