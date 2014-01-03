# KTextEditor interfaces
project (ktexteditor)

# ecm setup, will create version header & cmake file
ecm_setup_version (${KATE_VERSION} VARIABLE_PREFIX KTEXTEDITOR
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/ktexteditor/ktexteditor_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/data/KTextEditorConfigVersion.cmake")

# install version header
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ktexteditor/ktexteditor_version.h DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)

# handle include files, both normal ones and generated ones
add_subdirectory (include)

# add our ktexteditor includes to our search path
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include/ktexteditor)

# KTextEditor interface sources
set(ktexteditor_LIB_SRCS
   src/ktexteditor.cpp
   src/document.cpp
   src/range.cpp
   src/documentcursor.cpp
   src/templateinterface.cpp
   src/templateinterface2.cpp
   src/attribute.cpp
   src/codecompletioninterface.cpp
   src/codecompletionmodel.cpp
   src/codecompletionmodelcontrollerinterface.cpp
   src/configinterface.cpp
   src/configpageinterface.cpp
   src/highlightinterface.cpp
   src/movinginterface.cpp
   src/movingcursor.cpp
   src/movingrange.cpp
   src/movingrangefeedback.cpp
   src/messageinterface.cpp

   # application interfaces
   src/application.cpp
   src/mainwindow.cpp

   # private headers for moc
   src/documentadaptor_p.h
   )

add_library (ktexteditor SHARED ${ktexteditor_LIB_SRCS} ${KTEXTEDITOR_PUBLIC_HEADERS})

target_link_libraries(ktexteditor LINK_PUBLIC KF5::Parts
                                  LINK_PRIVATE KF5::I18n)
target_include_directories(ktexteditor INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/KTextEditor>")

set_target_properties(ktexteditor
   PROPERTIES VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)

# generate and install export header file
generate_export_header (ktexteditor EXPORT_FILE_NAME "include/KTextEditor/ktexteditor/ktexteditor_export.h")
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/include/KTextEditor/ktexteditor/ktexteditor_export.h DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)

install(TARGETS ktexteditor EXPORT KTextEditor ${INSTALL_TARGETS_DEFAULT_ARGS})

# handle data files, .desktop & .cmake
add_subdirectory (data)
