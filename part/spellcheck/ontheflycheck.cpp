/* 
   Copyright (C) 2008-2009 by Michel Ludwig (michel.ludwig@kdemail.net)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/* If ever threads should be used again, thread communication and
 * synchronization ought to be done with blocking queued signal connections.
 */

#include "ontheflycheck.h"

#include <QMutex>
#include <QHash>
#include <QTimer>
#include <QThread>

#include <ktexteditor/smartinterface.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/view.h>
#include <sonnet/speller.h>

#include "katehighlight.h"
#include "katetextline.h"

#define ON_THE_FLY_DEBUG kDebug(debugArea())

KateOnTheFlyChecker::KateOnTheFlyChecker(QObject *parent)
: QObject(parent),
  m_currentlyCheckedItem(invalidSpellCheckQueueItem),
  m_enabled(false),
  m_refreshView(NULL)
{
  ON_THE_FLY_DEBUG << "created";

  setWantsDirectChanges(true);

  m_backgroundChecker = new Sonnet::BackgroundChecker(Sonnet::Speller(), this);
  connect(m_backgroundChecker,
          SIGNAL(misspelling(const QString&,int)),
          this,
          SLOT(misspelling(const QString&,int)));
  connect(m_backgroundChecker, SIGNAL(done()), this, SLOT(spellCheckDone()));
  m_viewRefreshTimer = new QTimer(this);
  m_viewRefreshTimer->setSingleShot(true);
  connect(m_viewRefreshTimer, SIGNAL(timeout()), this, SLOT(viewRefreshTimeout()));
}

KateOnTheFlyChecker::~KateOnTheFlyChecker()
{
  ON_THE_FLY_DEBUG << "KateOnTheFlyChecker::~KateOnTheFlyChecker()";
}

int KateOnTheFlyChecker::debugArea()
{
  static int s_area = KDebug::registerArea("Kate (On-The-Fly Spellcheck)");
  return s_area;
}

const KateOnTheFlyChecker::SpellCheckQueueItem KateOnTheFlyChecker::invalidSpellCheckQueueItem =
                           SpellCheckQueueItem(NULL, SpellCheckItem(NULL, ""));

void KateOnTheFlyChecker::setEnabled(bool b)
{
  m_enabled = b;
  if(b) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
  else {
    m_currentlyCheckedItem = invalidSpellCheckQueueItem;
    m_spellCheckQueue.clear();
  }
}

