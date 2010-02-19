project(kdeviconinserter)

find_package(KDE4 REQUIRED)

include (KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(${KDE4_INCLUDES})

set(ktexteditor_iconinserter_SRCS
	iconinserterplugin.cpp
)

kde4_add_plugin(ktexteditor_iconinserter ${ktexteditor_iconinserter_SRCS})

target_link_libraries(ktexteditor_iconinserter
	${KDE4_KDEUI_LIBS}
	${KDE4_KTEXTEDITOR_LIBS}
)

install(TARGETS ktexteditor_iconinserter DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES ktexteditor_iconinserter.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES ktexteditor_iconinserterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_iconinserter )
