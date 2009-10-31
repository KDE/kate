

########### next target ###############

add_subdirectory(lib)

set(katesnippets_tngplugin_PART_SRCS jowennsnippets.cpp selector.cpp)

set(katesnippets_tngplugin_PART_UI
  snippet_selector.ui
)
kde4_add_ui_files(katesnippets_tngplugin_PART_SRCS ${katesnippets_tngplugin_PART_UI} )


kde4_add_plugin(katesnippets_tngplugin ${katesnippets_tngplugin_PART_SRCS})


target_link_libraries(katesnippets_tngplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces ktexteditor_codesnippets_core)

install(TARGETS katesnippets_tngplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesnippets_tng )

file(GLOB SNIPPETS "${CMAKE_CURRENT_SOURCE_DIR}/data/*.xml")
install( FILES ${SNIPPETS} DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesnippets_tng/data )

install( FILES katesnippets_tngplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.30")
find_package(SharedMimeInfo)
macro_log_feature(SHARED_MIME_INFO_FOUND "shared-mime-info" "The core MIME database used to represent types of files" "http://freedesktop.org/wiki/Software/shared-mime-info" TRUE "${SHARED_MIME_INFO_MINIMUM_VERSION}" "Required by KDE applications to help determine file types")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mimetypes/kateplugin_katesnippets_tng.xml" DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

add_subdirectory( editor )