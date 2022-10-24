/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

// Make "signals:", "slots:" visible as access specifiers
#define QT_ANNOTATE_ACCESS_SPECIFIER(a) __attribute__((annotate(#a)))

// Define PYTHON_BINDINGS this will be used in some part of c++ to skip problematic parts
#define PYTHON_BINDINGS

#ifndef QT_WIDGETS_LIB
#define QT_WIDGETS_LIB
#endif

#include <QIcon>

#include <ktexteditor/application.h>
#include <ktexteditor/attribute.h>
#include <ktexteditor/command.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/configpage.h>
#include <ktexteditor/cursor.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/linerange.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/range.h>
#include <ktexteditor/view.h>
