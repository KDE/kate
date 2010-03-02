########### next target ###############

set(katesnippetstng_editor_SRCS editor.cpp editorapp.cpp snippeteditorwindow.cpp
  snippeteditornewdialog.cpp ../lib/completionmodel.cpp
  ../lib/dbus_helpers.cpp)

set(katesnippetstng_editor_UI
  snippeteditorview.ui
  newsnippetfileview.ui
  filetypelistcreatorview.ui
)

add_definitions(-DSNIPPET_EDITOR)
include_directories("../lib/")


kde4_add_ui_files(katesnippetstng_editor_SRCS ${katesnippetstng_editor_UI} )

kde4_add_app_icon(katesnippetstng_editor_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/apps/kate.png")

kde4_add_executable(katesnippetstng_editor ${katesnippetstng_editor_SRCS})

target_link_libraries(katesnippetstng_editor  ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces)

install(TARGETS katesnippetstng_editor ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES "data/katesnippetstng_editor.desktop"  DESTINATION  ${XDG_APPS_INSTALL_DIR} )


