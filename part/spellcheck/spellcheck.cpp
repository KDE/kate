/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009 by Michel Ludwig <michel.ludwig@kdemail.net>
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

/* If ever threads should be used again, thread communication and
 * synchronization ought to be done with blocking queued signal connections.
 */

#include "spellcheck.h"

#include <QHash>
#include <QtAlgorithms>
#include <QTimer>
#include <QThread>

#include <kactioncollection.h>
#include <ktexteditor/view.h>
#include <sonnet/speller.h>

#include "katedocument.h"
#include "katehighlight.h"

KateSpellCheckManager::KateSpellCheckManager(QObject *parent)
: QObject(parent)
{
}

KateSpellCheckManager::~KateSpellCheckManager()
{
}

QStringList KateSpellCheckManager::suggestions(const QString& word, const QString& dictionary)
{
  Sonnet::Speller speller;
  speller.setLanguage(dictionary);
  return speller.suggest(word);
}

void KateSpellCheckManager::ignoreWord(const QString& word, const QString& dictionary)
{
  Sonnet::Speller speller;
  speller.setLanguage(dictionary);
  speller.addToSession(word);
}

void KateSpellCheckManager::addToDictionary(const QString& word, const QString& dictionary)
{
  Sonnet::Speller speller;
  speller.setLanguage(dictionary);
  speller.addToPersonal(word);
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
  QList<QPair<KTextEditor::MovingRange*, QString> > dictionaryRanges = doc->dictionaryRanges();
  if(dictionaryRanges.isEmpty()) {
    toReturn.push_back(RangeDictionaryPair(range, defaultDict));
    return toReturn;
  }
  QList<KTextEditor::Range> splitQueue;
  splitQueue.push_back(range);
  while(!splitQueue.isEmpty()) {
    bool handled = false;
    KTextEditor::Range consideredRange = splitQueue.takeFirst();
    for(QList<QPair<KTextEditor::MovingRange*, QString> >::iterator i = dictionaryRanges.begin();
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
                                                                                         bool singleLine,
                                                                                         bool returnSingleRange)
{
  QList<QPair<KTextEditor::Range, QString> > toReturn;
  if(range.isEmpty()) {
    return toReturn;
  }

  KateHighlighting *highlighting = document->highlight();

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
      Kate::TextLine kateTextLine = document->kateTextLine(line);
      const int start = (line == startLine) ? startColumn : 0;
      const int end = (line == endLine) ? endColumn : kateTextLine->length();
      const KTextEditor::Cursor startCursor();
      for(int i = start; i < end;) { // WARNING: 'i' has to be incremented manually!
        int attr = kateTextLine->attribute(i);
        const KatePrefixStore& prefixStore = highlighting->getCharacterEncodingsPrefixStore(attr);
        QString prefixFound = prefixStore.findPrefix(kateTextLine, i);
        unsigned int attribute = kateTextLine->attribute(i);
        if(!document->highlight()->attributeRequiresSpellchecking(attribute)
           && prefixFound.isEmpty()) {
          if(i == start) {
            ++i;
            continue;
          }
          else if(inSpellCheckArea) {
            KTextEditor::Range spellCheckRange(begin, KTextEditor::Cursor(line, i));
            // work around Qt bug 6498
            trimRange(document, spellCheckRange);
            if(!spellCheckRange.isEmpty()) {
              toReturn.push_back(RangeDictionaryPair(spellCheckRange, dictionary));
              if(returnSingleRange) {
                return toReturn;
              }
            }
            begin = KTextEditor::Cursor::invalid();
            inSpellCheckArea = false;
          }
        }
        else if(!inSpellCheckArea) {
          begin = KTextEditor::Cursor(line, i);
          inSpellCheckArea = true;
        }
        if(!prefixFound.isEmpty()) {
          i += prefixFound.length();
        }
        else {
          ++i;
        }
      }
    }
    if(inSpellCheckArea) {
      KTextEditor::Range spellCheckRange(begin, rangeToSplit.end());
      // work around Qt bug 6498
      trimRange(document, spellCheckRange);
      if(!spellCheckRange.isEmpty()) {
        toReturn.push_back(RangeDictionaryPair(spellCheckRange, dictionary));
        if(returnSingleRange) {
          return toReturn;
        }
      }
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

void KateSpellCheckManager::replaceCharactersEncodedIfNecessary(const QString& newWord, KateDocument *doc,
                                                                                        const KTextEditor::Range& replacementRange)
{
  const QString replacedString = doc->text(replacementRange);
  int attr = doc->kateTextLine(replacementRange.start().line())->attribute(replacementRange.start().column());
  int p = doc->highlight()->getEncodedCharactersInsertionPolicy(attr);

  if((p == KateDocument::EncodeAlways)
  || (p == KateDocument::EncodeWhenPresent && doc->containsCharacterEncoding(replacementRange))) {
    doc->replaceText(replacementRange, newWord);
    doc->replaceCharactersByEncoding(KTextEditor::Range(replacementRange.start(),
                                                        replacementRange.start() + KTextEditor::Cursor(0, newWord.length())));
  }
  else {
    doc->replaceText(replacementRange, newWord);
  }
}

void KateSpellCheckManager::trimRange(KateDocument *doc, KTextEditor::Range &r)
{
  if(r.isEmpty()) {
    return;
  }
  KTextEditor::Cursor cursor = r.start();
  while(cursor < r.end()) {
    if(doc->lineLength(cursor.line()) > 0
       && !doc->character(cursor).isSpace() && doc->character(cursor).category() != QChar::Other_Control) {
        break;
    }
    cursor.setColumn(cursor.column() + 1);
    if(cursor.column() >= doc->lineLength(cursor.line())) {
      cursor.setPosition(cursor.line() + 1, 0);
    }
  }
  r.start() = cursor;
  if(r.isEmpty()) {
    return;
  }

  cursor = r.end();
  KTextEditor::Cursor prevCursor = cursor;
  // the range cannot be empty now
  do {
    prevCursor = cursor;
    if(cursor.column() <= 0) {
      cursor.setPosition(cursor.line() - 1, doc->lineLength(cursor.line() - 1));
    }
    else {
      cursor.setColumn(cursor.column() - 1);
    }
    if(cursor.column() < doc->lineLength(cursor.line())
       && !doc->character(cursor).isSpace() && doc->character(cursor).category() != QChar::Other_Control) {
        break;
    }
  }
  while(cursor > r.start());
  r.end() = prevCursor;
}

#include "spellcheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
