include(ECMMarkAsTest)

add_executable(test_rainbowparens "")
target_include_directories(test_rainbowparens PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  test_rainbowparens
  PRIVATE
    KF6::TextEditor
    KF6::I18n
    Qt::Test
     $<TARGET_OBJECTS:rainbowparens>
)

target_sources(
  test_rainbowparens
  PRIVATE
    test.cpp
)

if(ENABLE_PCH)
    target_precompile_headers(test_rainbowparens REUSE_FROM katepch_tests)
endif()

add_test(NAME kateapp-test_rainbowparens COMMAND test_rainbowparens ${OFFSCREEN_QPA})
ecm_mark_as_test(test_rainbowparens)
