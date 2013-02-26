/*   Kate search plugin
 *
 * Copyright (C) 2012 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "search_while_typing.h"
#include "search_while_typing.moc"

#include <QTime>

void SearchWhileTyping::startSearch(const KTextEditor::Document *doc, const QRegExp &regExp)
{
  int column;
  QTime maxTime;

  maxTime.start();
  for (int line =0; line < doc->lines(); line++) {
    if (maxTime.elapsed() > 50) {
      kDebug() << "Search time exceeded -> stop" << maxTime.elapsed() << line;
      break;
    }
    column = regExp.indexIn(doc->line(line));
    while (column != -1) {
      if (regExp.cap().isEmpty()) break;
      emit matchFound(doc->url().pathOrUrl(), line, column,
                      doc->line(line), regExp.matchedLength());
      column = regExp.indexIn(doc->line(line), column + regExp.cap().size());
    }
  }
  emit searchDone();
}

