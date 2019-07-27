include(ECMQtDeclareLoggingCategory)

set(ctagsplugin_SRC
  readtags.c
  tags.cpp
  ctagskinds.cpp
  kate_ctags_view.cpp
  kate_ctags_plugin.cpp
  plugin.qrc
)

ecm_qt_declare_logging_category(
  ctagsplugin_SRC
  HEADER kate_ctags_debug.h
  IDENTIFIER KTECTAGS
  CATEGORY_NAME "katectagsplugin"
)

ki18n_wrap_ui(ctagsplugin_SRC kate_ctags.ui CTagsGlobalConfig.ui)

add_library(katectagsplugin MODULE ${ctagsplugin_SRC})
target_compile_definitions(katectagsplugin PRIVATE TRANSLATION_DOMAIN="kate-ctags-plugin")
target_link_libraries(katectagsplugin PRIVATE KF5::TextEditor)

kcoreaddons_desktop_to_json(katectagsplugin katectagsplugin.desktop)
install(TARGETS katectagsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
