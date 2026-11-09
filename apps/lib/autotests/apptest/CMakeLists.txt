# utilities for whole app tests, see kateapptest.h and KateAppTest.cmake
add_library(kateapptest STATIC kateapptest.cpp)
target_include_directories(kateapptest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  kateapptest
  PUBLIC
    kateprivate
    Qt::Test
  PRIVATE
    KF6::CoreAddons
    KF6::XmlGui
)
