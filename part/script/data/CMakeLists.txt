# install default api
install( FILES 00_katepart.js DESTINATION ${DATA_INSTALL_DIR}/katepart/api )

# install some javascripts
install( FILES jstest.js utils.js lisp.js cstyle.js python.js ruby.js lilypond.js haskell.js DESTINATION ${DATA_INSTALL_DIR}/katepart/script )