void KateOnTheFlyChecker::handleRespellCheckBlock(KateDocument *document, int start, int end)
{
  ON_THE_FLY_DEBUG << m_enabled;
  if (!(static_cast<KateDocument*>(document)->isOnTheFlySpellCheckingEnabled())) return;
  KTextEditor::Range range(start,0,end+1,0);
  bool listEmpty = m_modificationList.isEmpty();
  // we don't handle this directly as the highlighting information might not be up-to-date yet
  m_modificationList.push_back(ModificationItem(TEXT_INSERTED, DocumentRangePair(document, range)));
  if(listEmpty) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

void KateOnTheFlyChecker::textInserted(KTextEditor::Document *document, const KTextEditor::Range &range)
{
  ON_THE_FLY_DEBUG<<m_enabled<<endl;
  if (!(static_cast<KateDocument*>(document)->isOnTheFlySpellCheckingEnabled())) return;
  bool listEmpty = m_modificationList.isEmpty();
  // we don't handle this directly as the highlighting information might not be up-to-date yet
  m_modificationList.push_back(ModificationItem(TEXT_INSERTED, DocumentRangePair(document, range)));
  if(listEmpty) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

void KateOnTheFlyChecker::handleInsertedText(KTextEditor::Document *document, const KTextEditor::Range &range)
{
  ON_THE_FLY_DEBUG << document << range;
  if(!(static_cast<KateDocument*>(document)->isOnTheFlySpellCheckingEnabled())) {
    ON_THE_FLY_DEBUG << "leaving as on-the-fly checking is disabled "
                  << QThread::currentThreadId();
    return;
  }
  KateDocument *kateDocument = static_cast<KateDocument*>(document); 
  KTextEditor::Range spellCheckRange = KTextEditor::Range(findBeginningOfWord(document, range.start(), true),
                                                          findBeginningOfWord(document, range.end(), false));
  const bool emptyAtStart = m_spellCheckQueue.isEmpty();

  queueSpellCheckVisibleRange(kateDocument, spellCheckRange);
  
  if(emptyAtStart && !m_spellCheckQueue.isEmpty()) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }

// this seems to be necessary as otherwise the highlighting below the inserted lines is gone
//   updateInstalledSmartRanges(static_cast<KateDocument*>(document));
}

void KateOnTheFlyChecker::textRemoved(KTextEditor::Document *document, const KTextEditor::Range &range)
{
  if (!(static_cast<KateDocument*>(document)->isOnTheFlySpellCheckingEnabled())) return;
  bool listEmpty = m_modificationList.isEmpty();
  // we don't handle this directly as the highlighting information might not be up-to-date yet
  m_modificationList.push_back(ModificationItem(TEXT_REMOVED, DocumentRangePair(document, range)));
  if(listEmpty) {
    QTimer::singleShot(0, this, SLOT(handleModifiedRanges()));
  }
}

void KateOnTheFlyChecker::handleRemovedText(KTextEditor::Document *document, const KTextEditor::Range &range)
{
  ON_THE_FLY_DEBUG << document << range;
  if (!(static_cast<KateDocument*>(document)->isOnTheFlySpellCheckingEnabled())) {
    ON_THE_FLY_DEBUG << "leaving as on-the-fly checking is disabled "
                     << QThread::currentThreadId();
    return;
  }
  KateDocument *kateDocument = static_cast<KateDocument*>(document);
  for(QList<SpellCheckQueueItem>::iterator i = m_spellCheckQueue.begin();
                                          i != m_spellCheckQueue.end();) {
    KTextEditor::SmartRange *spellCheckRange = (*i).second.first;
    KTextEditor::Document *lineDocument = (*i).first;
    if(document == lineDocument && spellCheckRange->overlaps(range)) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      delete spellCheckRange;
      i = m_spellCheckQueue.erase(i);
    }
    else {
      ++i;
    }
  }
  const bool spellCheckInProgress = m_currentlyCheckedItem != invalidSpellCheckQueueItem;
  const bool emptyAtStart = m_spellCheckQueue.isEmpty();
  if(spellCheckInProgress) {
    KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.second.first;
    KTextEditor::Document *lineDocument = m_currentlyCheckedItem.first;
    if(document != lineDocument) {
      m_spellCheckQueue.push_front(m_currentlyCheckedItem);
    }
    else if(document->documentRange().contains(*spellCheckRange) && !spellCheckRange->overlaps(range)
                                                                 && !spellCheckRange->isEmpty()) {
      m_spellCheckQueue.push_front(m_currentlyCheckedItem);
      ON_THE_FLY_DEBUG << "added the range " << *spellCheckRange;
    }
    else {
      delete(spellCheckRange);
    }
    m_currentlyCheckedItem = invalidSpellCheckQueueItem;
  }
  KTextEditor::Range spellCheckRange = KTextEditor::Range(findBeginningOfWord(document, range.start(), true),
                                                          findBeginningOfWord(document, range.start(), false));
  queueSpellCheckVisibleRange(kateDocument, spellCheckRange);

  ON_THE_FLY_DEBUG << "exited";
  if(spellCheckInProgress || (emptyAtStart && !m_spellCheckQueue.isEmpty())) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }

  // update as otherwise the highlighting below the removed lines is not shown
  if(range.numberOfLines() > 1) {
    updateInstalledSmartRanges(static_cast<KateDocument*>(document));
  }
}

