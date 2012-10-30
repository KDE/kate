# install default lib files
file (GLOB KATE_JS_LIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.js")
install (FILES ${KATE_JS_LIB_FILES} DESTINATION ${DATA_INSTALL_DIR}/katepart/script/lib)

# install default bin files
file (GLOB KATE_JS_BIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/bin/*.js")
install (FILES ${KATE_JS_BIN_FILES} DESTINATION ${DATA_INSTALL_DIR}/katepart/script/bin)
