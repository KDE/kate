/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QBrush>
#include <QFont>
#include <QMetaType>

struct OutputStyle {
    QFont font;
    QBrush background;
    QBrush foreground;
};

// Q_DECLARE_METATYPE(OutputStyle)
