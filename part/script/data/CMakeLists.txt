# install default lib files
file (GLOB KATE_JS_LIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.js")
install (FILES ${KATE_JS_LIB_FILES} DESTINATION ${DATA_INSTALL_DIR}/katepart/script/lib)

# install default commands files
file (GLOB KATE_JS_COMMANDS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/commands/*.js")
install (FILES ${KATE_JS_COMMANDS_FILES} DESTINATION ${DATA_INSTALL_DIR}/katepart/script/commands)

# install default indentation files
file (GLOB KATE_JS_INDENTATION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/indentation/*.js")
install (FILES ${KATE_JS_INDENTATION_FILES} DESTINATION ${DATA_INSTALL_DIR}/katepart/script/indentation)
