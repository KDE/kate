# Kate shared library, used by Kate/KWrite and the addons
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

target_include_directories(
  kateprivate
  SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/3rdparty
)

find_package(
  KF6 ${KF6_DEP_VERSION}
  QUIET
  REQUIRED
  COMPONENTS
    Crash
    IconThemes
    WindowSystem
    WidgetsAddons
)

target_link_libraries(
  kateprivate
  PUBLIC
    KF6::CoreAddons
    KF6::Crash
    KF6::I18n
    KF6::IconThemes
    KF6::TextEditor
    KF6::WindowSystem
    KF6::KIOGui
    KF6::KIOWidgets
    KF6::WidgetsAddons
)

if (NOT WIN32 AND NOT HAIKU)
    target_compile_definitions(kateprivate PRIVATE -DKDE_INSTALL_FULL_LOCALEDIR=\"${KDE_INSTALL_FULL_LOCALEDIR}\")
    target_include_directories(kateprivate PRIVATE ${Intl_INCLUDE_DIRS})
    target_link_libraries(kateprivate PRIVATE ${Intl_LIBRARIES})
endif()

# shall we use DBus?
# enabled per default on Linux & BSD systems
set(USE_DBUS_DEFAULT OFF)
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
    set(USE_DBUS_DEFAULT ON)
endif()
option(USE_DBUS "Build components using DBus" ${USE_DBUS_DEFAULT})
if(USE_DBUS)
    set(WITH_DBUS TRUE PARENT_SCOPE)
    find_package(KF6DBusAddons ${KF6_DEP_VERSION} REQUIRED)
    target_compile_definitions(kateprivate PUBLIC -DWITH_DBUS)
    target_link_libraries(kateprivate PUBLIC KF6::DBusAddons)
    target_sources(
        kateprivate
        PRIVATE
        kateappadaptor.cpp
        katerunninginstanceinfo.cpp
    )
endif()

# optional KUserFeedback integration
find_package(KF6UserFeedback ${KF6_DEP_VERSION} CONFIG)

if (TARGET KF6::UserFeedbackWidgets)
    target_link_libraries(kateprivate PUBLIC KF6::UserFeedbackWidgets)
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

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
    LIB_KATE_TIME_SOURCES
    HEADER kate_timings_debug.h
    IDENTIFIER LibKateTime
    CATEGORY_NAME "libkate.time"
    DEFAULT_SEVERITY Warning
)

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
    kateconfigdialog.cpp
    kateconfigplugindialogpage.cpp
    katedocmanager.cpp
    katefileactions.cpp
    katemainwindow.cpp
    katemdi.cpp
    katemwmodonhddialog.cpp
    katepluginmanager.cpp
    mainwindow_commands.cpp

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
    texthint/hintview.cpp
    texthint/hintstate.cpp

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
    ${LIB_KATE_TIME_SOURCES}
)

if (ENABLE_PCH)
    target_precompile_headers(kateprivate REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
