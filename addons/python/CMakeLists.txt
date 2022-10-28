#
# Kate Python bindings
#

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

remove_definitions(-DQT_STRICT_ITERATORS)

set(PYTHON_BINDING_NAMESPACE "PyKate")
# Just to fix warnings with --warn-uninitialized
if(NOT DEFINED SHIBOKEN_CUSTOM_PREFIX) #look for shiboken in a custom location
    set(SHIBOKEN_CUSTOM_PREFIX "")
endif()
if(NOT DEFINED PYSIDE_CUSTOM_PREFIX) #look for pyside in a custom location
    set(PYSIDE_CUSTOM_PREFIX "")
endif()

if(${PROJECT_NAME}_QT6)
    set(PYSIDE_MAJOR_VERSION "6")
    set(PYTHON_BINDING_NAMESPACE "${PYTHON_BINDING_NAMESPACE}Qt${PYSIDE_MAJOR_VERSION}")
    set(QtWidgets_VERSION ${Qt6Widgets_VERSION})
else()
    set(PYSIDE_MAJOR_VERSION "2")
    set(QtWidgets_VERSION ${Qt5Widgets_VERSION})
endif()

find_package(Python3 3.7 REQUIRED COMPONENTS Interpreter Development)
find_package(Shiboken${PYSIDE_MAJOR_VERSION} REQUIRED)
find_package(PySide${PYSIDE_MAJOR_VERSION} REQUIRED)

if(NOT PYTHON_BINDINGS_INSTALL_PREFIX)
    set(PYTHON_BINDINGS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif()

set(Python3_VERSION_MAJORMINOR "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set(BINDINGS_DIR "${INSTALL_LIBRARY_DIR}/python${Python3_VERSION_MAJORMINOR}/site-packages/${PYTHON_BINDING_NAMESPACE}")
set(PYTHON_BINDINGS_INSTALL_PREFIX "${PYTHON_BINDINGS_INSTALL_PREFIX}/${BINDINGS_DIR}")

include(PySide${PYSIDE_MAJOR_VERSION}ModuleBuild)
add_subdirectory(PyKate)

#
# Kate Python plugin
#
kate_add_plugin(katepythonplugin)
target_compile_definitions(katepythonplugin PRIVATE TRANSLATION_DOMAIN="katepythonplugin")

target_link_libraries(
  katepythonplugin
  PRIVATE
    KF5::I18n
    KF5::TextEditor
    Shiboken2::libshiboken
    PySide2::pyside2
    ${Python3_LIBRARIES}
    PyKate
)

target_sources(
  katepythonplugin
  PRIVATE
    katepythonplugin.cpp
    pythonutils.cpp
    plugin.qrc
)
