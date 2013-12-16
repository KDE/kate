ecm_setup_version(5.0.0 VARIABLE_PREFIX KTEXTEDITOR
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfigVersion.cmake")

# KTextEditor interface header
set(KTEXTEDITOR_PUBLIC_HEADERS
    ktexteditor_export.h
    factory.h
    editor.h
    document.h
    view.h
    plugin.h
    cursor.h
    range.h
    documentcursor.h
    configpage.h
    searchinterface.h
    markinterface.h
    sessionconfiginterface.h
    texthintinterface.h
    variableinterface.h
    templateinterface.h
    templateinterface2.h
    modificationinterface.h
    commandinterface.h
    attribute.h
    codecompletioninterface.h
    codecompletionmodel.h
    codecompletionmodelcontrollerinterface.h
    configinterface.h
    containerinterface.h
    annotationinterface.h
    highlightinterface.h
    modeinterface.h
    movinginterface.h
    movingcursor.h
    movingrange.h
    movingrangefeedback.h
    recoveryinterface.h
    messageinterface.h
    )

# KTextEditor interface sources
set(ktexteditor_LIB_SRCS
   ktexteditor.cpp
   document.cpp
   range.cpp
   documentcursor.cpp
   templateinterface.cpp
   templateinterface2.cpp
   attribute.cpp
   codecompletioninterface.cpp
   codecompletionmodel.cpp
   codecompletionmodelcontrollerinterface.cpp
   configinterface.cpp
   highlightinterface.cpp
   movinginterface.cpp
   movingcursor.cpp
   movingrange.cpp
   movingrangefeedback.cpp
   messageinterface.cpp
   
   # private headers for moc
   documentadaptor_p.h
   )

add_library (ktexteditor SHARED ${ktexteditor_LIB_SRCS} ${KTEXTEDITOR_PUBLIC_HEADERS})

target_link_libraries(ktexteditor LINK_PUBLIC KF5::Parts
                                  LINK_PRIVATE KF5::I18n KF5::KDE4Support)
target_include_directories(ktexteditor INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>")

set_target_properties(ktexteditor
   PROPERTIES VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)

install(TARGETS ktexteditor EXPORT KTextEditor ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install( FILES ${KTEXTEDITOR_PUBLIC_HEADERS}
    DESTINATION  ${INCLUDE_INSTALL_DIR}/ktexteditor COMPONENT Devel)

install( FILES ktexteditor.desktop ktexteditorplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

########### export targets ##############
# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KTextEditor")

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KTextEditorConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KTextEditor DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KTextEditorTargets.cmake NAMESPACE KTextEditor:: )

install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h
  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)