void KateOnTheFlyChecker::freeDocument(KTextEditor::Document *document)
{
  ON_THE_FLY_DEBUG;

  for(QList<SpellCheckQueueItem>::iterator i = m_spellCheckQueue.begin();
                                           i != m_spellCheckQueue.end();) {
    KTextEditor::Document *lineDocument = (*i).first;
    if(document == lineDocument) {
      ON_THE_FLY_DEBUG << "erasing range " << *i;
      delete((*i).second.first);
      i = m_spellCheckQueue.erase(i);
    }
    else {
      ++i;
    }
  }
  if(m_currentlyCheckedItem != invalidSpellCheckQueueItem) {
    KTextEditor::Document *lineDocument = m_currentlyCheckedItem.first;
    if(document != lineDocument) {
      m_spellCheckQueue.push_front(m_currentlyCheckedItem);
    }
    else {
      delete(m_currentlyCheckedItem.second.first);
    }
  }
  m_currentlyCheckedItem = invalidSpellCheckQueueItem;

  m_misspelledMap.remove(document);
  const MisspelledList& misspelledList = m_misspelledMap[document];
  for(MisspelledList::const_iterator i = misspelledList.begin(); i != misspelledList.end(); ++i) {
    delete((*i).first);
  }
  ON_THE_FLY_DEBUG << "exited";
  removeDocumentFromModificationList(document);
  QTimer::singleShot(0, this, SLOT(performSpellCheck()));
}

void KateOnTheFlyChecker::performSpellCheck()
{
  ON_THE_FLY_DEBUG;
  /*if(!m_enabled) {
    ON_THE_FLY_DEBUG << "leaving as on-the-fly checking is disabled "
                  << QThread::currentThreadId();
    return;
  }*/
  if(m_currentlyCheckedItem != invalidSpellCheckQueueItem) {
    ON_THE_FLY_DEBUG << "exited as a check is currenly in progress";
    return;
  }
  if(m_spellCheckQueue.isEmpty()) {
    ON_THE_FLY_DEBUG << "exited as there is nothing to do";
    return;
  }
  m_currentlyCheckedItem = m_spellCheckQueue.takeFirst();

  KateDocument *document = static_cast<KateDocument*>(m_currentlyCheckedItem.first);
  KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.second.first;
  const QString& language = m_currentlyCheckedItem.second.second;
  ON_THE_FLY_DEBUG << "for the range " << *spellCheckRange;
  // clear all the highlights that are currently present in the range that
  // is supposed to be checked
  const SmartRangeList highlightsList = installedSmartRangesInDocument(document, *spellCheckRange); // make a copy!
  
  
  qDeleteAll(highlightsList);



  QString text = document->text(*spellCheckRange);
  ON_THE_FLY_DEBUG << "next spell checking line " << text;
  m_speller.setLanguage(language);
  m_backgroundChecker->setSpeller(m_speller);
  m_backgroundChecker->setText(text); // don't call 'start()' after this!
  ON_THE_FLY_DEBUG << "exited";
}

void KateOnTheFlyChecker::rangeDeleted(KTextEditor::SmartRange *smartRange)
{
  ON_THE_FLY_DEBUG << smartRange;

  if (m_eliminatedRanges.contains(smartRange)) {
      m_eliminatedRanges.removeAll(smartRange);
  }
  
  if (m_myranges.contains(smartRange)) {
      m_myranges.removeAll(smartRange);    
  }
  
  KTextEditor::Document *document = smartRange->document();
  KTextEditor::SmartInterface *smartInterface =
                                qobject_cast<KTextEditor::SmartInterface*>(document);
  if(!smartInterface) {
    return;
  }
  
  
  
    smartInterface->smartMutex()->lock();
    smartInterface->removeHighlightFromDocument(smartRange);
    smartInterface->smartMutex()->unlock();
    m_installedSmartRangeMap[document].removeAll(smartRange);

  MisspelledList& misspelledList = m_misspelledMap[document];
  for(MisspelledList::iterator i = misspelledList.begin(); i != misspelledList.end();) {
    if((*i).first == smartRange) {
      i = misspelledList.erase(i);
    }
    else {
      ++i;
    }
  }
}

