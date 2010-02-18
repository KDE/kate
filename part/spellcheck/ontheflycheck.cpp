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

#include <QMutex>
#include <QTimer>

#include <ktexteditor/smartrange.h>

#include "kateconfig.h"
#include "kateglobal.h"
#include "katerenderer.h"
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

  setWantsDirectChanges(true);

  m_viewRefreshTimer = new QTimer(this);
  m_viewRefreshTimer->setSingleShot(true);
  connect(m_viewRefreshTimer, SIGNAL(timeout()), this, SLOT(viewRefreshTimeout()));

  connect(document, SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&)),
          this, SLOT(textInserted(KTextEditor::Document*, const KTextEditor::Range&)));
  connect(document, SIGNAL(textRemoved(KTextEditor::Document*, const KTextEditor::Range&)),
          this, SLOT(textRemoved(KTextEditor::Document*, const KTextEditor::Range&)));
  connect(document, SIGNAL(viewCreated(KTextEditor::Document*, KTextEditor::View*)),
          this, SLOT(addView(KTextEditor::Document*, KTextEditor::View*)));
  connect(document, SIGNAL(highlightingModeChanged (KTextEditor::Document*)),
          this, SLOT(updateConfig()));
  connect(document, SIGNAL(respellCheckBlock(KateDocument*, int, int)), 
          this, SLOT(handleRespellCheckBlock(KateDocument*, int, int)));
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
  QMutexLocker smartLock(m_document->smartMutex());
  foreach(const MisspelledItem &item, m_misspelledList) {
    KTextEditor::SmartRange *smartRange = item.first;
    if(smartRange->contains(cursor)) {
      return QPair<KTextEditor::Range, QString>(*smartRange, item.second);
    }
  }
  return QPair<KTextEditor::Range, QString>(KTextEditor::Range::invalid(), QString());
}

QString KateOnTheFlyChecker::dictionaryForMisspelledRange(const KTextEditor::Range& range) const
{
  QMutexLocker smartLock(m_document->smartMutex());
  foreach(const MisspelledItem &item, m_misspelledList) {
    KTextEditor::SmartRange *smartRange = item.first;
    if(*smartRange == range) {
      return item.second;
    }
  }
  return QString();
}

void KateOnTheFlyChecker::clearMisspellingForWord(const QString& word)
{
  QMutexLocker smartLock(m_document->smartMutex());
  MisspelledList misspelledList = m_misspelledList; // make a copy
  foreach(const MisspelledItem &item, misspelledList) {
    KTextEditor::SmartRange *smartRange = item.first;
    if(m_document->text(*smartRange) == word) {
      delete(smartRange);
    }
  }
}

const KateOnTheFlyChecker::SpellCheckItem KateOnTheFlyChecker::invalidSpellCheckQueueItem =
                           SpellCheckItem(NULL, "");

void KateOnTheFlyChecker::handleRespellCheckBlock(KateDocument *kateDocument, int start, int end)
{
  Q_ASSERT(kateDocument == m_document);
  Q_UNUSED(kateDocument);

  ON_THE_FLY_DEBUG << start << end;
  KTextEditor::Range range(start, 0, end, m_document->lineLength(end));
  bool listEmpty = m_modificationList.isEmpty();
  QMutexLocker smartLock(m_document->smartMutex());
  KTextEditor::SmartRange *smartRange = m_document->newSmartRange(range);
  smartRange->addWatcher(this);
  // we don't handle this directly as the highlighting information might not be up-to-date yet
  m_modificationList.push_back(ModificationItem(TEXT_INSERTED, smartRange));
  ON_THE_FLY_DEBUG << "added" << *smartRange;
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

  QMutexLocker smartLock(m_document->smartMutex());
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
      KTextEditor::SmartRange *smartRange = m_document->newSmartRange(visibleIntersection);
      smartRange->addWatcher(this);
      m_modificationList.push_back(ModificationItem(TEXT_INSERTED, smartRange));
      ON_THE_FLY_DEBUG << "added" << *smartRange;
    }
  }

  if(listEmptyAtStart && !m_modificationList.isEmpty()) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

