file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp "/*empty file*/")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c "/*empty file*/")

add_library(katepch STATIC ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c)
target_precompile_headers(katepch PRIVATE pch.h)
set_target_properties(katepch PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(katepch PRIVATE Qt::Core Qt::Gui Qt::Widgets)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/kate_test_pch.h
"#pragma once
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <type_traits>
#include <QMetaObject>
#include <QBitArray>
#include <QByteArray>
#include <QCborArray>
#include <QtCborCommon>
#include <QCborMap>
#include <QCborValue>
#include <QStringList>
#include <QDateTime>
#include <algorithm>
#include <QDebug>
#include <thread>
#include <semaphore>
#include <future>
#include <atomic>
#include <QThread>
")
add_library(katepch_tests STATIC ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp)
target_precompile_headers(katepch_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/kate_test_pch.h)
target_link_libraries(katepch_tests PRIVATE Qt::Core)