void KateOnTheFlyChecker::rangeEliminated(KTextEditor::SmartRange *range)
{
  ON_THE_FLY_DEBUG << range->start() << range->end();
  m_eliminatedRanges.push_back(range);
  if(m_eliminatedRanges.size() == 1) { // otherwise there is already a call to '
                                       // 'deleteEliminatedRanges()' scheduled
    QTimer::singleShot(0, this, SLOT(deleteEliminatedRanges()));
  }
}

KTextEditor::Cursor KateOnTheFlyChecker::findBeginningOfWord(KTextEditor::Document* document,
                                                             const KTextEditor::Cursor &cursor,
                                                             bool reverse)
{
  const QRegExp boundaryRegExp("\\b");
  const int line = cursor.line();
  const int column = cursor.column();
  int boundary;
  QString text;
  if(reverse) {
    text = document->text(KTextEditor::Range(line, 0, line, column));
    boundary = qMax(0, text.lastIndexOf(boundaryRegExp));
  }
  else {
    const int lineLength = document->lineLength(line);
    text = document->text(KTextEditor::Range(line, column, line, lineLength));
    ON_THE_FLY_DEBUG << text;
    QRegExp extendedBoundaryRegExp("(\\W|$)");
    int completeMatch = extendedBoundaryRegExp.indexIn(text);

    boundary = column + qMax(0, completeMatch);
  }
  return KTextEditor::Cursor(line, boundary);
}

void KateOnTheFlyChecker::misspelling(const QString &word, int start)
{
  ON_THE_FLY_DEBUG;
//   if(!m_enabled) {
//     ON_THE_FLY_DEBUG << "leaving as on-the-fly checking is disabled "
//                   << QThread::currentThreadId();
//     return;
//   }
  if(m_currentlyCheckedItem == invalidSpellCheckQueueItem) {
    ON_THE_FLY_DEBUG << "exited as no spell check is taking place";
    return;
  }
  ON_THE_FLY_DEBUG << "misspelled " << word
                                    << " at line "
                                    << m_currentlyCheckedItem.second.first
                                    << " column " << start;

  KTextEditor::Document *document = m_currentlyCheckedItem.first;
  KTextEditor::SmartRange *spellCheckRange = m_currentlyCheckedItem.second.first;
  int line = spellCheckRange->start().line();
  int rangeStart = spellCheckRange->start().column();

  KTextEditor::SmartInterface *smartInterface =
                              qobject_cast<KTextEditor::SmartInterface*>(document);
  if(smartInterface) {
    smartInterface->smartMutex()->lock();
    KTextEditor::SmartRange *smartRange =
                              smartInterface->newSmartRange(KTextEditor::Range(line,
                                                                              rangeStart + start,
                                                                              line,
                                                                              rangeStart + start + word.length()));
    smartRange->addWatcher(this);
    m_myranges.push_back(smartRange);
    KTextEditor::Attribute *attribute = new KTextEditor::Attribute();
    attribute->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    attribute->setUnderlineColor(QColor(Qt::red));
    smartRange->setAttribute(KTextEditor::Attribute::Ptr(attribute));
    m_misspelledMap[document].push_back(MisspelledItem(smartRange, m_currentlyCheckedItem.second.second));
    installSmartRange(smartRange, static_cast<KateDocument*>(document));
    smartInterface->smartMutex()->unlock();
  }

  m_backgroundChecker->continueChecking();
  ON_THE_FLY_DEBUG << "exited";
}

