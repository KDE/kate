# Kate shared library, used by Kate/KWrite and the addons
ecm_set_disabled_deprecation_versions(
    QT 5.15
    KF 5.90
)

add_library(kateprivate SHARED "")
set_target_properties(kateprivate PROPERTIES VERSION ${RELEASE_SERVICE_VERSION})
install(TARGETS kateprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

# ensure right translations domain, we use one kate.po(t) for lib + apps
target_compile_definitions(kateprivate PRIVATE TRANSLATION_DOMAIN="kate")

# export version to be used by Kate/KWrite/...
target_compile_definitions(kateprivate PUBLIC KATE_VERSION="${RELEASE_SERVICE_VERSION}")

include(GenerateExportHeader)
generate_export_header(
  kateprivate
  EXPORT_FILE_NAME kateprivate_export.h
  EXPORT_MACRO_NAME KATE_PRIVATE_EXPORT
)

target_include_directories(
  kateprivate
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/session
    ${CMAKE_CURRENT_SOURCE_DIR}/quickopen
    ${CMAKE_CURRENT_SOURCE_DIR}/diff
    ${CMAKE_CURRENT_BINARY_DIR} # kateprivate_export.h
)

find_package(
  KF5 ${KF5_DEP_VERSION}
  QUIET
  REQUIRED
  COMPONENTS
    Crash
    IconThemes
    TextEditor
    TextWidgets
    WindowSystem
)

if (NOT WIN32 AND NOT APPLE)
    find_package(KF5 ${KF5_DEP_VERSION} OPTIONAL_COMPONENTS Activities)
endif()

target_link_libraries(
  kateprivate
  PUBLIC
    KF5::CoreAddons
    KF5::Crash
    KF5::I18n
    KF5::IconThemes
    KF5::TextEditor
    KF5::TextWidgets
    KF5::WindowSystem
)

if(KF5Activities_FOUND)
  target_link_libraries(kateprivate PUBLIC KF5::Activities)
  target_compile_definitions(kateprivate PUBLIC -DKF5Activities_FOUND)
endif()

# optional KUserFeedback integration
find_package(KUserFeedback)
if (TARGET KUserFeedbackWidgets)
    target_link_libraries(kateprivate PRIVATE KUserFeedbackWidgets)
    target_compile_definitions(kateprivate PUBLIC -DWITH_KUSERFEEDBACK)
endif()

ki18n_wrap_ui(kateprivate ui/sessionconfigwidget.ui welcomeview/welcomeview.ui session/katesessionmanagedialog.ui)

target_sources(
  kateprivate
  PRIVATE
    session/katesession.cpp
    session/katesessionmanagedialog.cpp
    session/katesessionmanager.cpp
    session/katesessionsaction.cpp

    quickopen/katequickopen.cpp
    quickopen/katequickopenmodel.cpp
    quickopen/katequickopenlineedit.cpp

    kateapp.cpp
    kateappadaptor.cpp
    kateconfigdialog.cpp
    kateconfigplugindialogpage.cpp
    katedocmanager.cpp
    katefileactions.cpp
    katemainwindow.cpp
    katemdi.cpp
    katemwmodonhddialog.cpp
    katepluginmanager.cpp

    katerunninginstanceinfo.cpp
    katesavemodifieddialog.cpp
    katetabbar.cpp
    katesplitter.cpp
    kateviewmanager.cpp
    kateviewspace.cpp
    tabmimedata.cpp

    kateoutputview.cpp
    katestashmanager.cpp

    kateurlbar.cpp

    signal_watcher.cpp
    gitprocess.cpp
    quickdialog.cpp
    ktexteditor_utils.cpp

    data/kateprivate.qrc
    hostprocess.cpp

    diff/diffwidget.cpp
    diff/diffeditor.cpp
    diff/difflinenumarea.cpp
    diff/gitdiff.cpp

    filehistorywidget.cpp

    welcomeview/welcomeview.cpp
    welcomeview/recentitemsmodel.cpp
    welcomeview/savedsessionsmodel.cpp
)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
