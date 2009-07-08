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

#ifndef ONTHEFLYCHECK_H
#define ONTHEFLYCHECK_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>

#include <ktexteditor/document.h>
#include <ktexteditor/rangefeedback.h>
#include <sonnet/backgroundchecker.h>
#include <sonnet/speller.h>

#include "katedocument.h"
#include "kateview.h"

class KateOnTheFlyChecker : public QObject, private KTextEditor::SmartRangeWatcher {
  Q_OBJECT

  enum ModificationType {TEXT_INSERTED = 0, TEXT_REMOVED};

  typedef QPair<KTextEditor::SmartRange*, QString> SpellCheckItem;
  typedef QPair<KTextEditor::Document*, SpellCheckItem> SpellCheckQueueItem;
  typedef QMap<KTextEditor::Document*, QMap<int, KTextEditor::SmartRange*> > DocumentSmartRangeMap;
  typedef QList<KTextEditor::SmartRange*> SmartRangeList;
  typedef QPair<KTextEditor::SmartRange*, QString> MisspelledItem;
  typedef QList<MisspelledItem> MisspelledList;

  typedef QPair<KTextEditor::Document*, KTextEditor::Range> DocumentRangePair;
  typedef QPair<ModificationType, DocumentRangePair> ModificationItem;
  typedef QList<ModificationItem> ModificationList; 

  public:
    KateOnTheFlyChecker(QObject *parent = NULL);
    ~KateOnTheFlyChecker();

  public Q_SLOTS:
    void textInserted(KTextEditor::Document *document, const KTextEditor::Range &range);
    void textRemoved(KTextEditor::Document *document, const KTextEditor::Range &range);
    void freeDocument(KTextEditor::Document *document);

    void setEnabled(bool b);

    void addDocument(KateDocument *document);
    void removeDocument(KateDocument *document);
    void updateDocument(KTextEditor::Document *document);

    void updateInstalledSmartRanges(KateView *view);

  protected:
    Sonnet::Speller m_speller;
    QList<SpellCheckQueueItem> m_spellCheckQueue;
    Sonnet::BackgroundChecker *m_backgroundChecker;
    SpellCheckQueueItem m_currentlyCheckedItem;
    static const SpellCheckQueueItem invalidSpellCheckQueueItem;
    bool m_enabled;
    QMap<KTextEditor::Document*, MisspelledList> m_misspelledMap;
    QMap<KTextEditor::View*, SmartRangeList> m_installedSmartRangeMap;
    ModificationList m_modificationList;

    SmartRangeList installedSmartRangesInViews(KTextEditor::Document *document,
                                                                const KTextEditor::Range& range);

    void installSmartRangeIfVisible(KTextEditor::SmartRange *smartRange, KateDocument* document);
    void installSmartRangeIfVisible(KTextEditor::SmartRange *smartRange, KateView* view);
    void installSmartRanges(KateView* view);
    void installSmartRanges();
    void removeInstalledSmartRanges(KTextEditor::Document* document);
    void removeInstalledSmartRanges(KTextEditor::View* view);

    SmartRangeList getSmartRanges(KateDocument *document, const KTextEditor::Range& range);

    void queueLineSpellCheck(KateDocument *document, int line);
    /**
     * 'range' must be on a single line
     **/
    void queueLineSpellCheck(KateDocument *document, const KTextEditor::Range& range);
    void queueSpellCheckVisibleRange(KateDocument *document, const KTextEditor::Range& range);

    void addToSpellCheckQueue(KateDocument *document, const KTextEditor::Range& range, const QString& dictionary);
    void addToSpellCheckQueue(KateDocument *document, KTextEditor::SmartRange *range, const QString& dictionary);

    QTimer *m_viewRefreshTimer;
    QPointer<KateView> m_refreshView;

    virtual void rangeDeleted(KTextEditor::SmartRange *range);
    virtual void rangeEliminated(KTextEditor::SmartRange *range);
    SmartRangeList m_eliminatedRanges;

    QMap<KTextEditor::View*, KTextEditor::Range> m_displayRangeMap;
    QList<KTextEditor::SmartRange*> m_myranges;
    KTextEditor::Cursor findBeginningOfWord(KTextEditor::Document* document,
                                            const KTextEditor::Cursor &cursor,
                                            bool reverse);
 
  protected Q_SLOTS:
    void performSpellCheck();
    void misspelling(const QString &word, int start);
    void spellCheckDone();

    void viewDestroyed(QObject* obj);
    void addView(KTextEditor::Document *document, KTextEditor::View *view);
    void removeView(KTextEditor::View *view);
    void updateInstalledSmartRanges(KateDocument *document);

    void restartViewRefreshTimer(KateView *view);
    void viewRefreshTimeout();

    void deleteEliminatedRanges();
    
    void handleModifiedRanges();
    void handleInsertedText(KTextEditor::Document *document, const KTextEditor::Range &range);
    void handleRemovedText(KTextEditor::Document *document, const KTextEditor::Range &range);
    void removeDocumentFromModificationList(KTextEditor::Document *document);
};

#endif
 
// kate: space-indent on; indent-width 2; replace-tabs on;