void KateOnTheFlyChecker::spellCheckDone()
{
  ON_THE_FLY_DEBUG;
  ON_THE_FLY_DEBUG << "on-the-fly spell check done, queue length " << m_spellCheckQueue.size();
  if(!m_enabled) {
    ON_THE_FLY_DEBUG << "leaving as on-the-fly checking is disabled "
                << QThread::currentThreadId();
    return;
  }
  if(m_currentlyCheckedItem == invalidSpellCheckQueueItem) {
    ON_THE_FLY_DEBUG << "exited as no spell check is taking place";
    return;
  }
  delete(m_currentlyCheckedItem.second.first);
  m_currentlyCheckedItem = invalidSpellCheckQueueItem;
  if(!m_spellCheckQueue.empty()) {
    QTimer::singleShot(0, this, SLOT(performSpellCheck()));
  }
  ON_THE_FLY_DEBUG << "exited";
}

void KateOnTheFlyChecker::removeInstalledSmartRanges(KTextEditor::Document* document)
{
  ON_THE_FLY_DEBUG;
  KTextEditor::SmartInterface *smartInterface =
                              qobject_cast<KTextEditor::SmartInterface*>(document);
  if (!smartInterface) return;
  SmartRangeList& smartRangeList = m_installedSmartRangeMap[document];
  for(SmartRangeList::iterator i = smartRangeList.begin(); i != smartRangeList.end(); ++i) {
    smartInterface->smartMutex()->lock();
    smartInterface->removeHighlightFromDocument(*i);
    smartInterface->smartMutex()->unlock();
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

QList<KTextEditor::SmartRange*> KateOnTheFlyChecker::installedSmartRangesInDocument(KTextEditor::Document *document,
                                                                                 const KTextEditor::Range& range)
{
  ON_THE_FLY_DEBUG << range;
  SmartRangeList toReturn;
  KTextEditor::SmartInterface *smartInterface =
                              qobject_cast<KTextEditor::SmartInterface*>(document);
  if(!smartInterface) {
    return toReturn;
  }
 
  ON_THE_FLY_DEBUG<<"inspecting document:"<<document;
  smartInterface->smartMutex()->lock();
  const SmartRangeList& smartRangeList = smartInterface->documentHighlights();
  for(SmartRangeList::const_iterator j = smartRangeList.begin(); j != smartRangeList.end(); ++j) {
    KTextEditor::SmartRange *smartRange = *j;
    if (m_myranges.contains(smartRange)) { //JOWENN
      if(smartRange->overlaps(range)) {
        toReturn.push_back(smartRange);
      }
    }
  }
  smartInterface->smartMutex()->unlock();  
  ON_THE_FLY_DEBUG << "found" << toReturn.size() << "ranges";
  return toReturn;
}

void KateOnTheFlyChecker::installSmartRange(KTextEditor::SmartRange *smartRange, KateDocument* document)
{
  ON_THE_FLY_DEBUG;
  if(!(document->isOnTheFlySpellCheckingEnabled())) {
    return;
  }
  KTextEditor::SmartInterface *smartInterface = document;
  if(!smartInterface) {
    return;
  }

    smartInterface->smartMutex()->lock();
    smartInterface->addHighlightToDocument(smartRange);
    smartInterface->smartMutex()->unlock();
    m_installedSmartRangeMap[document].push_back(smartRange);
    //perhaps only update views, where the range is visible
    Q_FOREACH(KTextEditor::View* view, document->views()) { view->update();}

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

void KateOnTheFlyChecker::addDocument(KateDocument *document)
{
  ON_THE_FLY_DEBUG;
  connect(document, SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&)),
          this, SLOT(textInserted(KTextEditor::Document*, const KTextEditor::Range&)));
  connect(document, SIGNAL(textRemoved(KTextEditor::Document*, const KTextEditor::Range&)),
          this, SLOT(textRemoved(KTextEditor::Document*, const KTextEditor::Range&)));
  connect(document, SIGNAL(viewCreated(KTextEditor::Document*, KTextEditor::View*)),
          this, SLOT(addView(KTextEditor::Document*, KTextEditor::View*)));
  connect(document, SIGNAL(highlightingModeChanged (KTextEditor::Document*)),
          this, SLOT(updateDocument(KTextEditor::Document*)));
  connect(document, SIGNAL(respellCheckBlock(KateDocument *, int, int)), this , SLOT(handleRespellCheckBlock(KateDocument *, int, int)));
  const QList<KTextEditor::View*>& views = document->views();
  for(QList<KTextEditor::View*>::const_iterator i = views.begin(); i != views.end(); ++i) {
    addView(document, *i);
  }
}

void KateOnTheFlyChecker::removeDocument(KateDocument *document)
{
  ON_THE_FLY_DEBUG;
  disconnect(document, SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&)),
          this, SLOT(textInserted(KTextEditor::Document*, const KTextEditor::Range&)));
  disconnect(document, SIGNAL(textRemoved(KTextEditor::Document*, const KTextEditor::Range&)),
          this, SLOT(textRemoved(KTextEditor::Document*, const KTextEditor::Range&)));
  disconnect(document, SIGNAL(viewCreated(KTextEditor::Document*, KTextEditor::View*)),
          this, SLOT(addView(KTextEditor::Document*, KTextEditor::View*)));
  disconnect(document, SIGNAL(highlightingModeChanged (KTextEditor::Document*)),
          this, SLOT(updateDocument(KTextEditor::Document*)));
  const QList<KTextEditor::View*>& views = document->views();
  for(QList<KTextEditor::View*>::const_iterator i = views.begin(); i != views.end(); ++i) {
    removeView(*i);
  }
  freeDocument(document);
}

