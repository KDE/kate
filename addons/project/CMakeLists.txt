project(kateprojectplugin)

find_package(KF5NewStuff ${KF5_DEP_VERSION} REQUIRED) # For KMoreTools

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
  kateprojectcodeanalysistool.cpp
  tools/kateprojectcodeanalysistoolcppcheck.cpp
  tools/kateprojectcodeanalysistoolflake8.cpp
  tools/kateprojectcodeanalysisselector.cpp
)

# resource for ui file and stuff
qt5_add_resources(kateprojectplugin_PART_SRCS plugin.qrc)

add_library(kateprojectplugin MODULE ${kateprojectplugin_PART_SRCS})
kcoreaddons_desktop_to_json (kateprojectplugin kateprojectplugin.desktop)
target_link_libraries(kateprojectplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n
    KF5::GuiAddons
    KF5::ItemViews KF5::ItemModels KF5::IconThemes KF5::ThreadWeaver
    KF5::NewStuff # For KMoreTools
)

########### install files ###############
install(TARGETS kateprojectplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
install( FILES kateproject.example DESTINATION  ${DATA_INSTALL_DIR}/kateproject )

############# unit tests ################
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
