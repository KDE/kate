add_definitions(-DTRANSLATION_DOMAIN=\"ktexteditorpreviewplugin\")

include(ECMQtDeclareLoggingCategory)

set(ktexteditorpreviewplugin_SRCS
    ktexteditorpreviewplugin.cpp
    ktexteditorpreviewview.cpp
    previewwidget.cpp
    kpartview.cpp
)

ecm_qt_declare_logging_category(ktexteditorpreviewplugin_SRCS
    HEADER ktexteditorpreview_debug.h
    IDENTIFIER KTEPREVIEW
    CATEGORY_NAME "ktexteditorpreviewplugin"
)

add_library(ktexteditorpreviewplugin MODULE ${ktexteditorpreviewplugin_SRCS})

target_link_libraries(ktexteditorpreviewplugin
    KF5::TextEditor
    KF5::I18n
)

install(TARGETS ktexteditorpreviewplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
