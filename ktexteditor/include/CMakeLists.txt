# KTextEditor interface headers
set (KTEXTEDITOR_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/document.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/view.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/plugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/cursor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/range.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/documentcursor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/configpage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/configpageinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/searchinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/markinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/sessionconfiginterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/texthintinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/variableinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/templateinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/templateinterface2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/modificationinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/commandinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/attribute.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/codecompletioninterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/codecompletionmodel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/codecompletionmodelcontrollerinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/configinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/annotationinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/highlightinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/modeinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/movinginterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/movingcursor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/movingrange.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/movingrangefeedback.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/recoveryinterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/messageinterface.h
    
    # application interfaces (including plugin interface for application side plugins)
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/mainwindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ktexteditor/applicationplugin.h
    )

# export headers to parent scope
set (KTEXTEDITOR_PUBLIC_HEADERS ${KTEXTEDITOR_PUBLIC_HEADERS}
    PARENT_SCOPE)
    
# install the public header files
install (FILES ${KTEXTEDITOR_PUBLIC_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)

# generate CamelCase headers
ecm_generate_headers(AnnotationInterface CodeCompletionModelControllerInterface HighlightInterface MovingCursor Range TextHintInterface
  Cursor MarkInterface MovingInterface RecoveryInterface VariableInterface
  Document  MovingRange SearchInterface View
  Attribute CommandInterface DocumentCursor MessageInterface MovingRangeFeedback SessionConfigInterface
  CodeCompletionInterface ConfigInterface Editor ModeInterface  TemplateInterface
  CodeCompletionModel ConfigPage ConfigPageInterface ModificationInterface Plugin TemplateInterface2
  Application MainWindow ApplicationPlugin
  MODULE_NAME "KTextEditor" PREFIX "KTextEditor" RELATIVE "ktexteditor" REQUIRED_HEADERS KTEXTEDITOR_PUBLIC_HEADERS)
   
# missing files ATM:   
# AnnotationModel Command AnnotationViewInterface CommandExtension Message ParameterizedSessionConfigInterface

# install CamelCase headers
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/KTextEditor/KTextEditor DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor COMPONENT Devel)
