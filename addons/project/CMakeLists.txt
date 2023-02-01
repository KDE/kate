find_package(Qt${QT_MAJOR_VERSION}Concurrent ${QT_MIN_VERSION} QUIET)
find_package(
  KF${KF_MAJOR_VERSION} ${KF5_DEP_VERSION}
  QUIET
  COMPONENTS
    GuiAddons
    NewStuff
)

set_package_properties(KF${KF_MAJOR_VERSION}GuiAddons PROPERTIES PURPOSE "Required to build the project addon")
set_package_properties(KF${KF_MAJOR_VERSION}NewStuff PROPERTIES PURPOSE "Required to build the project addon")

if(NOT KF${KF_MAJOR_VERSION}_FOUND OR NOT Qt${QT_MAJOR_VERSION}Concurrent_FOUND)
  return()
endif()

kate_add_plugin(kateprojectplugin)
target_compile_definitions(kateprojectplugin PRIVATE TRANSLATION_DOMAIN="kateproject")

target_link_libraries(
    kateprojectplugin
    PRIVATE
        kateprivate
        Qt::Concurrent
        KF${KF_MAJOR_VERSION}::GuiAddons
        KF${KF_MAJOR_VERSION}::I18n
        KF${KF_MAJOR_VERSION}::NewStuff
        KF${KF_MAJOR_VERSION}::TextEditor
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
    pushpulldialog.cpp
    comparebranchesview.cpp
    branchdeletedialog.cpp
    currentgitbranchbutton.cpp

    tools/clazy.cpp
    tools/clazycurrent.cpp
    tools/codeanalysisselector.cpp
    tools/cppcheck.cpp
    tools/flake8.cpp
    tools/shellcheck.cpp
    tools/eslint.cpp
    tools/clippy.cpp

    git/gitutils.cpp
    git/gitstatus.cpp

    plugin.qrc
)

install(FILES kateproject.example DESTINATION ${KDE_INSTALL_DATADIR}/kateproject)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
