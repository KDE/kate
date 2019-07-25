find_package(KF5NewStuff ${KF5_DEP_VERSION} REQUIRED) # For KMoreTools

add_library(kateprojectplugin MODULE "")
target_compile_definitions(kateprojectplugin PRIVATE TRANSLATION_DOMAIN="kateproject")

target_link_libraries(
  kateprojectplugin 
  PRIVATE
    KF5::TextEditor
    KF5::Parts 
    KF5::I18n
    KF5::GuiAddons
    KF5::ItemViews 
    KF5::ItemModels 
    KF5::IconThemes 
    KF5::ThreadWeaver
    KF5::NewStuff
)

check_function_exists(ctermid HAVE_CTERMID)
if(HAVE_CTERMID)
  target_compile_definitions(kateprojectplugin PRIVATE HAVE_CTERMID)
endif()

target_sources(
  kateprojectplugin 
  PRIVATE
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
    tools/kateprojectcodeanalysistoolshellcheck.cpp
    tools/kateprojectcodeanalysisselector.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(kateprojectplugin kateprojectplugin.desktop)
install(TARGETS kateprojectplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
install(FILES kateproject.example DESTINATION ${DATA_INSTALL_DIR}/kateproject)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
