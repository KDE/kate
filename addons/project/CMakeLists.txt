find_package(Qt5Concurrent ${QT_MIN_VERSION} QUIET)
find_package(
  KF5 ${KF5_DEP_VERSION}
  QUIET
  COMPONENTS
    GuiAddons
    NewStuff
)

set_package_properties(KFGuiAddons PROPERTIES PURPOSE "Required to build the project addon")
set_package_properties(KF5NewStuff PROPERTIES PURPOSE "Required to build the project addon")

if(NOT KF5_FOUND OR NOT Qt5Concurrent_FOUND)
  return()
endif()

kate_add_plugin(kateprojectplugin)
target_compile_definitions(kateprojectplugin PRIVATE TRANSLATION_DOMAIN="kateproject")

target_link_libraries(
    kateprojectplugin
    PRIVATE
        kateshared
        Qt5::Concurrent
        KF5::GuiAddons
        KF5::I18n
        KF5::NewStuff
        KF5::TextEditor
)

include(CheckFunctionExists)
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
    branchesdialog.cpp
    branchcheckoutdialog.cpp
    branchesdialogmodel.cpp
    gitwidget.cpp
    gitstatusmodel.cpp
    gitcommitdialog.cpp
    stashdialog.cpp
    filehistorywidget.cpp
    pushpulldialog.cpp
    comparebranchesview.cpp
    branchdeletedialog.cpp

    tools/clazy.cpp
    tools/clazycurrent.cpp
    tools/codeanalysisselector.cpp
    tools/cppcheck.cpp
    tools/flake8.cpp
    tools/shellcheck.cpp

    git/gitdiff.cpp
    git/gitutils.cpp
    git/gitstatus.cpp

    plugin.qrc
)

install(FILES kateproject.example DESTINATION ${KDE_INSTALL_DATADIR}/kateproject)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
