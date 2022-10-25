# Auto-Genereate files every class will have his cpp/h files
set(PyKate_SRC
    # individual classes
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_application_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_application_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_attribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_attribute_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_command_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_command_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_configpage_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_configpage_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_cursor_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_cursor_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_document_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_document_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_editor_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_editor_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_linerange_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_linerange_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_mainwindow_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_mainwindow_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/kate_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_range_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_range_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_view_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_view_wrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_configinterface_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Kate/ktexteditor_configinterface_wrapper.h
)

# includes necessary to parse and build the classes specified on typesystem
set(PyKate_include_paths
    $<JOIN:$<TARGET_PROPERTY:kateprivate,INTERFACE_INCLUDE_DIRECTORIES>,${PATH_SEP}>
    ${CMAKE_SOURCE_DIR}/src/include
)

# A list of paths where shiboken should look for typesystem
set(PyKate_typesystem_paths # PySide path, this variable was exposed by FindPySide2.cmake
    ${PYSIDE_TYPESYSTEMS}
)

# Include flags/path that will be set in 'target_include_directories'
# set(PyKate_target_include_directories ${CMAKE_SOURCE_DIR}/src/include)

# Libraries that will be necessary to link the target, this will used in the command 'target_link_libraries'
set(PyKate_target_link_libraries
    kateprivate
    ${Python3_LIBRARIES}
)

# changes on these files should trigger a new generation
set(PyKate_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/kate_python.h
)

create_python_bindings(
    "Kate"
    "${PyKate_typesystem_paths}"
    "${PyKate_include_paths}"
    "${PyKate_SRC}"
    "${PyKate_target_include_directories}"
    "${PyKate_target_link_libraries}"
    ${CMAKE_CURRENT_SOURCE_DIR}/kate_python.h
    ${CMAKE_CURRENT_SOURCE_DIR}/typesystem_kate.xml
    "${PyKate_DEPENDS}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Make module import from build dir work
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.cmake ${CMAKE_CURRENT_BINARY_DIR}/__init__.py @ONLY)

# install
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/__init__.py $<TARGET_FILE:PyKate>
#         DESTINATION ${PYTHON_BINDINGS_INSTALL_PREFIX}
# )