void KateOnTheFlyChecker::handleInsertedText(const KTextEditor::Range &range)
{
  KTextEditor::Range consideredRange = range;
  QMutexLocker smartLock(m_document->smartMutex());
  ON_THE_FLY_DEBUG << m_document << range;

  bool spellCheckInProgress = m_currentlyCheckedItem != invalidSpellCheckQueueItem;

  if(spellCheckInProgress) {
    KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.first;
    if(spellCheckRange->contains(consideredRange)) {
      consideredRange = *spellCheckRange;
      stopCurrentSpellCheck();
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
    }
    else if(consideredRange.contains(*spellCheckRange)) {
      stopCurrentSpellCheck();
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
    }
    else if(consideredRange.overlaps(*spellCheckRange)) {
      consideredRange.expandToRange(*spellCheckRange);
      stopCurrentSpellCheck();
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
    }
    else {
      spellCheckInProgress = false;
    }
  }
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                          i != m_spellCheckQueue.end();) {
    KTextEditor::SmartRange *spellCheckRange = (*i).first;
    if(spellCheckRange->contains(consideredRange)) {
      consideredRange = *spellCheckRange;
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      i = m_spellCheckQueue.erase(i);
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
    }
    else if(consideredRange.contains(*spellCheckRange)) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      i = m_spellCheckQueue.erase(i);
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
    }
    else if(consideredRange.overlaps(*spellCheckRange)) {
      consideredRange.expandToRange(*spellCheckRange);
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      i = m_spellCheckQueue.erase(i);
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
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
  QMutexLocker smartLock(m_document->smartMutex());

  // for performance reasons we only want to schedule spellchecks for ranges that are visible
  foreach(KTextEditor::View *i, m_document->views()) {
    KateView *view = static_cast<KateView*>(i);
    KTextEditor::Range visibleIntersection = documentIntersection.intersect(view->visibleRange());
    if(visibleIntersection.isValid()) { // see above
      // we don't handle this directly as the highlighting information might not be up-to-date yet
      KTextEditor::SmartRange *smartRange = m_document->newSmartRange(visibleIntersection);
      smartRange->addWatcher(this);
      m_modificationList.push_back(ModificationItem(TEXT_REMOVED, smartRange));
      ON_THE_FLY_DEBUG << "added" << *smartRange << view->visibleRange();
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
  QMutexLocker lock(m_document->smartMutex());

  ON_THE_FLY_DEBUG << range;

  QList<KTextEditor::Range> rangesToReCheck;
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                          i != m_spellCheckQueue.end();) {
    KTextEditor::SmartRange *spellCheckRange = (*i).first;
    if(rangesAdjacent(*spellCheckRange, range) || spellCheckRange->contains(range)) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      if(!spellCheckRange->isEmpty()) {
        rangesToReCheck.push_back(*spellCheckRange);
      }
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
      i = m_spellCheckQueue.erase(i);
    }
    else {
      ++i;
    }
  }
  bool spellCheckInProgress = m_currentlyCheckedItem != invalidSpellCheckQueueItem;
  const bool emptyAtStart = m_spellCheckQueue.isEmpty();
  if(spellCheckInProgress) {
    KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.first;
    ON_THE_FLY_DEBUG << *spellCheckRange;
    if(m_document->documentRange().contains(*spellCheckRange)
         && (rangesAdjacent(*spellCheckRange, range) || spellCheckRange->contains(range))
         && !spellCheckRange->isEmpty()) {
      rangesToReCheck.push_back(*spellCheckRange);
      ON_THE_FLY_DEBUG << "added the range " << *spellCheckRange;
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
      stopCurrentSpellCheck();
    }
    else if(spellCheckRange->isEmpty()) {
      spellCheckRange->removeWatcher(this);
      delete(spellCheckRange);
      stopCurrentSpellCheck();
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
  QMutexLocker smartLock(m_document->smartMutex());

  // ensure that 'document' does not occur in the eliminated ranges list
  deleteEliminatedRanges();
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                      i != m_spellCheckQueue.end();) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      KTextEditor::SmartRange *smartRange = (*i).first;
      smartRange->removeWatcher(this);
      delete(smartRange);
      i = m_spellCheckQueue.erase(i);
  }
  if(m_currentlyCheckedItem != invalidSpellCheckQueueItem) {
      KTextEditor::SmartRange *smartRange = m_currentlyCheckedItem.first;
      smartRange->removeWatcher(this);
      delete(smartRange);
  }
  stopCurrentSpellCheck();

  MisspelledList misspelledList = m_misspelledList; // make a copy!
  foreach(const MisspelledItem &i, misspelledList) {
    delete(i.first);
  }
  m_misspelledList.clear();
  clearModificationList();
  if(!m_spellCheckQueue.isEmpty()) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
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

  QMutexLocker lock(m_document->smartMutex());

  KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.first;
  const QString& language = m_currentlyCheckedItem.second;
  ON_THE_FLY_DEBUG << "for the range " << *spellCheckRange;
  // clear all the highlights that are currently present in the range that
  // is supposed to be checked
  const SmartRangeList highlightsList = installedSmartRanges(*spellCheckRange); // make a copy!
  qDeleteAll(highlightsList);

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
            SIGNAL(misspelling(const QString&,int)),
            this,
            SLOT(misspelling(const QString&,int)));
    connect(m_backgroundChecker, SIGNAL(done()), this, SLOT(spellCheckDone()));
  }
  m_backgroundChecker->setSpeller(m_speller);
  m_backgroundChecker->setText(text); // don't call 'start()' after this!
}


