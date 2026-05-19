//
// SPDX-FileCopyrightText: 2026 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "AnsiParser.h"
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

AnsiParser::AnsiParser()
{
}

void AnsiParser::setColors(const QColor &black, //
                           const QColor &red, //
                           const QColor &green, //
                           const QColor &yellow, //
                           const QColor &blue, //
                           const QColor &magenta, //
                           const QColor &cyan, //
                           const QColor &white, //
                           const QColor &defColor)
{
    m_black = black;
    m_red = red;
    m_green = green;
    m_yellow = yellow;
    m_blue = blue;
    m_magenta = magenta;
    m_cyan = cyan;
    m_white = white;
    m_defColor = defColor;
}

void AnsiParser::setBrightColors(const QColor &black, //
                                 const QColor &red, //
                                 const QColor &green, //
                                 const QColor &yellow, //
                                 const QColor &blue, //
                                 const QColor &magenta, //
                                 const QColor &cyan, //
                                 const QColor &white, //
                                 const QColor &defColor)
{
    m_brightBlack = black;
    m_brightRed = red;
    m_brightGreen = green;
    m_brightYellow = yellow;
    m_brightBlue = blue;
    m_brightMagenta = magenta;
    m_brightCyan = cyan;
    m_brightWhite = white;
    m_brightDefColor = defColor;
}

const QRegularExpression &AnsiParser::htmlAnsiCodeRegEx()
{
    const static QRegularExpression reg(u"\\\u001B\\[((?:\\d+;)*(?:\\d+)?)m"_s);
    return reg;
}

std::optional<AnsiParser::FontState> AnsiParser::parseColorCode(const QStringList &codes, const AnsiParser::FontState &oldState) const
{
    FontState font = oldState;
    if (codes.isEmpty()) {
        return FontState();
    }
    for (const auto &codeStr : codes) {
        bool ok;
        int code = codeStr.toInt(&ok);
        if (!ok) {
            qDebug() << "Conversion failed";
        }
        switch (code) {
        case 0:
            font = FontState();
            break;
        case 1:
            qDebug() << "bold";
            font.weight = FontState::FontWeight::Bold;
            break;
        case 2:
            font.weight = FontState::FontWeight::Faint;
            break;
        case 22:
            font.weight = FontState::FontWeight::Normal;
            break;
        case 3:
            font.italic = true;
            break;
        case 23:
            font.italic = false;
            break;
        case 4:
            font.underline = true;
            break;
        case 24:
            font.underline = false;
            break;
        case 5:
            font.blinking = true;
            break;
        case 25:
            font.blinking = false;
            break;
        case 7:
            font.inverse = true;
            break;
        case 27:
            font.inverse = false;
            break;
        case 8:
            font.hidden = true;
            break;
        case 28:
            font.hidden = false;
            break;
        case 9:
            font.striketrough = true;
            break;
        case 29:
            font.striketrough = false;
            break;
        case 30:
            font.foreground = m_black;
            break;
        case 31:
            font.foreground = m_red;
            break;
        case 32:
            font.foreground = m_green;
            break;
        case 33:
            font.foreground = m_yellow;
            break;
        case 34:
            font.foreground = m_blue;
            break;
        case 35:
            font.foreground = m_magenta;
            break;
        case 36:
            font.foreground = m_cyan;
            break;
        case 37:
            font.foreground = m_white;
            break;
        case 39:
            font.foreground = m_defColor;
            break;
        case 40:
            font.background = m_black;
            break;
        case 41:
            font.background = m_red;
            break;
        case 42:
            font.background = m_green;
            break;
        case 43:
            font.background = m_yellow;
            break;
        case 44:
            font.background = m_blue;
            break;
        case 45:
            font.background = m_magenta;
            break;
        case 46:
            font.background = m_cyan;
            break;
        case 47:
            font.background = m_white;
            break;
        case 49:
            font.background = m_defColor;
            break;
        case 90:
            font.foreground = m_brightBlack;
            break;
        case 91:
            font.foreground = m_brightRed;
            break;
        case 92:
            font.foreground = m_brightGreen;
            break;
        case 93:
            font.foreground = m_brightYellow;
            break;
        case 94:
            font.foreground = m_brightBlue;
            break;
        case 95:
            font.foreground = m_brightMagenta;
            break;
        case 96:
            font.foreground = m_brightCyan;
            break;
        case 97:
            font.foreground = m_brightWhite;
            break;
        case 100:
            font.background = m_brightBlack;
            break;
        case 101:
            font.background = m_brightRed;
            break;
        case 102:
            font.background = m_brightGreen;
            break;
        case 103:
            font.background = m_brightYellow;
            break;
        case 104:
            font.background = m_brightBlue;
            break;
        case 105:
            font.background = m_brightMagenta;
            break;
        case 106:
            font.background = m_brightCyan;
            break;
        case 107:
            font.background = m_brightWhite;
            break;
        default:
            qDebug() << "failed to parse";
            return std::nullopt;
        }
    }
    return font;
}
