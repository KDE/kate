/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QColor>
#include <QLatin1String>
#include <QStringView>

#include <algorithm>
#include <array>

namespace ColorNames
{
struct NamedColor {
    QLatin1String name;
    QRgb rgb;
};

// The CSS named colors, sorted by name, see https://www.w3.org/TR/css-color-4/#named-colors
// QColor only knows the SVG 1.1 names, which lack e.g. rebeccapurple, bug 523349
// NOLINTBEGIN(modernize-use-designated-initializers)
inline constexpr auto s_colorNames = std::to_array<NamedColor>({
    {QLatin1String("aliceblue"), 0xf0f8ff},
    {QLatin1String("antiquewhite"), 0xfaebd7},
    {QLatin1String("aqua"), 0x00ffff},
    {QLatin1String("aquamarine"), 0x7fffd4},
    {QLatin1String("azure"), 0xf0ffff},
    {QLatin1String("beige"), 0xf5f5dc},
    {QLatin1String("bisque"), 0xffe4c4},
    {QLatin1String("black"), 0x000000},
    {QLatin1String("blanchedalmond"), 0xffebcd},
    {QLatin1String("blue"), 0x0000ff},
    {QLatin1String("blueviolet"), 0x8a2be2},
    {QLatin1String("brown"), 0xa52a2a},
    {QLatin1String("burlywood"), 0xdeb887},
    {QLatin1String("cadetblue"), 0x5f9ea0},
    {QLatin1String("chartreuse"), 0x7fff00},
    {QLatin1String("chocolate"), 0xd2691e},
    {QLatin1String("coral"), 0xff7f50},
    {QLatin1String("cornflowerblue"), 0x6495ed},
    {QLatin1String("cornsilk"), 0xfff8dc},
    {QLatin1String("crimson"), 0xdc143c},
    {QLatin1String("cyan"), 0x00ffff},
    {QLatin1String("darkblue"), 0x00008b},
    {QLatin1String("darkcyan"), 0x008b8b},
    {QLatin1String("darkgoldenrod"), 0xb8860b},
    {QLatin1String("darkgray"), 0xa9a9a9},
    {QLatin1String("darkgreen"), 0x006400},
    {QLatin1String("darkgrey"), 0xa9a9a9},
    {QLatin1String("darkkhaki"), 0xbdb76b},
    {QLatin1String("darkmagenta"), 0x8b008b},
    {QLatin1String("darkolivegreen"), 0x556b2f},
    {QLatin1String("darkorange"), 0xff8c00},
    {QLatin1String("darkorchid"), 0x9932cc},
    {QLatin1String("darkred"), 0x8b0000},
    {QLatin1String("darksalmon"), 0xe9967a},
    {QLatin1String("darkseagreen"), 0x8fbc8f},
    {QLatin1String("darkslateblue"), 0x483d8b},
    {QLatin1String("darkslategray"), 0x2f4f4f},
    {QLatin1String("darkslategrey"), 0x2f4f4f},
    {QLatin1String("darkturquoise"), 0x00ced1},
    {QLatin1String("darkviolet"), 0x9400d3},
    {QLatin1String("deeppink"), 0xff1493},
    {QLatin1String("deepskyblue"), 0x00bfff},
    {QLatin1String("dimgray"), 0x696969},
    {QLatin1String("dimgrey"), 0x696969},
    {QLatin1String("dodgerblue"), 0x1e90ff},
    {QLatin1String("firebrick"), 0xb22222},
    {QLatin1String("floralwhite"), 0xfffaf0},
    {QLatin1String("forestgreen"), 0x228b22},
    {QLatin1String("fuchsia"), 0xff00ff},
    {QLatin1String("gainsboro"), 0xdcdcdc},
    {QLatin1String("ghostwhite"), 0xf8f8ff},
    {QLatin1String("gold"), 0xffd700},
    {QLatin1String("goldenrod"), 0xdaa520},
    {QLatin1String("gray"), 0x808080},
    {QLatin1String("green"), 0x008000},
    {QLatin1String("greenyellow"), 0xadff2f},
    {QLatin1String("grey"), 0x808080},
    {QLatin1String("honeydew"), 0xf0fff0},
    {QLatin1String("hotpink"), 0xff69b4},
    {QLatin1String("indianred"), 0xcd5c5c},
    {QLatin1String("indigo"), 0x4b0082},
    {QLatin1String("ivory"), 0xfffff0},
    {QLatin1String("khaki"), 0xf0e68c},
    {QLatin1String("lavender"), 0xe6e6fa},
    {QLatin1String("lavenderblush"), 0xfff0f5},
    {QLatin1String("lawngreen"), 0x7cfc00},
    {QLatin1String("lemonchiffon"), 0xfffacd},
    {QLatin1String("lightblue"), 0xadd8e6},
    {QLatin1String("lightcoral"), 0xf08080},
    {QLatin1String("lightcyan"), 0xe0ffff},
    {QLatin1String("lightgoldenrodyellow"), 0xfafad2},
    {QLatin1String("lightgray"), 0xd3d3d3},
    {QLatin1String("lightgreen"), 0x90ee90},
    {QLatin1String("lightgrey"), 0xd3d3d3},
    {QLatin1String("lightpink"), 0xffb6c1},
    {QLatin1String("lightsalmon"), 0xffa07a},
    {QLatin1String("lightseagreen"), 0x20b2aa},
    {QLatin1String("lightskyblue"), 0x87cefa},
    {QLatin1String("lightslategray"), 0x778899},
    {QLatin1String("lightslategrey"), 0x778899},
    {QLatin1String("lightsteelblue"), 0xb0c4de},
    {QLatin1String("lightyellow"), 0xffffe0},
    {QLatin1String("lime"), 0x00ff00},
    {QLatin1String("limegreen"), 0x32cd32},
    {QLatin1String("linen"), 0xfaf0e6},
    {QLatin1String("magenta"), 0xff00ff},
    {QLatin1String("maroon"), 0x800000},
    {QLatin1String("mediumaquamarine"), 0x66cdaa},
    {QLatin1String("mediumblue"), 0x0000cd},
    {QLatin1String("mediumorchid"), 0xba55d3},
    {QLatin1String("mediumpurple"), 0x9370db},
    {QLatin1String("mediumseagreen"), 0x3cb371},
    {QLatin1String("mediumslateblue"), 0x7b68ee},
    {QLatin1String("mediumspringgreen"), 0x00fa9a},
    {QLatin1String("mediumturquoise"), 0x48d1cc},
    {QLatin1String("mediumvioletred"), 0xc71585},
    {QLatin1String("midnightblue"), 0x191970},
    {QLatin1String("mintcream"), 0xf5fffa},
    {QLatin1String("mistyrose"), 0xffe4e1},
    {QLatin1String("moccasin"), 0xffe4b5},
    {QLatin1String("navajowhite"), 0xffdead},
    {QLatin1String("navy"), 0x000080},
    {QLatin1String("oldlace"), 0xfdf5e6},
    {QLatin1String("olive"), 0x808000},
    {QLatin1String("olivedrab"), 0x6b8e23},
    {QLatin1String("orange"), 0xffa500},
    {QLatin1String("orangered"), 0xff4500},
    {QLatin1String("orchid"), 0xda70d6},
    {QLatin1String("palegoldenrod"), 0xeee8aa},
    {QLatin1String("palegreen"), 0x98fb98},
    {QLatin1String("paleturquoise"), 0xafeeee},
    {QLatin1String("palevioletred"), 0xdb7093},
    {QLatin1String("papayawhip"), 0xffefd5},
    {QLatin1String("peachpuff"), 0xffdab9},
    {QLatin1String("peru"), 0xcd853f},
    {QLatin1String("pink"), 0xffc0cb},
    {QLatin1String("plum"), 0xdda0dd},
    {QLatin1String("powderblue"), 0xb0e0e6},
    {QLatin1String("purple"), 0x800080},
    {QLatin1String("rebeccapurple"), 0x663399},
    {QLatin1String("red"), 0xff0000},
    {QLatin1String("rosybrown"), 0xbc8f8f},
    {QLatin1String("royalblue"), 0x4169e1},
    {QLatin1String("saddlebrown"), 0x8b4513},
    {QLatin1String("salmon"), 0xfa8072},
    {QLatin1String("sandybrown"), 0xf4a460},
    {QLatin1String("seagreen"), 0x2e8b57},
    {QLatin1String("seashell"), 0xfff5ee},
    {QLatin1String("sienna"), 0xa0522d},
    {QLatin1String("silver"), 0xc0c0c0},
    {QLatin1String("skyblue"), 0x87ceeb},
    {QLatin1String("slateblue"), 0x6a5acd},
    {QLatin1String("slategray"), 0x708090},
    {QLatin1String("slategrey"), 0x708090},
    {QLatin1String("snow"), 0xfffafa},
    {QLatin1String("springgreen"), 0x00ff7f},
    {QLatin1String("steelblue"), 0x4682b4},
    {QLatin1String("tan"), 0xd2b48c},
    {QLatin1String("teal"), 0x008080},
    {QLatin1String("thistle"), 0xd8bfd8},
    {QLatin1String("tomato"), 0xff6347},
    {QLatin1String("turquoise"), 0x40e0d0},
    {QLatin1String("violet"), 0xee82ee},
    {QLatin1String("wheat"), 0xf5deb3},
    {QLatin1String("white"), 0xffffff},
    {QLatin1String("whitesmoke"), 0xf5f5f5},
    {QLatin1String("yellow"), 0xffff00},
    {QLatin1String("yellowgreen"), 0x9acd32},
});
}
// NOLINTEND(modernize-use-designated-initializers)

/**
 * Parse a color as written in CSS: hex (#rgb, #rrggbb, ...) or a named color, e.g. rebeccapurple.
 * Returns an invalid color for anything else.
 */
inline QColor colorFromText(QStringView text)
{
    if (text.startsWith(u'#')) {
        return QColor::fromString(text);
    }

    // names are case insensitive in CSS
    using ColorNames::NamedColor;
    using ColorNames::s_colorNames;
    auto it = std::lower_bound(s_colorNames.begin(), s_colorNames.end(), text, [](const NamedColor &color, QStringView name) {
        return color.name.compare(name, Qt::CaseInsensitive) < 0;
    });
    if (it != s_colorNames.end() && it->name.compare(text, Qt::CaseInsensitive) == 0) {
        return QColor::fromRgb(it->rgb);
    }
    return {};
}
