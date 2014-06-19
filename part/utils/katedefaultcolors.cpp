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
  , m_foreground(m_view.foreground().color())
  , m_backgroundLuma(KColorUtils::luma(m_background))
  , m_foregroundLuma(KColorUtils::luma(m_foreground))
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
      return adaptToScheme(Qt::yellow, BackgroundColor);
    case ReplaceHighlight:
      return adaptToScheme(Qt::green, BackgroundColor);
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
  // note: the mark color is used as background color at very low opacity (around 0.1)
  // hence, make sure the color returned here has a high saturation
  switch (mark) {
    case Bookmark:
      return adaptToScheme(Qt::blue, BackgroundColor);
    case ActiveBreakpoint:
      return adaptToScheme(Qt::red, BackgroundColor);
    case ReachedBreakpoint:
      return adaptToScheme(Qt::yellow, BackgroundColor);
    case DisabledBreakpoint:
      return adaptToScheme(Qt::magenta, BackgroundColor);
    case Execution:
      return adaptToScheme(Qt::gray, BackgroundColor);
    case Warning:
      return m_view.foreground(KColorScheme::NeutralText).color();
    case Error: {
      return m_view.foreground(KColorScheme::NegativeText).color();
    }
  }
  qFatal("Unhandled color for mark requested: %d\n", mark);
  return QColor();
}

QColor KateDefaultColors::mark(int i) const
{
  Q_ASSERT(i >= FIRST_MARK && i <= LAST_MARK);
  return mark(static_cast<Mark>(i));
}

QColor KateDefaultColors::adaptToScheme(const QColor& color, ColorType type) const
{
  if (m_foregroundLuma > m_backgroundLuma) {
    // for dark color schemes, produce a fitting color by tinting with the foreground/background color
    return KColorUtils::tint(type == ForegroundColor ? m_foreground : m_background, color, 0.5);
  }
  return color;
}
