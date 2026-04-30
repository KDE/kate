/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "katesqlconstants.h"

#include <QBrush>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMetaType>
#include <QPalette>

struct OutputStyle {
    QFont font;
    QBrush background;
    QBrush foreground;
    QBrush changedForeground;
    QBrush changedBackground;

    /**
     * Linearly blend two RGBA colors.
     * @param base     The underlying color.
     * @param overlay  The color to mix in.
     * @param percent  Blend strength (0 = pure @p base, 100 = pure @p overlay).
     */
    static QColor blendColors(const QColor &base, const QColor &overlay, int percent)
    {
        const qreal a = percent / 100.0;
        return QColor::fromRgbF(base.redF() * (1.0 - a) + overlay.redF() * a,
                                base.greenF() * (1.0 - a) + overlay.greenF() * a,
                                base.blueF() * (1.0 - a) + overlay.blueF() * a,
                                base.alphaF() * (1.0 - a) + overlay.alphaF() * a);
    }

    /**
     * Populate an OutputStyle with system-default values.
     *
     * The "changed" foreground/background are pre-blended at 25 % intensity
     * against the base palette colors so they serve as a subtle tint for
     * dirty fields without competing with the full-strength selection highlight.
     */
    static void updateDefault(OutputStyle &style)
    {
        const auto pal = qApp->palette();

        style.font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        style.foreground = pal.text();
        style.background = pal.base();
        style.changedForeground = QBrush(
            blendColors(style.foreground.color(), pal.highlightedText().color(), KateSQLConstants::Config::DefaultValues::HighlightForChangedFieldColorBlend));
        style.changedBackground =
            QBrush(blendColors(style.background.color(), pal.highlight().color(), KateSQLConstants::Config::DefaultValues::HighlightForChangedFieldColorBlend));
    }
};

// Q_DECLARE_METATYPE(OutputStyle)
