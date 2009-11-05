
project(ktexteditor_codesnippets_core)

########### next target ###############
set(ktexteditor_codesnippets_core_LIB_SRCS
 completionmodel.cpp
 repository.cpp
 dbus_helpers.cpp
)

set(ktexteditor_codesnippets_core_LIB_UI
  snippet_repository.ui

)


kde4_add_ui_files(ktexteditor_codesnippets_core_LIB_SRCS ${ktexteditor_codesnippets_core_LIB_UI} )

kde4_add_library(ktexteditor_codesnippets_core SHARED ${ktexteditor_codesnippets_core_LIB_SRCS})

target_link_libraries(ktexteditor_codesnippets_core ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS}  ${KDE4_KNEWSTUFF2_LIBS})


target_link_libraries(ktexteditor_codesnippets_core LINK_INTERFACE_LIBRARIES ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

set_target_properties(ktexteditor_codesnippets_core PROPERTIES VERSION "0.0.1" 
                                       SOVERSION "0" 
                       )

install(TARGETS ktexteditor_codesnippets_core EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
########### install files ###############

install( FILES
 ktexteditor_codesnippets_core_export.h
 repository.h
 completionmodel.h
 DESTINATION ${INCLUDE_INSTALL_DIR}/ktexteditor_codesnippets_core COMPONENT Devel )

install( FILES ktexteditor_codesnippets_core.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )