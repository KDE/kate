/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2010 by Michel Ludwig <michel.ludwig@kdemail.net>
 *  Copyright (C) 2009 by Joseph Wenninger <jowenn@kde.org>
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

#include "ontheflycheck.h"

#include <QTimer>

#include "kateconfig.h"
#include "kateglobal.h"
#include "katerenderer.h"
#include "katebuffer.h"
#include "kateview.h"
#include "spellcheck.h"
#include "spellingmenu.h"

#define ON_THE_FLY_DEBUG kDebug(debugArea())

KateOnTheFlyChecker::KateOnTheFlyChecker(KateDocument *document)
: QObject(document),
  m_document(document),
  m_backgroundChecker(NULL),
  m_currentlyCheckedItem(invalidSpellCheckQueueItem),
  m_refreshView(NULL)
{
  ON_THE_FLY_DEBUG << "created";

  m_viewRefreshTimer = new QTimer(this);
  m_viewRefreshTimer->setSingleShot(true);
  connect(m_viewRefreshTimer, SIGNAL(timeout()), this, SLOT(viewRefreshTimeout()));

  connect(document, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
          this, SLOT(textInserted(KTextEditor::Document*,KTextEditor::Range)));
  connect(document, SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
          this, SLOT(textRemoved(KTextEditor::Document*,KTextEditor::Range)));
  connect(document, SIGNAL(viewCreated(KTextEditor::Document*,KTextEditor::View*)),
          this, SLOT(addView(KTextEditor::Document*,KTextEditor::View*)));
  connect(document, SIGNAL(highlightingModeChanged(KTextEditor::Document*)),
          this, SLOT(updateConfig()));
  connect(&document->buffer(), SIGNAL(respellCheckBlock(int,int)),
          this, SLOT(handleRespellCheckBlock(int,int)));

  // load the settings for the speller
  updateConfig();

  foreach(KTextEditor::View* view, document->views()) {
    addView(document, view);
  }
  refreshSpellCheck();
}

KateOnTheFlyChecker::~KateOnTheFlyChecker()
{
  freeDocument();
}

int KateOnTheFlyChecker::debugArea()
{
  static int s_area = KDebug::registerArea("Kate (On-The-Fly Spellcheck)");
  return s_area;
}

QPair<KTextEditor::Range, QString> KateOnTheFlyChecker::getMisspelledItem(const KTextEditor::Cursor &cursor) const
{
  foreach(const MisspelledItem &item, m_misspelledList) {
    KTextEditor::MovingRange *movingRange = item.first;
    if(movingRange->contains(cursor)) {
      return QPair<KTextEditor::Range, QString>(*movingRange, item.second);
    }
  }
  return QPair<KTextEditor::Range, QString>(KTextEditor::Range::invalid(), QString());
}

QString KateOnTheFlyChecker::dictionaryForMisspelledRange(const KTextEditor::Range& range) const
{
  foreach(const MisspelledItem &item, m_misspelledList) {
    KTextEditor::MovingRange *movingRange = item.first;
    if(*movingRange == range) {
      return item.second;
    }
  }
  return QString();
}

void KateOnTheFlyChecker::clearMisspellingForWord(const QString& word)
{
  MisspelledList misspelledList = m_misspelledList; // make a copy
  foreach(const MisspelledItem &item, misspelledList) {
    KTextEditor::MovingRange *movingRange = item.first;
    if(m_document->text(*movingRange) == word) {
      deleteMovingRange(movingRange);
    }
  }
}

const KateOnTheFlyChecker::SpellCheckItem KateOnTheFlyChecker::invalidSpellCheckQueueItem =
                           SpellCheckItem(NULL, "");

