/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2014 Milian Wolff <mail@milianw.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katedefaultcolors.h"

#include <ktexteditor/markinterface.h>

#include <KColorUtils>

using namespace Kate;

KateDefaultColors::KateDefaultColors()
  : m_view(QPalette::Active, KColorScheme::View)
  , m_window(QPalette::Active, KColorScheme::Window)
  , m_selection(QPalette::Active, KColorScheme::Selection)
  , m_inactiveSelection(QPalette::Inactive, KColorScheme::Selection)
  , m_background(m_view.background().color())
  , m_backgroundLuma(KColorUtils::luma(m_background))
{
}

QColor KateDefaultColors::color(ColorRole role) const
{
  switch(role) {
    case Background:
      return m_background;
    case SelectionBackground:
      return m_selection.background().color();
    case HighlightedLineBackground:
      return m_view.background(KColorScheme::AlternateBackground).color();
    case HighlightedBracket:
      return KColorUtils::tint(m_background, m_view.decoration(KColorScheme::HoverColor).color());
    case WordWrapMarker:
      return KColorUtils::shade(m_background, m_backgroundLuma > 0.3 ? -0.15 : 0.03);
    case TabMarker:
      return KColorUtils::shade(m_background, m_backgroundLuma > 0.7 ? -0.35 : 0.3);
    case IndentationLine:
      return KColorUtils::shade(m_background, m_backgroundLuma > 0.7 ? -0.35 : 0.3);
    case IconBar:
      return m_window.background().color();
    case CodeFolding:
      return m_inactiveSelection.background().color();
    case LineNumber:
      return m_window.foreground().color();
    case Separator:
      return m_view.foreground(KColorScheme::InactiveText).color();
    case SpellingMistakeLine:
      return m_view.foreground(KColorScheme::NegativeText).color();
    case ModifiedLine:
      return m_view.background(KColorScheme::NegativeBackground).color();
    case SavedLine:
      return m_view.background(KColorScheme::PositiveBackground).color();
    case SearchHighlight:
      return QColor(Qt::yellow); // TODO: adapt to color scheme
    case ReplaceHighlight:
      return QColor(Qt::green); // TODO: adapt to color scheme
    case TemplateBackground:
      return m_window.background().color();
    case TemplateFocusedEditablePlaceholder:
      return m_view.background(KColorScheme::PositiveBackground).color();
    case TemplateEditablePlaceholder:
      return m_view.background(KColorScheme::PositiveBackground).color();
    case TemplateNotEditablePlaceholder:
      return m_view.background(KColorScheme::NegativeBackground).color();
  }
  qFatal("Unhandled color requested: %d\n", role);
  return QColor();
}

QColor KateDefaultColors::mark(Mark mark) const
{
  static const QColor colors[LAST_MARK + 1] = {
    Qt::blue,
    Qt::red,
    Qt::yellow,
    Qt::magenta,
    Qt::gray,
    Qt::green,
    Qt::red
  };
   // TODO: adapt to color scheme
  return colors[mark];
}

QColor KateDefaultColors::mark(int i) const
{
  Q_ASSERT(i >= FIRST_MARK && i <= LAST_MARK);
  return mark(static_cast<Mark>(i));
}
