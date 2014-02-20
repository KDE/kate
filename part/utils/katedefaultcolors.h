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

#ifndef __KATE_DEFAULTCOLORS_H__
#define __KATE_DEFAULTCOLORS_H__

#include <KColorScheme>

namespace Kate {

enum ColorRole {
  // editor backgrounds
  Background,
  SelectionBackground,
  HighlightedLineBackground,
  SearchHighlight,
  ReplaceHighlight,
  // text decorations
  HighlightedBracket,
  TabMarker,
  IndentationLine,
  SpellingMistakeLine,
  // icon border
  WordWrapMarker,
  IconBar,
  CodeFolding,
  LineNumber,
  Separator,
  ModifiedLine,
  SavedLine,
  // templates
  TemplateBackground,
  TemplateFocusedEditablePlaceholder,
  TemplateEditablePlaceholder,
  TemplateNotEditablePlaceholder
};

enum Mark {
  Bookmark = 0,
  ActiveBreakpoint,
  ReachedBreakpoint,
  DisabledBreakpoint,
  Execution,
  Warning,
  Error,

  FIRST_MARK = Bookmark,
  LAST_MARK = Error
};

}

class KateDefaultColors
{
public:
  KateDefaultColors();

  QColor color(Kate::ColorRole color) const;
  QColor mark(Kate::Mark mark) const;
  QColor mark(int mark) const;

  enum ColorType {
    ForegroundColor,
    BackgroundColor
  };
  QColor adaptToScheme(const QColor& color, ColorType type) const;

private:
  KColorScheme m_view;
  KColorScheme m_window;
  KColorScheme m_selection;
  KColorScheme m_inactiveSelection;
  QColor m_background;
  QColor m_foreground;
  qreal m_backgroundLuma;
  qreal m_foregroundLuma;
};

#endif // __KATE_DEFAULTCOLORS_H__
