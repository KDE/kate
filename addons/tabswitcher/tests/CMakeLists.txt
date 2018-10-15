cmake_minimum_required(VERSION 3.0.0)
project(tstestapp)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

set(SRC
  main.cpp
  tstestapp.cpp
  ../tabswitcherfilesmodel.cpp
)

add_executable(tstestapp ${SRC})
target_link_libraries(tstestapp Qt5::Widgets)

