cmake_minimum_required(VERSION 3.16)

project(test-blame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(Qt6 REQUIRED COMPONENTS Core Test)
find_package(ECM REQUIRED COMPONENTS I18n)
# We append to the module path so modules can be overridden from the command line.
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(KF6 REQUIRED COMPONENTS I18n)

set(PROJECT_SOURCES
    ../gitblameparser.cpp
    gitblametest.cpp
    testfiles.qrc
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Test KF6::I18n)

enable_testing(true)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
