//
// SPDX-FileCopyrightText: 2026 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#pragma once

#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <QStringView>

class AnsiParser
{
public:
    struct FontState {
        enum class FontWeight {
            Normal,
            Faint,
            Bold,
        };

        FontWeight weight = FontWeight::Normal;
        bool italic = false;
        bool underline = false;
        bool blinking = false;
        bool inverse = false;
        bool hidden = false;
        bool striketrough = false;
        QColor foreground{"#f9f9f9"};
        QColor background{"#000000"};
    };
    explicit AnsiParser();

    void setColors(const QColor &black, //
                   const QColor &red, //
                   const QColor &green, //
                   const QColor &yellow, //
                   const QColor &blue, //
                   const QColor &magenta, //
                   const QColor &cyan, //
                   const QColor &white, //
                   const QColor &defColor);

    void setBrightColors(const QColor &black, //
                         const QColor &red, //
                         const QColor &green, //
                         const QColor &yellow, //
                         const QColor &blue, //
                         const QColor &magenta, //
                         const QColor &cyan, //
                         const QColor &white, //
                         const QColor &defColor);

    const static QRegularExpression &htmlAnsiCodeRegEx();
    std::optional<FontState> parseColorCode(const QStringList &commands, const FontState &oldState) const;

private:
    QColor m_black{"#000000"};
    QColor m_red{"#cc0000"};
    QColor m_green{"#00ab00"};
    QColor m_yellow{"#b5a600"};
    QColor m_blue{"#0000be"};
    QColor m_magenta{"#b108bd"};
    QColor m_cyan{"#18d0d0"};
    QColor m_white{"#f5f5f5"};
    QColor m_defColor{"#f9f9f9"};
    QColor m_brightBlack{"#5a5a5a"};
    QColor m_brightRed{"#cc5e5e"};
    QColor m_brightGreen{"#59c359"};
    QColor m_brightYellow{"#e1aa1e"};
    QColor m_brightBlue{"#4375bf"};
    QColor m_brightMagenta{"#ba49d6"};
    QColor m_brightCyan{"#43a8a1"};
    QColor m_brightWhite{"#ffffff"};
    QColor m_brightDefColor{"#ffffff"};
};