/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateOnTheFlyChecker::rangeDeleted(KTextEditor::SmartRange *smartRange)
{
  Q_ASSERT(m_document == smartRange->document());
  ON_THE_FLY_DEBUG << *smartRange << "(" << smartRange << ")";

  for(SmartRangeList::iterator i = m_eliminatedRanges.begin();
      i != m_eliminatedRanges.end();) {
      if((*i) == smartRange) {
        i = m_eliminatedRanges.erase(i);
      }
      else {
        ++i;
      }
  }

  if(removeRangeFromModificationList(smartRange)) {
    return; // range was part of the modification queue, so we don't have
            // to look further for it
  }

  if(removeRangeFromSpellCheckQueue(smartRange)) {
    return; // range was part of the spell check queue, so it cannot have been
            // a misspelled range
  }

  if (m_myranges.contains(smartRange)) {
      m_myranges.removeAll(smartRange);
  }

  m_document->removeHighlightFromDocument(smartRange);
  m_installedSmartRangeList.removeAll(smartRange);

  MisspelledList& misspelledList = m_misspelledList;
  for(MisspelledList::iterator i = misspelledList.begin(); i != misspelledList.end();) {
    if((*i).first == smartRange) {
      i = misspelledList.erase(i);
    }
    else {
      ++i;
    }
  }
}

