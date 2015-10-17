project(kateprojectplugin)

find_package(KF5NewStuff ${KF5_DEP_VERSION} REQUIRED) # For KMoreTools

# libgit2 integration, at least 0.22 with proper git_libgit2_init()
find_package(LibGit2 "0.22.0")

# set right defines for libgit2 usage
if(LIBGIT2_FOUND)
    add_definitions(-DLIBGIT2_FOUND)
    set (PROJECT_OPTIONAL_LIBS LibGit2::LibGit2)
endif()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DTRANSLATION_DOMAIN=\"kateproject\")
set(kateprojectplugin_PART_SRCS
  fileutil.cpp
  kateprojectplugin.cpp
  kateprojectpluginview.cpp
  kateproject.cpp
  kateprojectworker.cpp
  kateprojectitem.cpp
  kateprojectview.cpp
  kateprojectviewtree.cpp
  kateprojecttreeviewcontextmenu.cpp
  kateprojectinfoview.cpp
  kateprojectcompletion.cpp
  kateprojectindex.cpp
  kateprojectinfoviewindex.cpp
  kateprojectinfoviewterminal.cpp
  kateprojectinfoviewcodeanalysis.cpp
  kateprojectinfoviewnotes.cpp
  kateprojectconfigpage.cpp
)

add_library(kateprojectplugin MODULE ${kateprojectplugin_PART_SRCS})

target_link_libraries(kateprojectplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n
    KF5::GuiAddons
    Qt5::Script KF5::ItemViews KF5::ItemModels KF5::IconThemes KF5::ThreadWeaver
    KF5::NewStuff # For KMoreTools
    ${PROJECT_OPTIONAL_LIBS}
)

########### install files ###############
install(TARGETS kateprojectplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

install( FILES ui.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/kateproject )
install( FILES kateproject.example DESTINATION  ${DATA_INSTALL_DIR}/kateproject )

kcoreaddons_desktop_to_json (kateprojectplugin kateprojectplugin.desktop)

############# unit tests ################
ecm_optional_add_subdirectory (autotests)
