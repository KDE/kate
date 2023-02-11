
project(qtermwidget)

set(KF5_DEP_VERSION "5.90")
if (UNIX)
    find_package(KF${KF_MAJOR_VERSION} ${KF5_DEP_VERSION} REQUIRED Pty)
endif()

# just change version for releases
# keep this in sync with the version in pyqt/pyproject.toml
set(QTERMWIDGET_VERSION_MAJOR "1")
set(QTERMWIDGET_VERSION_MINOR "2")
set(QTERMWIDGET_VERSION_PATCH "0")

set(QTERMWIDGET_VERSION "${QTERMWIDGET_VERSION_MAJOR}.${QTERMWIDGET_VERSION_MINOR}.${QTERMWIDGET_VERSION_PATCH}")

# additional cmake files
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Minimum Versions

set(QTERMWIDGET_LIBRARY_NAME qtermwidget5)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# main library

set(SRCS
    # BlockArray.cpp
    ColorScheme.cpp
    Emulation.cpp
    Filter.cpp
    History.cpp
    HistorySearch.cpp
    KeyboardTranslator.cpp
    konsole_wcwidth.cpp
    Pty.cpp
    qtermwidget.cpp
    Screen.cpp
    ScreenWindow.cpp
    SearchBar.cpp
    Session.cpp
    ShellCommand.cpp
    TerminalCharacterDecoder.cpp
    TerminalDisplay.cpp
    tools.cpp
    Vt102Emulation.cpp
    plugin.qrc
)

if (WIN32)
    list(APPEND SRCS iptyprocess.cpp conptyprocess.cpp)
endif()

set(UI
    SearchBar.ui
)

# dirs
set(KB_LAYOUT_DIR "${CMAKE_INSTALL_FULL_DATADIR}/${QTERMWIDGET_LIBRARY_NAME}/kb-layouts")
message(STATUS "Keyboard layouts will be installed in: ${KB_LAYOUT_DIR}")

add_library(${QTERMWIDGET_LIBRARY_NAME} STATIC ${SRCS})

if (UNIX)
    set(PTYLIB KF${KF_MAJOR_VERSION}::Pty)
else()
    set(PTYLIB "")
endif()

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(${QTERMWIDGET_LIBRARY_NAME} PRIVATE Qt::Widgets Qt6::Core5Compat ${PTYLIB})
else()
    target_link_libraries(${QTERMWIDGET_LIBRARY_NAME} PRIVATE Qt::Widgets ${PTYLIB})
endif()

set_target_properties(${QTERMWIDGET_LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(${QTERMWIDGET_LIBRARY_NAME}
    PRIVATE
        "KB_LAYOUT_DIR=\"${KB_LAYOUT_DIR}\""
        "COLORSCHEMES_DIR=\"${COLORSCHEMES_DIR}\""
        "TRANSLATIONS_DIR=\"${TRANSLATIONS_DIR}\""
        "HAVE_POSIX_OPENPT"
        "HAVE_SYS_TIME_H"
)

