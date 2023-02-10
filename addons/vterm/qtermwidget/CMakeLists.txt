cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(qtermwidget)

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

if (UNIX)
    list(APPEND SRCS
        kprocess.cpp
        kpty.cpp
        kptydevice.cpp
        kptyprocess.cpp
    )
endif()

if (WIN32)
    list(APPEND SRCS iptyprocess.cpp conptyprocess.cpp)
endif()

# Only the Headers that need to be moc'd go here
set(HDRS
    Emulation.h
    Filter.h
    HistorySearch.h
    kprocess.h
    kptydevice.h
    kptyprocess.h
    Pty.h
    qtermwidget.h
    ScreenWindow.h
    SearchBar.h
    Session.h
    TerminalDisplay.h
    Vt102Emulation.h
)

set(UI
    SearchBar.ui
)

# for distribution
set(HDRS_DISTRIB
    qtermwidget.h
    Emulation.h
    KeyboardTranslator.h
    Filter.h
    qtermwidget_interface.h
)

# dirs
set(KB_LAYOUT_DIR "${CMAKE_INSTALL_FULL_DATADIR}/${QTERMWIDGET_LIBRARY_NAME}/kb-layouts")
message(STATUS "Keyboard layouts will be installed in: ${KB_LAYOUT_DIR}")

set(QTERMWIDGET_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${QTERMWIDGET_LIBRARY_NAME}")

# qt5_wrap_cpp(MOCS ${HDRS})
# qt5_wrap_ui(UI_SRCS ${UI})

add_library(${QTERMWIDGET_LIBRARY_NAME} STATIC ${SRCS} ${MOCS} ${UI_SRCS} ${QTERMWIDGET_QM})
if (TARGET Qt6::Core5Compat)
    target_link_libraries(${QTERMWIDGET_LIBRARY_NAME} PRIVATE Qt::Widgets Qt6::Core5Compat) # QTextCodec
else()
    target_link_libraries(${QTERMWIDGET_LIBRARY_NAME} Qt::Widgets)
endif()
set_target_properties( ${QTERMWIDGET_LIBRARY_NAME} PROPERTIES
                       SOVERSION ${QTERMWIDGET_VERSION_MAJOR}
                       VERSION ${QTERMWIDGET_VERSION}
                     )

target_compile_definitions(${QTERMWIDGET_LIBRARY_NAME}
    PRIVATE
        "KB_LAYOUT_DIR=\"${KB_LAYOUT_DIR}\""
        "COLORSCHEMES_DIR=\"${COLORSCHEMES_DIR}\""
        "TRANSLATIONS_DIR=\"${TRANSLATIONS_DIR}\""
        "HAVE_POSIX_OPENPT"
        "HAVE_SYS_TIME_H"
)

