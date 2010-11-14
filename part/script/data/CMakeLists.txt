# install default api
install( FILES 05_cursor.js 10_range.js DESTINATION ${DATA_INSTALL_DIR}/katepart/api )

# install some javascripts
install( FILES utils.js lisp.js lua.js cstyle.js python.js ruby.js lilypond.js haskell.js xml.js DESTINATION ${DATA_INSTALL_DIR}/katepart/script )