void KateOnTheFlyChecker::updateDocument(KTextEditor::Document *document)
{
  KateDocument *kateDocument = dynamic_cast<KateDocument*>(document);
  if(!kateDocument) {
    return;
  }
ON_THE_FLY_DEBUG;
  removeDocument(kateDocument);
  addDocument(kateDocument);
}

void KateOnTheFlyChecker::addView(KTextEditor::Document *document, KTextEditor::View *view)
{
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

void KateOnTheFlyChecker::updateInstalledSmartRanges(KateDocument *document)
{
  const QList<KTextEditor::View*>& views = document->views();
  for(QList<KTextEditor::View*>::const_iterator i = views.begin(); i != views.end(); ++i) {
    updateInstalledSmartRanges(static_cast<KateView*>(*i));
  }
}

void KateOnTheFlyChecker::updateInstalledSmartRanges(KateView *view)
{
  ON_THE_FLY_DEBUG;
  KTextEditor::Range oldDisplayRange = m_displayRangeMap[view];
  KateDocument *document = static_cast<KateDocument*>(view->document());
  KTextEditor::Range newDisplayRange = view->visibleRange();
  ON_THE_FLY_DEBUG << "new range: " << newDisplayRange;
  ON_THE_FLY_DEBUG << "old range: " << oldDisplayRange;
  const QList<KTextEditor::View*>& viewList = view->document()->views();
  MisspelledList& misspelledList = m_misspelledMap[view->document()];
  SmartRangeList deleteList;
  for(MisspelledList::iterator it = misspelledList.begin(); it != misspelledList.end(); ++it) {
    KTextEditor::SmartRange *smartRange = (*it).first;
    if(!smartRange->overlaps(newDisplayRange)) {
      bool stillVisible = false;
      for(QList<KTextEditor::View*>::const_iterator it2 = viewList.begin();
                                                   it2 != viewList.end(); ++it2) {
        KateView *view2 = static_cast<KateView*>(*it2);
        if(view != view2 && smartRange->overlaps(view2->visibleRange())) {
          stillVisible = true;
          break;
        }
      }
      if(!stillVisible) {
        // only delete them later as 'smartRangeList' will otherwise be modified
        deleteList.push_front(smartRange);
      }
    }
  }
  for(SmartRangeList::iterator it = deleteList.begin(); it != deleteList.end(); ++it) {
    delete(*it);
  }
  deleteList.clear();
  m_displayRangeMap[view] = newDisplayRange;
  if(oldDisplayRange.isValid()) {  
    bool emptyAtStart = m_spellCheckQueue.empty();
    for(int line = newDisplayRange.end().line(); line >= newDisplayRange.start().line(); --line) {
      if(!oldDisplayRange.containsLine(line)) {
        bool visible = false;
        for(QList<KTextEditor::View*>::const_iterator it2 = viewList.begin();
                                                    it2 != viewList.end(); ++it2) {
          KateView *view2 = static_cast<KateView*>(*it2);
          if(view != view2 && view2->visibleRange().containsLine(line)) {
            visible = true;
            break;
          }
        }
        if(!visible) {
          queueLineSpellCheck(document, line);
        }
      }
    }
    if(emptyAtStart && !m_spellCheckQueue.isEmpty()) {
      QTimer::singleShot(0, this, SLOT(performSpellCheck()));
    }
  }
//  removeInstalledSmartRanges(document);
//  installSmartRanges(document);
}

QList<KTextEditor::SmartRange*> KateOnTheFlyChecker::getSmartRanges(KateDocument *document)
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

void KateOnTheFlyChecker::queueSpellCheckVisibleRange(KateDocument *kateDocument, const KTextEditor::Range& range)
{
  const QList<KTextEditor::View*>& viewList = kateDocument->views();
  for(QList<KTextEditor::View*>::const_iterator j = viewList.begin(); j != viewList.end(); ++j) {
    KateView *view = static_cast<KateView*>(*j);
    KTextEditor::Range visibleRange = view->visibleRange();
    KTextEditor::Range intersection = visibleRange.intersect(range);
    if(intersection.isEmpty()) {
      continue;
    }
    if(intersection.onSingleLine()) {
      queueLineSpellCheck(kateDocument, intersection);
    }
    else {
      int intersectionBegin = intersection.start().line();
      int intersectionEnd = intersection.end().line();
      for(int line = intersectionEnd; line >= intersectionBegin; --line) {
        const int lineLength = kateDocument->lineLength(line);
        if(line == intersectionBegin) {
          queueLineSpellCheck(kateDocument, KTextEditor::Range(intersection.start(),
                                                           KTextEditor::Cursor(line, lineLength)));
        }
        else if(line == intersectionEnd) {
          queueLineSpellCheck(kateDocument, KTextEditor::Range(KTextEditor::Cursor(line, 0),
                                                           intersection.end()));
        }
        else {
          queueLineSpellCheck(kateDocument, KTextEditor::Range(KTextEditor::Cursor(line, 0),
                                                           KTextEditor::Cursor(line, lineLength)));
        }
      }
    }
  }  
}

void KateOnTheFlyChecker::queueLineSpellCheck(KateDocument *document, int line)
{
  queueLineSpellCheck(document, KTextEditor::Range(line, 0, line, document->lineLength(line)));
}

void KateOnTheFlyChecker::queueLineSpellCheck(KateDocument *document, const KTextEditor::Range& range)
{
  ON_THE_FLY_DEBUG << document << range;
  KTextEditor::SmartInterface *smartInterface =
                                qobject_cast<KTextEditor::SmartInterface*>(document);
  if(!smartInterface) {
    return;
  }

  Q_ASSERT(range.onSingleLine());

  if(range.isEmpty()) {
    return;
  }
  const int line = range.start().line();
  const int end = range.end().column();
  
  KateTextLine::Ptr kateTextLine = document->kateTextLine(line);
  int begin = -1;
  bool inSpellCheckArea = false;
  // clear all the highlights that are currently present in the range that
  // is supposed to be checked, necessary due to highlighting
  
  //items can be non unique !!!!!
  const SmartRangeList highlightsList = installedSmartRangesInDocument(document, range);
  qDeleteAll(highlightsList);

  for(int i = range.start().column(); i < end; ++i) {
    unsigned int attribute = kateTextLine->attribute(i);
    ON_THE_FLY_DEBUG << attribute << kateTextLine->at(i);
    if(!document->highlight()->attributeRequiresSpellchecking(attribute)) {
      if(i == 0) {
        continue;
      }
      else if(inSpellCheckArea) {
        addToSpellCheckQueue(document, KTextEditor::Range(line, begin, line, i), document->dictionary());
        begin = -1;
        inSpellCheckArea = false;
      }
    }
    else if(!inSpellCheckArea) {
      begin = i;
      inSpellCheckArea = true;
    }
  }
  if(inSpellCheckArea) {
    addToSpellCheckQueue(document, KTextEditor::Range(line, begin, line, end), document->dictionary());
  }
}

void KateOnTheFlyChecker::addToSpellCheckQueue(KateDocument *document, const KTextEditor::Range& range, const QString& dictionary)
{
  KTextEditor::SmartInterface *smartInterface =
                                qobject_cast<KTextEditor::SmartInterface*>(document);
  if(!smartInterface) {
    return;
  }

  addToSpellCheckQueue(document, smartInterface->newSmartRange(range), dictionary);
}

void KateOnTheFlyChecker::addToSpellCheckQueue(KateDocument *document, KTextEditor::SmartRange *range,
                                                                       const QString& dictionary)
{
  ON_THE_FLY_DEBUG << document << *range << dictionary;
/*    for(QList<SpellCheckQueueItem>::const_iterator i = m_spellCheckQueue.constBegin();
                                          i != m_spellCheckQueue.constEnd();) {
      KTextEditor::SmartRange *spellCheckRange = (*i).second.first;
      KTextEditor::Document *lineDocument = (*i).first;
      if(document == lineDocument && spellCheckRange->overlaps(*range)) {
        ON_THE_FLY_DEBUG<<"Item already in queue, not adding again";
        delete range;
        return;
      }
    }*/
  // if the queue contains a subrange of 'range', we remove that one
  for(QList<SpellCheckQueueItem>::iterator i = m_spellCheckQueue.begin();
                                           i != m_spellCheckQueue.end();) {
      KTextEditor::SmartRange *spellCheckRange = (*i).second.first;
      KTextEditor::Document *queuedDocument = (*i).first;
      if(document == queuedDocument && range->contains(*spellCheckRange)) {
        delete spellCheckRange;
        i = m_spellCheckQueue.erase(i);
      }
      else {
        ++i;
      }
  }
  // leave 'push_front' here as it is a LIFO queue, i.e. a stack
  m_spellCheckQueue.push_front(SpellCheckQueueItem(document,
                                                     SpellCheckItem(range, dictionary)));                                                     
  ON_THE_FLY_DEBUG << "added"
                   << m_spellCheckQueue.first()
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
  ON_THE_FLY_DEBUG<<m_enabled<<endl;
  if (!(static_cast<KateDocument*>(view->document())->isOnTheFlySpellCheckingEnabled())) return;
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
    KTextEditor::SmartRange *r = m_eliminatedRanges.takeFirst();
    ON_THE_FLY_DEBUG << r;
    delete r;
  }
}

void KateOnTheFlyChecker::handleModifiedRanges()
{
  for(ModificationList::iterator i = m_modificationList.begin(); i != m_modificationList.end(); ++i) {
    ModificationItem item = *i;
    DocumentRangePair documentRangePair = item.second;
    if(item.first == TEXT_INSERTED) {
      handleInsertedText(documentRangePair.first, documentRangePair.second);
    }
    else {
      handleRemovedText(documentRangePair.first, documentRangePair.second);
    }
  }
  m_modificationList.clear();
}

void KateOnTheFlyChecker::removeDocumentFromModificationList(KTextEditor::Document *document)
{
  for(ModificationList::iterator i = m_modificationList.begin(); i != m_modificationList.end();) {
    ModificationItem item = *i;
    DocumentRangePair documentRangePair = item.second;
    if(documentRangePair.first == document) {
      i = m_modificationList.erase(i);
    }
    else {
      ++i;
    }
  }
}

#include "ontheflycheck.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