void KateOnTheFlyChecker::handleRespellCheckBlock(int start, int end)
{
  ON_THE_FLY_DEBUG << start << end;
  KTextEditor::Range range(start, 0, end, m_document->lineLength(end));
  bool listEmpty = m_modificationList.isEmpty();
  KTextEditor::MovingRange *movingRange = m_document->newMovingRange(range);
  movingRange->setFeedback(this);
  // we don't handle this directly as the highlighting information might not be up-to-date yet
  m_modificationList.push_back(ModificationItem(TEXT_INSERTED, movingRange));
  ON_THE_FLY_DEBUG << "added" << *movingRange;
  if(listEmpty) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

void KateOnTheFlyChecker::textInserted(KTextEditor::Document *document, const KTextEditor::Range &range)
{
  Q_ASSERT(document == m_document);
  Q_UNUSED(document);
  if(!range.isValid()) {
    return;
  }

  bool listEmptyAtStart = m_modificationList.isEmpty();

  // don't consider a range that is not within the document range
  const KTextEditor::Range documentIntersection = m_document->documentRange().intersect(range);
  if(!documentIntersection.isValid()) {
    return;
  }
  // for performance reasons we only want to schedule spellchecks for ranges that are visible
  foreach(KTextEditor::View* i, m_document->views()) {
    KateView *view = static_cast<KateView*>(i);
    KTextEditor::Range visibleIntersection = documentIntersection.intersect(view->visibleRange());
    if(visibleIntersection.isValid()) { // allow empty intersections
      // we don't handle this directly as the highlighting information might not be up-to-date yet
      KTextEditor::MovingRange *movingRange = m_document->newMovingRange(visibleIntersection);
      movingRange->setFeedback(this);
      m_modificationList.push_back(ModificationItem(TEXT_INSERTED, movingRange));
      ON_THE_FLY_DEBUG << "added" << *movingRange;
    }
  }

  if(listEmptyAtStart && !m_modificationList.isEmpty()) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

void KateOnTheFlyChecker::handleInsertedText(const KTextEditor::Range &range)
{
  KTextEditor::Range consideredRange = range;
  ON_THE_FLY_DEBUG << m_document << range;

  bool spellCheckInProgress = m_currentlyCheckedItem != invalidSpellCheckQueueItem;

  if(spellCheckInProgress) {
    KTextEditor::MovingRange *spellCheckRange = m_currentlyCheckedItem.first;
    if(spellCheckRange->contains(consideredRange)) {
      consideredRange = *spellCheckRange;
      stopCurrentSpellCheck();
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else if(consideredRange.contains(*spellCheckRange)) {
      stopCurrentSpellCheck();
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else if(consideredRange.overlaps(*spellCheckRange)) {
      consideredRange.expandToRange(*spellCheckRange);
      stopCurrentSpellCheck();
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else {
      spellCheckInProgress = false;
    }
  }
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                          i != m_spellCheckQueue.end();) {
    KTextEditor::MovingRange *spellCheckRange = (*i).first;
    if(spellCheckRange->contains(consideredRange)) {
      consideredRange = *spellCheckRange;
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      i = m_spellCheckQueue.erase(i);
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else if(consideredRange.contains(*spellCheckRange)) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      i = m_spellCheckQueue.erase(i);
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else if(consideredRange.overlaps(*spellCheckRange)) {
      consideredRange.expandToRange(*spellCheckRange);
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      i = m_spellCheckQueue.erase(i);
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else {
      ++i;
    }
  }
  KTextEditor::Range spellCheckRange = findWordBoundaries(consideredRange.start(),
                                                          consideredRange.end());
  const bool emptyAtStart = m_spellCheckQueue.isEmpty();

  queueSpellCheckVisibleRange(spellCheckRange);

  if(spellCheckInProgress || (emptyAtStart && !m_spellCheckQueue.isEmpty())) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
}

void KateOnTheFlyChecker::textRemoved(KTextEditor::Document *document, const KTextEditor::Range &range)
{
  Q_ASSERT(document == m_document);
  Q_UNUSED(document);
  if(!range.isValid()) {
    return;
  }

  bool listEmptyAtStart = m_modificationList.isEmpty();

  // don't consider a range that is behind the end of the document
  const KTextEditor::Range documentIntersection = m_document->documentRange().intersect(range);
  if(!documentIntersection.isValid()) { // the intersection might however be empty if the last
    return;                             // word has been removed, for example
  }

  // for performance reasons we only want to schedule spellchecks for ranges that are visible
  foreach(KTextEditor::View *i, m_document->views()) {
    KateView *view = static_cast<KateView*>(i);
    KTextEditor::Range visibleIntersection = documentIntersection.intersect(view->visibleRange());
    if(visibleIntersection.isValid()) { // see above
      // we don't handle this directly as the highlighting information might not be up-to-date yet
      KTextEditor::MovingRange *movingRange = m_document->newMovingRange(visibleIntersection);
      movingRange->setFeedback(this);
      m_modificationList.push_back(ModificationItem(TEXT_REMOVED, movingRange));
      ON_THE_FLY_DEBUG << "added" << *movingRange << view->visibleRange();
    }
  }
  if(listEmptyAtStart && !m_modificationList.isEmpty()) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

inline bool rangesAdjacent(const KTextEditor::Range &r1, const KTextEditor::Range &r2)
{
  return (r1.end() == r2.start()) || (r2.end() == r1.start());
}

void KateOnTheFlyChecker::handleRemovedText(const KTextEditor::Range &range)
{

  ON_THE_FLY_DEBUG << range;

  QList<KTextEditor::Range> rangesToReCheck;
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                          i != m_spellCheckQueue.end();) {
    KTextEditor::MovingRange *spellCheckRange = (*i).first;
    if(rangesAdjacent(*spellCheckRange, range) || spellCheckRange->contains(range)) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      if(!spellCheckRange->isEmpty()) {
        rangesToReCheck.push_back(*spellCheckRange);
      }
      deleteMovingRangeQuickly(spellCheckRange);
      i = m_spellCheckQueue.erase(i);
    }
    else {
      ++i;
    }
  }
  bool spellCheckInProgress = m_currentlyCheckedItem != invalidSpellCheckQueueItem;
  const bool emptyAtStart = m_spellCheckQueue.isEmpty();
  if(spellCheckInProgress) {
    KTextEditor::MovingRange *spellCheckRange = m_currentlyCheckedItem.first;
    ON_THE_FLY_DEBUG << *spellCheckRange;
    if(m_document->documentRange().contains(*spellCheckRange)
         && (rangesAdjacent(*spellCheckRange, range) || spellCheckRange->contains(range))
         && !spellCheckRange->isEmpty()) {
      rangesToReCheck.push_back(*spellCheckRange);
      ON_THE_FLY_DEBUG << "added the range " << *spellCheckRange;
      stopCurrentSpellCheck();
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else if(spellCheckRange->isEmpty()) {
      stopCurrentSpellCheck();
      deleteMovingRangeQuickly(spellCheckRange);
    }
    else {
      spellCheckInProgress = false;
    }
  }
  for(QList<KTextEditor::Range>::iterator i = rangesToReCheck.begin(); i != rangesToReCheck.end(); ++i) {
    queueSpellCheckVisibleRange(*i);
  }

  KTextEditor::Range spellCheckRange = findWordBoundaries(range.start(), range.start());
  KTextEditor::Cursor spellCheckEnd = spellCheckRange.end();

  queueSpellCheckVisibleRange(spellCheckRange);

  if(range.numberOfLines() > 0) {
    //FIXME: there is no currently no way of doing this better as we only get notifications for removals of
    //       of single lines, i.e. we don't know here how many lines have been removed in total
    KTextEditor::Cursor nextLineStart(spellCheckEnd.line() + 1, 0);
    const KTextEditor::Cursor documentEnd = m_document->documentEnd();
    if(nextLineStart < documentEnd) {
      KTextEditor::Range rangeBelow = KTextEditor::Range(nextLineStart, documentEnd);

      const QList<KTextEditor::View*>& viewList = m_document->views();
      for(QList<KTextEditor::View*>::const_iterator i = viewList.begin(); i != viewList.end(); ++i) {
        KateView *view = static_cast<KateView*>(*i);
        const KTextEditor::Range visibleRange = view->visibleRange();
        KTextEditor::Range intersection = visibleRange.intersect(rangeBelow);
        if(intersection.isValid()) {
          queueSpellCheckVisibleRange(view, intersection);
        }
      }
    }
  }

  ON_THE_FLY_DEBUG << "finished";
  if(spellCheckInProgress || (emptyAtStart && !m_spellCheckQueue.isEmpty())) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
}

void KateOnTheFlyChecker::freeDocument()
{
  ON_THE_FLY_DEBUG;

  // empty the spell check queue
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                      i != m_spellCheckQueue.end();) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      KTextEditor::MovingRange *movingRange = (*i).first;
      deleteMovingRangeQuickly(movingRange);
      i = m_spellCheckQueue.erase(i);
  }
  if(m_currentlyCheckedItem != invalidSpellCheckQueueItem) {
      KTextEditor::MovingRange *movingRange = m_currentlyCheckedItem.first;
      deleteMovingRangeQuickly(movingRange);
  }
  stopCurrentSpellCheck();
  
  MisspelledList misspelledList = m_misspelledList; // make a copy!
  foreach(const MisspelledItem &i, misspelledList) {
    deleteMovingRange(i.first);
  }
  m_misspelledList.clear();
  clearModificationList();
}

void KateOnTheFlyChecker::performSpellCheck()
{
  if(m_currentlyCheckedItem != invalidSpellCheckQueueItem) {
    ON_THE_FLY_DEBUG << "exited as a check is currently in progress";
    return;
  }
  if(m_spellCheckQueue.isEmpty()) {
    ON_THE_FLY_DEBUG << "exited as there is nothing to do";
    return;
  }
  m_currentlyCheckedItem = m_spellCheckQueue.takeFirst();

  KTextEditor::MovingRange *spellCheckRange = m_currentlyCheckedItem.first;
  const QString& language = m_currentlyCheckedItem.second;
  ON_THE_FLY_DEBUG << "for the range " << *spellCheckRange;
  // clear all the highlights that are currently present in the range that
  // is supposed to be checked
  const MovingRangeList highlightsList = installedMovingRanges(*spellCheckRange); // make a copy!
  deleteMovingRanges(highlightsList);

  m_currentDecToEncOffsetList.clear();
  KateDocument::OffsetList encToDecOffsetList;
  QString text = m_document->decodeCharacters(*spellCheckRange,
                                              m_currentDecToEncOffsetList,
                                              encToDecOffsetList);
  ON_THE_FLY_DEBUG << "next spell checking" << text;
  if(text.isEmpty()) { // passing an empty string to Sonnet can lead to a bad allocation exception
    spellCheckDone();  // (bug 225867)
    return;
  }
  if(m_speller.language() != language) {
    m_speller.setLanguage(language);
  }
  if(!m_backgroundChecker) {
    m_backgroundChecker = new Sonnet::BackgroundChecker(m_speller, this);
    connect(m_backgroundChecker,
            SIGNAL(misspelling(QString,int)),
            this,
            SLOT(misspelling(QString,int)));
    connect(m_backgroundChecker, SIGNAL(done()), this, SLOT(spellCheckDone()));

    m_backgroundChecker->restore(KGlobal::config().data());
  }
  m_backgroundChecker->setSpeller(m_speller);
  m_backgroundChecker->setText(text); // don't call 'start()' after this!
}

void KateOnTheFlyChecker::removeRangeFromEverything(KTextEditor::MovingRange *movingRange)
{
  Q_ASSERT(m_document == movingRange->document());
  ON_THE_FLY_DEBUG << *movingRange << "(" << movingRange << ")";

  if(removeRangeFromModificationList(movingRange)) {
    return; // range was part of the modification queue, so we don't have
            // to look further for it
  }

  if(removeRangeFromSpellCheckQueue(movingRange)) {
    return; // range was part of the spell check queue, so it cannot have been
            // a misspelled range
  }

  for(MisspelledList::iterator i = m_misspelledList.begin(); i != m_misspelledList.end();) {
    if((*i).first == movingRange) {
      i = m_misspelledList.erase(i);
    }
    else {
      ++i;
    }
  }
}

bool KateOnTheFlyChecker::removeRangeFromCurrentSpellCheck(KTextEditor::MovingRange *range)
{
  if(m_currentlyCheckedItem != invalidSpellCheckQueueItem
    && m_currentlyCheckedItem.first == range) {
    stopCurrentSpellCheck();
    return true;
  }
  return false;
}

void KateOnTheFlyChecker::stopCurrentSpellCheck()
{
  m_currentDecToEncOffsetList.clear();
  m_currentlyCheckedItem = invalidSpellCheckQueueItem;
  if(m_backgroundChecker) {
    m_backgroundChecker->stop();
  }
}

bool KateOnTheFlyChecker::removeRangeFromSpellCheckQueue(KTextEditor::MovingRange *range)
{
  if(removeRangeFromCurrentSpellCheck(range)) {
    if(!m_spellCheckQueue.isEmpty()) {
      QTimer::singleShot(0, this, SLOT(performSpellCheck()));
    }
    return true;
  }
  bool found = false;
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                      i != m_spellCheckQueue.end();) {
    if((*i).first == range) {
      i = m_spellCheckQueue.erase(i);
      found = true;
    }
    else {
      ++i;
    }
  }
  return found;
}

void KateOnTheFlyChecker::rangeEmpty(KTextEditor::MovingRange *range)
{
  ON_THE_FLY_DEBUG << range->start() << range->end() << "(" << range << ")";
  deleteMovingRange (range);
}

void KateOnTheFlyChecker::rangeInvalid (KTextEditor::MovingRange* range)
{
  ON_THE_FLY_DEBUG << range->start() << range->end() << "(" << range << ")";
  deleteMovingRange (range);
}

void KateOnTheFlyChecker::mouseEnteredRange(KTextEditor::MovingRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  kateView->spellingMenu()->mouseEnteredMisspelledRange(range);
}

void KateOnTheFlyChecker::mouseExitedRange(KTextEditor::MovingRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  kateView->spellingMenu()->mouseExitedMisspelledRange(range);
}

/**
 * It is not enough to use 'caret/Entered/ExitedRange' only as the cursor doesn't move when some
 * text has been selected.
 **/
void KateOnTheFlyChecker::caretEnteredRange(KTextEditor::MovingRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  kateView->spellingMenu()->caretEnteredMisspelledRange(range);
}

void KateOnTheFlyChecker::caretExitedRange(KTextEditor::MovingRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  kateView->spellingMenu()->caretExitedMisspelledRange(range);
}

void KateOnTheFlyChecker::deleteMovingRange(KTextEditor::MovingRange *range)
{
  ON_THE_FLY_DEBUG << range;
  // remove it from all our structures
  removeRangeFromEverything(range);
  range->setFeedback(NULL);
  foreach(KTextEditor::View *view, m_document->views()) {
    static_cast<KateView*>(view)->spellingMenu()->rangeDeleted(range);
  }
  delete(range);
}

void KateOnTheFlyChecker::deleteMovingRanges(const QList<KTextEditor::MovingRange*>& list)
{
  foreach(KTextEditor::MovingRange *r, list) {
    deleteMovingRange(r);
  }
}

KTextEditor::Range KateOnTheFlyChecker::findWordBoundaries(const KTextEditor::Cursor& begin,
                                                           const KTextEditor::Cursor& end)
{
  // FIXME: QTextBoundaryFinder should be ideally used for this, but it is currently
  //        still broken in Qt
  const QRegExp boundaryRegExp("\\b");
  const QRegExp boundaryQuoteRegExp("\\b\\w+'\\w*$");  // handle spell checking of "isn't", "doesn't", etc.
  const QRegExp extendedBoundaryRegExp("(\\W|$)");
  const QRegExp extendedBoundaryQuoteRegExp("^\\w*'\\w+\\b"); // see above
  KateDocument::OffsetList decToEncOffsetList, encToDecOffsetList;
  const int startLine = begin.line();
  const int startColumn = begin.column();
  KTextEditor::Cursor boundaryStart, boundaryEnd;
  // first we take care of the start position
  const KTextEditor::Range startLineRange(startLine, 0, startLine, m_document->lineLength(startLine));
  QString decodedLineText = m_document->decodeCharacters(startLineRange,
                                                         decToEncOffsetList,
                                                         encToDecOffsetList);
  int translatedColumn = m_document->computePositionWrtOffsets(encToDecOffsetList,
                                                               startColumn);
  QString text = decodedLineText.mid(0, translatedColumn);
  boundaryStart.setLine(startLine);
  int match = text.lastIndexOf(boundaryQuoteRegExp);
  if(match < 0) {
    match = text.lastIndexOf(boundaryRegExp);
  }
  boundaryStart.setColumn(m_document->computePositionWrtOffsets(decToEncOffsetList, qMax(0, match)));
  // and now the end position
  const int endLine = end.line();
  const int endColumn = end.column();
  if(endLine != startLine) {
    decToEncOffsetList.clear();
    encToDecOffsetList.clear();
    const KTextEditor::Range endLineRange(endLine, 0, endLine, m_document->lineLength(endLine));
    decodedLineText = m_document->decodeCharacters(endLineRange,
                                                   decToEncOffsetList,
                                                   encToDecOffsetList);
  }
  translatedColumn = m_document->computePositionWrtOffsets(encToDecOffsetList,
                                                           endColumn);
  text = decodedLineText.mid(translatedColumn);
  boundaryEnd.setLine(endLine);
  match = extendedBoundaryQuoteRegExp.indexIn(text);
  if(match == 0) {
    match = extendedBoundaryQuoteRegExp.matchedLength();
  }
  else {
    match = extendedBoundaryRegExp.indexIn(text);
  }
  boundaryEnd.setColumn(m_document->computePositionWrtOffsets(decToEncOffsetList,
                                                              translatedColumn + qMax(0, match)));
  return KTextEditor::Range(boundaryStart, boundaryEnd);
}

void KateOnTheFlyChecker::misspelling(const QString &word, int start)
{
  if(m_currentlyCheckedItem == invalidSpellCheckQueueItem) {
    ON_THE_FLY_DEBUG << "exited as no spell check is taking place";
    return;
  }
  int translatedStart = m_document->computePositionWrtOffsets(m_currentDecToEncOffsetList,
                                                              start);
//   ON_THE_FLY_DEBUG << "misspelled " << word
//                                     << " at line "
//                                     << *m_currentlyCheckedItem.first
//                                     << " column " << start;

  KTextEditor::MovingRange *spellCheckRange = m_currentlyCheckedItem.first;
  int line = spellCheckRange->start().line();
  int rangeStart = spellCheckRange->start().column();
  int translatedEnd = m_document->computePositionWrtOffsets(m_currentDecToEncOffsetList,
                                                            start + word.length());

  KTextEditor::MovingRange *movingRange =
                            m_document->newMovingRange(KTextEditor::Range(line,
                                                                         rangeStart + translatedStart,
                                                                         line,
                                                                         rangeStart + translatedEnd));
  movingRange->setFeedback(this);
  KTextEditor::Attribute *attribute = new KTextEditor::Attribute();
  attribute->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
  attribute->setUnderlineColor(KateRendererConfig::global()->spellingMistakeLineColor());

  // don't print this range
  movingRange->setAttributeOnlyForViews (true);

  movingRange->setAttribute(KTextEditor::Attribute::Ptr(attribute));
  m_misspelledList.push_back(MisspelledItem(movingRange, m_currentlyCheckedItem.second));

  if(m_backgroundChecker) {
    m_backgroundChecker->continueChecking();
  }
}

void KateOnTheFlyChecker::spellCheckDone()
{
  ON_THE_FLY_DEBUG << "on-the-fly spell check done, queue length " << m_spellCheckQueue.size();
  if(m_currentlyCheckedItem == invalidSpellCheckQueueItem) {
    return;
  }
  KTextEditor::MovingRange *movingRange = m_currentlyCheckedItem.first;
  stopCurrentSpellCheck();
  deleteMovingRangeQuickly(movingRange);

  if(!m_spellCheckQueue.empty()) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
}

QList<KTextEditor::MovingRange*> KateOnTheFlyChecker::installedMovingRanges(const KTextEditor::Range& range)
{
  ON_THE_FLY_DEBUG << range;
  MovingRangeList toReturn;

  for(QList<SpellCheckItem>::iterator i = m_misspelledList.begin();
                                      i != m_misspelledList.end(); ++i) {
    KTextEditor::MovingRange *movingRange = (*i).first;
    if(movingRange->overlaps(range)) {
      toReturn.push_back(movingRange);
    }
  }
  return toReturn;
}

void KateOnTheFlyChecker::updateConfig()
{
  ON_THE_FLY_DEBUG;
  m_speller.restore(KGlobal::config().data());

  if(m_backgroundChecker) {
    m_backgroundChecker->restore(KGlobal::config().data());
  }
}

void KateOnTheFlyChecker::refreshSpellCheck(const KTextEditor::Range &range)
{
  if(range.isValid()) {
    textInserted(m_document, range);
  }
  else {
    freeDocument();
    textInserted(m_document, m_document->documentRange());
  }
}

void KateOnTheFlyChecker::addView(KTextEditor::Document *document, KTextEditor::View *view)
{
  Q_ASSERT(document == m_document);
  Q_UNUSED(document);
  ON_THE_FLY_DEBUG;
  connect(view, SIGNAL(destroyed(QObject*)), this, SLOT(viewDestroyed(QObject*)));
  connect(view, SIGNAL(displayRangeChanged(KateView*)), this, SLOT(restartViewRefreshTimer(KateView*)));
  updateInstalledMovingRanges(static_cast<KateView*>(view));
}

void KateOnTheFlyChecker::viewDestroyed(QObject* obj)
{
  ON_THE_FLY_DEBUG;
  KTextEditor::View *view = static_cast<KTextEditor::View*>(obj);
  m_displayRangeMap.remove(view);
}

void KateOnTheFlyChecker::removeView(KTextEditor::View *view)
{
  ON_THE_FLY_DEBUG;
  m_displayRangeMap.remove(view);
}

void KateOnTheFlyChecker::updateInstalledMovingRanges(KateView *view)
{
  Q_ASSERT(m_document == view->document());
  ON_THE_FLY_DEBUG;
  KTextEditor::Range oldDisplayRange = m_displayRangeMap[view];

  KTextEditor::Range newDisplayRange = view->visibleRange();
  ON_THE_FLY_DEBUG << "new range: " << newDisplayRange;
  ON_THE_FLY_DEBUG << "old range: " << oldDisplayRange;
  QList<KTextEditor::MovingRange*> toDelete;
  foreach(const MisspelledItem &item, m_misspelledList) {
    KTextEditor::MovingRange *movingRange = item.first;
    if(!movingRange->overlaps(newDisplayRange)) {
      bool stillVisible = false;
      foreach(KTextEditor::View *it2, m_document->views()) {
        KateView *view2 = static_cast<KateView*>(it2);
        if(view != view2 && movingRange->overlaps(view2->visibleRange())) {
          stillVisible = true;
          break;
        }
      }
      if(!stillVisible) {
        toDelete.push_back(movingRange);
      }
    }
  }
  deleteMovingRanges (toDelete);
  m_displayRangeMap[view] = newDisplayRange;
  if(oldDisplayRange.isValid()) {
    bool emptyAtStart = m_spellCheckQueue.empty();
    for(int line = newDisplayRange.end().line(); line >= newDisplayRange.start().line(); --line) {
      if(!oldDisplayRange.containsLine(line)) {
        bool visible = false;
        foreach(KTextEditor::View *it2, m_document->views()) {
          KateView *view2 = static_cast<KateView*>(it2);
          if(view != view2 && view2->visibleRange().containsLine(line)) {
            visible = true;
            break;
          }
        }
        if(!visible) {
          queueLineSpellCheck(m_document, line);
        }
      }
    }
    if(emptyAtStart && !m_spellCheckQueue.isEmpty()) {
      QTimer::singleShot(0, this, SLOT(performSpellCheck()));
    }
  }
}

void KateOnTheFlyChecker::queueSpellCheckVisibleRange(const KTextEditor::Range& range)
{
  const QList<KTextEditor::View*>& viewList = m_document->views();
  for(QList<KTextEditor::View*>::const_iterator i = viewList.begin(); i != viewList.end(); ++i) {
    queueSpellCheckVisibleRange(static_cast<KateView*>(*i), range);
  }
}

void KateOnTheFlyChecker::queueSpellCheckVisibleRange(KateView *view, const KTextEditor::Range& range)
{
    Q_ASSERT(m_document == view->doc());
    KTextEditor::Range visibleRange = view->visibleRange();
    KTextEditor::Range intersection = visibleRange.intersect(range);
    if(intersection.isEmpty()) {
      return;
    }

    // clear all the highlights that are currently present in the range that
    // is supposed to be checked, necessary due to highlighting
    const MovingRangeList highlightsList = installedMovingRanges(intersection);
    deleteMovingRanges(highlightsList);

    QList<QPair<KTextEditor::Range, QString> > spellCheckRanges
                         = KateGlobal::self()->spellCheckManager()->spellCheckRanges(m_document,
                                                                                     intersection,
                                                                                     true);
    //we queue them up in reverse
    QListIterator<QPair<KTextEditor::Range, QString> > i(spellCheckRanges);
    i.toBack();
    while(i.hasPrevious()) {
      QPair<KTextEditor::Range, QString> p = i.previous();
      queueLineSpellCheck(p.first, p.second);
    }
}

void KateOnTheFlyChecker::queueLineSpellCheck(KateDocument *kateDocument, int line)
{
    const KTextEditor::Range range = KTextEditor::Range(line, 0, line, kateDocument->lineLength(line));
    // clear all the highlights that are currently present in the range that
    // is supposed to be checked, necessary due to highlighting

    const MovingRangeList highlightsList = installedMovingRanges(range);
    deleteMovingRanges(highlightsList);

    QList<QPair<KTextEditor::Range, QString> > spellCheckRanges
                             = KateGlobal::self()->spellCheckManager()->spellCheckRanges(kateDocument,
                                                                                         range,
                                                                                         true);
    //we queue them up in reverse
    QListIterator<QPair<KTextEditor::Range, QString> > i(spellCheckRanges);
    i.toBack();
    while(i.hasPrevious()) {
      QPair<KTextEditor::Range, QString> p = i.previous();
      queueLineSpellCheck(p.first, p.second);
    }
}

void KateOnTheFlyChecker::queueLineSpellCheck(const KTextEditor::Range& range, const QString& dictionary)
{
  ON_THE_FLY_DEBUG << m_document << range;

  Q_ASSERT(range.onSingleLine());

  if(range.isEmpty()) {
    return;
  }

  addToSpellCheckQueue(range, dictionary);
}

void KateOnTheFlyChecker::addToSpellCheckQueue(const KTextEditor::Range& range, const QString& dictionary)
{
  addToSpellCheckQueue(m_document->newMovingRange(range), dictionary);
}

void KateOnTheFlyChecker::addToSpellCheckQueue(KTextEditor::MovingRange *range, const QString& dictionary)
{
  ON_THE_FLY_DEBUG << m_document << *range << dictionary;

  range->setFeedback(this);

  // if the queue contains a subrange of 'range', we remove that one
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                      i != m_spellCheckQueue.end();) {
      KTextEditor::MovingRange *spellCheckRange = (*i).first;
      if(range->contains(*spellCheckRange)) {
        deleteMovingRangeQuickly(spellCheckRange);
        i = m_spellCheckQueue.erase(i);
      }
      else {
        ++i;
      }
  }
  // leave 'push_front' here as it is a LIFO queue, i.e. a stack
  m_spellCheckQueue.push_front(SpellCheckItem(range, dictionary));
  ON_THE_FLY_DEBUG << "added"
                   << *range << dictionary
                   << "to the queue, which has a length of" << m_spellCheckQueue.size();
}

void KateOnTheFlyChecker::viewRefreshTimeout()
{
  if(m_refreshView) {
    updateInstalledMovingRanges(m_refreshView);
  }
  m_refreshView = NULL;
}

void KateOnTheFlyChecker::restartViewRefreshTimer(KateView *view)
{
  if(m_refreshView && view != m_refreshView) { // a new view should be refreshed
    updateInstalledMovingRanges(m_refreshView); // so refresh the old one first
  }
  m_refreshView = view;
  m_viewRefreshTimer->start(100);
}

void KateOnTheFlyChecker::deleteMovingRangeQuickly(KTextEditor::MovingRange *range)
{
  range->setFeedback(NULL);
  foreach(KTextEditor::View *view, m_document->views()) {
    static_cast<KateView*>(view)->spellingMenu()->rangeDeleted(range);
  }
  delete(range);
}

void KateOnTheFlyChecker::handleModifiedRanges()
{
  foreach(const ModificationItem &item, m_modificationList) {
    KTextEditor::MovingRange *movingRange = item.second;
    KTextEditor::Range range = *movingRange;
    deleteMovingRangeQuickly(movingRange);
    if(item.first == TEXT_INSERTED) {
      handleInsertedText(range);
    }
    else {
      handleRemovedText(range);
    }
  }
  m_modificationList.clear();
}

bool KateOnTheFlyChecker::removeRangeFromModificationList(KTextEditor::MovingRange *range)
{
  bool found = false;
  for(ModificationList::iterator i = m_modificationList.begin(); i != m_modificationList.end();) {
    ModificationItem item = *i;
    KTextEditor::MovingRange *movingRange = item.second;
    if(movingRange == range) {
      found = true;
      i = m_modificationList.erase(i);
    }
    else {
      ++i;
    }
  }
  return found;
}

void KateOnTheFlyChecker::clearModificationList()
{
  foreach(const ModificationItem &item, m_modificationList) {
    KTextEditor::MovingRange *movingRange = item.second;
    deleteMovingRangeQuickly(movingRange);
  }
  m_modificationList.clear();
}

#include "ontheflycheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
