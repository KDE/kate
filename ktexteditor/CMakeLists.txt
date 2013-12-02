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
    loadsavefiltercheckplugin.h
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

target_link_libraries(ktexteditor
    KF5::KParts
    KF5::KDE4Support
)

set_target_properties(ktexteditor
   PROPERTIES VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)

install(TARGETS ktexteditor EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install( FILES ${KTEXTEDITOR_PUBLIC_HEADERS}
    DESTINATION  ${INCLUDE_INSTALL_DIR}/ktexteditor COMPONENT Devel)

install( FILES ktexteditor.desktop ktexteditorplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

install( FILES ktexteditor_loadsavefiltercheckplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
