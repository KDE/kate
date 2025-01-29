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
    project_commands.cpp

    tools/codeanalysisselector.cpp
    git/gitutils.cpp
    git/gitstatus.cpp

    plugin.qrc
)

install(FILES kateproject.example DESTINATION ${KDE_INSTALL_DATADIR}/kateproject)

if (ENABLE_PCH)
    target_precompile_headers(kateprojectplugin REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
