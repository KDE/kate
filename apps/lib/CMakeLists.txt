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
  KF6 ${KF5_DEP_VERSION}
  QUIET
  REQUIRED
  COMPONENTS
    Crash
    IconThemes
    TextEditor
    TextWidgets
    WindowSystem
)

target_link_libraries(
  kateprivate
  PUBLIC
    KF6::CoreAddons
    KF6::Crash
    KF6::I18n
    KF6::IconThemes
    KF6::TextEditor
    KF6::TextWidgets
    KF6::WindowSystem
    KF6::KIOGui
    KF6::KIOWidgets
)

if (NOT WIN32 AND NOT HAIKU)
    target_compile_definitions(kateprivate PRIVATE -DKDE_INSTALL_FULL_LOCALEDIR=\"${KDE_INSTALL_FULL_LOCALEDIR}\")
    target_include_directories(kateprivate PRIVATE ${Intl_INCLUDE_DIRS})
    target_link_libraries(kateprivate PRIVATE ${Intl_LIBRARIES})
endif()

# optional KUserFeedback integration
find_package(KUserFeedbackQt6 1.2.1 CONFIG)

if (TARGET KUserFeedbackWidgetsQt6)
    target_link_libraries(kateprivate PUBLIC KUserFeedbackWidgetsQt6)
    target_compile_definitions(kateprivate PUBLIC -DWITH_KUSERFEEDBACK)
endif()

# terminal detection
include(CheckFunctionExists)
check_function_exists(ctermid HAVE_CTERMID)
if(HAVE_CTERMID)
  target_compile_definitions(kateprivate PRIVATE HAVE_CTERMID)
endif()

# detaching from terminal
check_function_exists(daemon HAVE_DAEMON)
if(HAVE_DAEMON)
  target_compile_definitions(kateprivate PRIVATE HAVE_DAEMON)
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
    diagnostics/diagnosticview.cpp
    diagnostics/diagnosticview.qrc
    texthint/KateTextHintManager.cpp
    texthint/tooltip.cpp

    kateurlbar.cpp

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
    commitfilesview.cpp

    welcomeview/welcomeview.cpp
    welcomeview/recentitemsmodel.cpp
    welcomeview/savedsessionsmodel.cpp
)

if (BUILD_PCH)
    target_precompile_headers(kateprivate REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
