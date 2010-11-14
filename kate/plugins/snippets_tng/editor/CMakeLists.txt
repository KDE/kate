########### next target ###############

set(ktesnippets_editor_SRCS editor.cpp editorapp.cpp snippeteditorwindow.cpp
  snippeteditornewdialog.cpp ../lib/completionmodel.cpp
  ../lib/dbus_helpers.cpp)

set(ktesnippets_editor_UI
  snippeteditorview.ui
  newsnippetfileview.ui
  filetypelistcreatorview.ui
)

add_definitions(-DSNIPPET_EDITOR)
include_directories("../lib/")


kde4_add_ui_files(ktesnippets_editor_SRCS ${ktesnippets_editor_UI} )

kde4_add_app_icon(ktesnippets_editor_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/apps/kate.png")

kde4_add_executable(ktesnippets_editor ${ktesnippets_editor_SRCS})

target_link_libraries(ktesnippets_editor  ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces)

install(TARGETS ktesnippets_editor ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS "data/ktesnippets_editor.desktop"  DESTINATION  ${XDG_APPS_INSTALL_DIR} )