bool KateOnTheFlyChecker::removeRangeFromCurrentSpellCheck(KTextEditor::SmartRange *range)
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

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
bool KateOnTheFlyChecker::removeRangeFromSpellCheckQueue(KTextEditor::SmartRange *range)
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

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateOnTheFlyChecker::rangeEliminated(KTextEditor::SmartRange *range)
{
  ON_THE_FLY_DEBUG << range->start() << range->end();
  // remove it from all our structures
  range->removeWatcher(this);
  rangeDeleted(range);
  // but only delete it later
  deleteSmartRangeLater(range);
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateOnTheFlyChecker::mouseEnteredRange(KTextEditor::SmartRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  QMutexLocker(kateView->doc()->smartMutex());
  kateView->spellingMenu()->mouseEnteredMisspelledRange(range);
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/

void KateOnTheFlyChecker::mouseExitedRange(KTextEditor::SmartRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  QMutexLocker(kateView->doc()->smartMutex());
  kateView->spellingMenu()->mouseExitedMisspelledRange(range);
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 *
 * It is not enough to use 'caret/Entered/ExitedRange' only as the cursor doesn't move when some
 * text has been selected.
 **/
void KateOnTheFlyChecker::caretEnteredRange(KTextEditor::SmartRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  QMutexLocker(kateView->doc()->smartMutex());
  kateView->spellingMenu()->caretEnteredMisspelledRange(range);
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/

void KateOnTheFlyChecker::caretExitedRange(KTextEditor::SmartRange *range, KTextEditor::View *view)
{
  KateView *kateView = static_cast<KateView*>(view);
  QMutexLocker(kateView->doc()->smartMutex());
  kateView->spellingMenu()->caretExitedMisspelledRange(range);
}

void KateOnTheFlyChecker::deleteSmartRangeLater(KTextEditor::SmartRange *range)
{
  m_eliminatedRanges.push_back(range);
  if(m_eliminatedRanges.size() == 1) { // otherwise there is already a call to '
                                       // 'deleteEliminatedRanges()' scheduled
    QTimer::singleShot(0, this, SLOT(deleteEliminatedRanges()));
  }
}

void KateOnTheFlyChecker::deleteSmartRangesLater(const QList<KTextEditor::SmartRange*>& list)
{
  bool isEmptyAtStart = m_eliminatedRanges.isEmpty();
  m_eliminatedRanges += list;
  if(isEmptyAtStart) { // otherwise there is already a call to '
                                     // 'deleteEliminatedRanges()' scheduled
    QTimer::singleShot(0, this, SLOT(deleteEliminatedRanges()));
  }
}

KTextEditor::Range KateOnTheFlyChecker::findWordBoundaries(const KTextEditor::Cursor& begin,
                                                           const KTextEditor::Cursor& end)
{
  const QRegExp boundaryRegExp("\\b");
  const QRegExp extendedBoundaryRegExp("(\\W|$)");
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
  boundaryStart.setColumn(m_document->computePositionWrtOffsets(decToEncOffsetList,
                                                                qMax(0, text.lastIndexOf(boundaryRegExp))));
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
  boundaryEnd.setColumn(m_document->computePositionWrtOffsets(decToEncOffsetList,
                                                              translatedColumn +
                                                              qMax(0, extendedBoundaryRegExp.indexIn(text))));
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

  QMutexLocker smartLock(m_document->smartMutex());
  KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.first;
  int line = spellCheckRange->start().line();
  int rangeStart = spellCheckRange->start().column();
  int translatedEnd = m_document->computePositionWrtOffsets(m_currentDecToEncOffsetList,
                                                            start + word.length());

  KTextEditor::SmartRange *smartRange =
                            m_document->newSmartRange(KTextEditor::Range(line,
                                                                         rangeStart + translatedStart,
                                                                         line,
                                                                         rangeStart + translatedEnd));
  smartRange->addWatcher(this);
  m_myranges.push_back(smartRange);
  KTextEditor::Attribute *attribute = new KTextEditor::Attribute();
  attribute->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
  QColor lineColor(Qt::red);
  KateView *activeView = dynamic_cast<KateView*>(m_document->activeView());
  if(activeView) {
    lineColor = activeView->renderer()->config()->spellingMistakeLineColor();
  }
  attribute->setUnderlineColor(lineColor);
  smartRange->setAttribute(KTextEditor::Attribute::Ptr(attribute));
  m_misspelledList.push_back(MisspelledItem(smartRange, m_currentlyCheckedItem.second));
  installSmartRange(smartRange);

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
  QMutexLocker smartLock(m_document->smartMutex());
  KTextEditor::SmartRange *smartRange = m_currentlyCheckedItem.first;
  stopCurrentSpellCheck();
  smartRange->removeWatcher(this);
  delete(smartRange);

  if(!m_spellCheckQueue.empty()) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
}

#if 0
void KateOnTheFlyChecker::removeInstalledSmartRanges(KTextEditor::View* view)
{
  ON_THE_FLY_DEBUG;
  KTextEditor::SmartInterface *smartInterface =
                              qobject_cast<KTextEditor::SmartInterface*>(view->document());
  if(!smartInterface) {
    return;
  }
  SmartRangeList& smartRangeList = m_installedSmartRangeMap[document];
  for(SmartRangeList::iterator i = smartRangeList.begin(); i != smartRangeList.end(); ++i) {
    smartInterface->smartMutex()->lock();
    smartInterface->removeHighlightFromView(view, *i);
    smartInterface->smartMutex()->unlock();
  }
}
#endif

QList<KTextEditor::SmartRange*> KateOnTheFlyChecker::installedSmartRanges(const KTextEditor::Range& range)
{
  ON_THE_FLY_DEBUG << range;
  SmartRangeList toReturn;

  m_document->smartMutex()->lock();
  foreach(KTextEditor::SmartRange *smartRange, m_document->documentHighlights()) {
    if (m_myranges.contains(smartRange)) { //JOWENN
      if(smartRange->overlaps(range)) {
        toReturn.push_back(smartRange);
      }
    }
  }
  m_document->smartMutex()->unlock();
  return toReturn;
}

void KateOnTheFlyChecker::installSmartRange(KTextEditor::SmartRange *smartRange)
{
  QMutexLocker smartLock(m_document->smartMutex());
  m_document->addHighlightToDocument(smartRange, true);
  m_installedSmartRangeList.push_back(smartRange);
}

#if 0
void KateOnTheFlyChecker::installSmartRanges(KateDocument* document)
{
  ON_THE_FLY_DEBUG;
  if(!(document->isOnTheFlySpellCheckingEnabled())) {
    return;
  }
 
  KTextEditor::SmartInterface *smartInterface =document;
  if(!smartInterface) {
    return;
  }
  const SmartRangeList& smartRangeList = getSmartRanges(document); 
  for(SmartRangeList::const_iterator i = smartRangeList.begin(); i != smartRangeList.end(); ++i) {
    KTextEditor::SmartRange* smartRange = *i;
      smartInterface->smartMutex()->lock();
      smartInterface->addHighlightToDocument(smartRange);
      smartInterface->smartMutex()->unlock();
      m_installedSmartRangeMap[document].push_back(smartRange);
  }
}
#endif

void KateOnTheFlyChecker::updateConfig()
{
  ON_THE_FLY_DEBUG;
  m_speller.restore(KGlobal::config().data());
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
  updateInstalledSmartRanges(static_cast<KateView*>(view));
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

void KateOnTheFlyChecker::updateInstalledSmartRanges(KateView *view)
{
  Q_ASSERT(m_document == view->document());
  ON_THE_FLY_DEBUG;
  KTextEditor::Range oldDisplayRange = m_displayRangeMap[view];
  QMutexLocker lock(m_document->smartMutex());

  KTextEditor::Range newDisplayRange = view->visibleRange();
  ON_THE_FLY_DEBUG << "new range: " << newDisplayRange;
  ON_THE_FLY_DEBUG << "old range: " << oldDisplayRange;
  foreach(const MisspelledItem &item, m_misspelledList) {
    KTextEditor::SmartRange *smartRange = item.first;
    if(!smartRange->overlaps(newDisplayRange)) {
      bool stillVisible = false;
      foreach(KTextEditor::View *it2, m_document->views()) {
        KateView *view2 = static_cast<KateView*>(it2);
        if(view != view2 && smartRange->overlaps(view2->visibleRange())) {
          stillVisible = true;
          break;
        }
      }
      if(!stillVisible) {
        // only delete them later as 'smartRangeList' will otherwise be modified
        deleteSmartRangeLater(smartRange);
      }
    }
  }
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

#if 0
/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
QList<KTextEditor::SmartRange*> KateOnTheFlyChecker::getSmartRanges()
{
  ON_THE_FLY_DEBUG;
  SmartRangeList toReturn;
  const MisspelledList& misspelledList = m_misspelledMap[document];
  for(MisspelledList::const_iterator i = misspelledList.begin(); i != misspelledList.end(); ++i) {
    KTextEditor::SmartRange* smartRange = (*i).first;
      toReturn.push_back(smartRange);
  }
  return toReturn;
}
#endif

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

    QMutexLocker lock(m_document->smartMutex());

    // clear all the highlights that are currently present in the range that
    // is supposed to be checked, necessary due to highlighting
    const SmartRangeList highlightsList = installedSmartRanges(intersection);
    qDeleteAll(highlightsList);

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
    QMutexLocker lock(kateDocument->smartMutex());
    const KTextEditor::Range range = KTextEditor::Range(line, 0, line, kateDocument->lineLength(line));
    // clear all the highlights that are currently present in the range that
    // is supposed to be checked, necessary due to highlighting

    const SmartRangeList highlightsList = installedSmartRanges(range);
    qDeleteAll(highlightsList);

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

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateOnTheFlyChecker::queueLineSpellCheck(const KTextEditor::Range& range, const QString& dictionary)
{
  ON_THE_FLY_DEBUG << m_document << range;

  Q_ASSERT(range.onSingleLine());

  if(range.isEmpty()) {
    return;
  }

  addToSpellCheckQueue(range, dictionary);
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateOnTheFlyChecker::addToSpellCheckQueue(const KTextEditor::Range& range, const QString& dictionary)
{
  addToSpellCheckQueue(m_document->newSmartRange(range), dictionary);
}

void KateOnTheFlyChecker::addToSpellCheckQueue(KTextEditor::SmartRange *range, const QString& dictionary)
{
  QMutexLocker lock(m_document->smartMutex());
  ON_THE_FLY_DEBUG << m_document << *range << dictionary;

  range->addWatcher(this);

  // if the queue contains a subrange of 'range', we remove that one
  for(QList<SpellCheckItem>::iterator i = m_spellCheckQueue.begin();
                                      i != m_spellCheckQueue.end();) {
      KTextEditor::SmartRange *spellCheckRange = (*i).first;
      if(range->contains(*spellCheckRange)) {
        spellCheckRange->removeWatcher(this);
        delete(spellCheckRange);
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
    updateInstalledSmartRanges(m_refreshView);
  }
  m_refreshView = NULL;
}

void KateOnTheFlyChecker::restartViewRefreshTimer(KateView *view)
{
  if(m_refreshView && view != m_refreshView) { // a new view should be refreshed
    updateInstalledSmartRanges(m_refreshView); // so refresh the old one first
  }
  m_refreshView = view;
  m_viewRefreshTimer->start(100);
}

void KateOnTheFlyChecker::deleteEliminatedRanges()
{
  ON_THE_FLY_DEBUG << "deleting eliminated ranges\n";
  while(!m_eliminatedRanges.isEmpty()) {
    QMutexLocker smartLock(m_document->smartMutex());
    KTextEditor::SmartRange *r = m_eliminatedRanges.takeFirst();
    // the watcher has already been removed
    ON_THE_FLY_DEBUG << r;
    delete r;
  }
}

void KateOnTheFlyChecker::handleModifiedRanges()
{
  foreach(const ModificationItem &item, m_modificationList) {
    QMutexLocker smartLock(m_document->smartMutex());
    KTextEditor::SmartRange *smartRange = item.second;
    KTextEditor::Range range = *smartRange;
    smartRange->removeWatcher(this);
    delete(smartRange);
    if(item.first == TEXT_INSERTED) {
      handleInsertedText(range);
    }
    else {
      handleRemovedText(range);
    }
  }
  m_modificationList.clear();
}

bool KateOnTheFlyChecker::removeRangeFromModificationList(KTextEditor::SmartRange *range)
{
  bool found = false;
  for(ModificationList::iterator i = m_modificationList.begin(); i != m_modificationList.end();) {
    ModificationItem item = *i;
    QMutexLocker smartLock(m_document->smartMutex());
    KTextEditor::SmartRange *smartRange = item.second;
    if(smartRange == range) {
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
    QMutexLocker smartLock(m_document->smartMutex());
    KTextEditor::SmartRange *smartRange = item.second;
    smartRange->removeWatcher(this);
    delete(smartRange);
  }
  m_modificationList.clear();
}

#include "ontheflycheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
