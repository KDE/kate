/* 
 * Copyright (C) 2008-2009 by Michel Ludwig (michel.ludwig@kdemail.net)
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

/* If ever threads should be used again, thread communication and
 * synchronization ought to be done with blocking queued signal connections.
 */

#include "spellcheck.h"

#include <QMutex>
#include <QHash>
#include <QtAlgorithms>
#include <QTimer>
#include <QThread>

#include <kactioncollection.h>
#include <ktexteditor/smartinterface.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/view.h>
#include <sonnet/speller.h>

#include "katedocument.h"
#include "katehighlight.h"
#include "spellcheck.h"

KateSpellCheckManager::KateSpellCheckManager(QObject *parent)
: QObject(parent)
{
}

KateSpellCheckManager::~KateSpellCheckManager()
{
}

QList<KTextEditor::Range> KateSpellCheckManager::rangeDifference(const KTextEditor::Range& r1,
                                                                 const KTextEditor::Range& r2)
{
  Q_ASSERT(r1.contains(r2));
  QList<KTextEditor::Range> toReturn;
  KTextEditor::Range before(r1.start(), r2.start());
  KTextEditor::Range after(r2.end(), r1.end());
  if(!before.isEmpty()) {
    toReturn.push_back(before);
  }
  if(!after.isEmpty()) {
    toReturn.push_back(after);
  }
  return toReturn;
}

namespace {
  bool lessThanRangeDictionaryPair(const QPair<KTextEditor::Range, QString> &s1,
                                          const QPair<KTextEditor::Range, QString> &s2)
  {
      return s1.first.end() <= s2.first.start();
  }
}

QList<QPair<KTextEditor::Range, QString> > KateSpellCheckManager::spellCheckLanguageRanges(KateDocument *doc,
                                                                                           const KTextEditor::Range& range)
{
  QString defaultDict = doc->defaultDictionary();
  QList<RangeDictionaryPair> toReturn;
  QMutexLocker smartLock(doc->smartMutex());
  QList<QPair<KTextEditor::SmartRange*, QString> > dictionaryRanges = doc->dictionaryRanges();
  if(dictionaryRanges.isEmpty()) {
    toReturn.push_back(RangeDictionaryPair(range, defaultDict));
    return toReturn;
  }
  QList<KTextEditor::Range> splitQueue;
  splitQueue.push_back(range);
  while(!splitQueue.isEmpty()) {
    bool handled = false;
    KTextEditor::Range consideredRange = splitQueue.takeFirst();
    for(QList<QPair<KTextEditor::SmartRange*, QString> >::iterator i = dictionaryRanges.begin();
        i != dictionaryRanges.end(); ++i) {
      KTextEditor::Range languageRange = *((*i).first);
      KTextEditor::Range intersection = languageRange.intersect(consideredRange);
      if(intersection.isEmpty()) {
        continue;
      }
      toReturn.push_back(RangeDictionaryPair(intersection, (*i).second));
      splitQueue += rangeDifference(consideredRange, intersection);
      handled = true;
      break;
    }
    if(!handled) {
      // 'consideredRange' did not intersect with any dictionary range, so we add it with the default dictionary
      toReturn.push_back(RangeDictionaryPair(consideredRange, defaultDict));
    }
  }
  // finally, we still have to sort the list
  qStableSort(toReturn.begin(), toReturn.end(), lessThanRangeDictionaryPair);
  return toReturn;
}

QList<QPair<KTextEditor::Range, QString> > KateSpellCheckManager::spellCheckWrtHighlightingRanges(KateDocument *document,
                                                                                         const KTextEditor::Range& range,
                                                                                         const QString& dictionary,
                                                                                         bool singleLine)
{
  QList<QPair<KTextEditor::Range, QString> > toReturn;
  QList<KTextEditor::Range> rangesToSplit;
  if(!singleLine || range.onSingleLine()) {
    rangesToSplit.push_back(range);
  }
  else {
    const int startLine = range.start().line();
    const int startColumn = range.start().column();
    const int endLine = range.end().line();
    const int endColumn = range.end().column();
    for(int line = startLine; line <= endLine; ++line) {
      const int start = (line == startLine) ? startColumn : 0;
      const int end = (line == endLine) ? endColumn : document->lineLength(line);
      KTextEditor::Range toAdd(line, start, line, end);
      if(!toAdd.isEmpty()) {
        rangesToSplit.push_back(toAdd);
      }
    }
  }
  for(QList<KTextEditor::Range>::iterator i = rangesToSplit.begin(); i != rangesToSplit.end(); ++i) {
    KTextEditor::Range rangeToSplit = *i;
    KTextEditor::Cursor begin = KTextEditor::Cursor::invalid();
    const int startLine = rangeToSplit.start().line();
    const int startColumn = rangeToSplit.start().column();
    const int endLine = rangeToSplit.end().line();
    const int endColumn = rangeToSplit.end().column();
    bool inSpellCheckArea = false;
    for(int line = startLine; line <= endLine; ++line) {
      KateTextLine::Ptr kateTextLine = document->kateTextLine(line);
      const int start = (line == startLine) ? startColumn : 0;
      const int end = (line == endLine) ? endColumn : kateTextLine->length();
      const KTextEditor::Cursor startCursor();
      for(int i = start; i < end; ++i) {
        unsigned int attribute = kateTextLine->attribute(i);
        if(!document->highlight()->attributeRequiresSpellchecking(attribute)) {
          if(i == start) {
            continue;
          }
          else if(inSpellCheckArea) {
            toReturn.push_back(RangeDictionaryPair(KTextEditor::Range(begin, KTextEditor::Cursor(line, i)), dictionary));
            begin = KTextEditor::Cursor::invalid();
            inSpellCheckArea = false;
          }
        }
        else if(!inSpellCheckArea) {
          begin = KTextEditor::Cursor(line, i);
          inSpellCheckArea = true;
        }
      }
    }
    if(inSpellCheckArea) {
      toReturn.push_back(RangeDictionaryPair(KTextEditor::Range(begin, rangeToSplit.end()), dictionary));
    }
  }
  return toReturn;
}

QList<QPair<KTextEditor::Range, QString> > KateSpellCheckManager::spellCheckRanges(KateDocument *doc,
                                                                                   const KTextEditor::Range& range,
                                                                                   bool singleLine)
{
  QList<RangeDictionaryPair> toReturn;
  QList<RangeDictionaryPair> languageRangeList = spellCheckLanguageRanges(doc, range);
  for(QList<RangeDictionaryPair>::iterator i = languageRangeList.begin(); i != languageRangeList.end(); ++i) {
    const RangeDictionaryPair& p = *i;
    toReturn += spellCheckWrtHighlightingRanges(doc, p.first, p.second, singleLine);
  }
  return toReturn;
}

#include "spellcheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
