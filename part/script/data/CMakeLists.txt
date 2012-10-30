# install default libraries files
install (DIRECTORY libraries/ DESTINATION ${DATA_INSTALL_DIR}/katepart/script/libraries
         FILES_MATCHING PATTERN "*.js")

# install default commands files
install (DIRECTORY commands/ DESTINATION ${DATA_INSTALL_DIR}/katepart/script/commands
         FILES_MATCHING PATTERN "*.js")

# install default indentation files
install (DIRECTORY indentation/ DESTINATION ${DATA_INSTALL_DIR}/katepart/script/indentation
         FILES_MATCHING PATTERN "*.js")
