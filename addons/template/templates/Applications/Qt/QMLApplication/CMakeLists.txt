cmake_minimum_required(VERSION 3.16)

project(TMPL_APPLICATION_NAME_TMPL VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)

qt_add_qml_module(${PROJECT_NAME}
    URI Module${PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
