find_package(Qt6Concurrent ${QT_MIN_VERSION} QUIET)
find_package(KF6GuiAddons ${KF5_DEP_VERSION} QUIET)

set_package_properties(KF6GuiAddons PROPERTIES PURPOSE "Required to build the project addon")

if(NOT KF6_FOUND OR NOT Qt6Concurrent_FOUND)
  return()
endif()

kate_add_plugin(kateprojectplugin)
target_compile_definitions(kateprojectplugin PRIVATE TRANSLATION_DOMAIN="kateproject")

target_link_libraries(
    kateprojectplugin
    PRIVATE
        kateprivate
        Qt::Concurrent
        KF6::GuiAddons
        KF6::I18n
        KF6::TextEditor
)

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

if (BUILD_PCH)
    target_precompile_headers(kateprojectplugin REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
