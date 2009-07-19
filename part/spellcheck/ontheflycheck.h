/* 
 * Copyright (C) 2008-2009 by Michel Ludwig (michel.ludwig@kdemail.net)
 * Copyright (C) 2009 by Joseph Wenninger (jowenn@kde.org)
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
  typedef QList<KTextEditor::SmartRange*> SmartRangeList;
  typedef QPair<KTextEditor::SmartRange*, QString> MisspelledItem;
  typedef QList<MisspelledItem> MisspelledList;

  typedef QPair<ModificationType, KTextEditor::SmartRange*> ModificationItem;
  typedef QList<ModificationItem> ModificationList; 

  public:
    KateOnTheFlyChecker(KateDocument *document);
    ~KateOnTheFlyChecker();

    static int debugArea();

  public Q_SLOTS:
    void textInserted(KTextEditor::Document *document, const KTextEditor::Range &range);
    void textRemoved(KTextEditor::Document *document, const KTextEditor::Range &range);

    void updateConfig();

    void updateInstalledSmartRanges(KateView *view);
    void updateInstalledSmartRanges();

  protected:
    KateDocument *const m_document;
    QList<SpellCheckItem> m_spellCheckQueue;
    Sonnet::BackgroundChecker *m_backgroundChecker;
    SpellCheckItem m_currentlyCheckedItem;
    static const SpellCheckItem invalidSpellCheckQueueItem;
    MisspelledList m_misspelledList;
    SmartRangeList m_installedSmartRangeList;
    ModificationList m_modificationList;

    void freeDocument();

    SmartRangeList installedSmartRanges(const KTextEditor::Range& range);

    void installSmartRange(KTextEditor::SmartRange *smartRange);

    void queueLineSpellCheck(KateDocument *document, int line);
    /**
     * 'range' must be on a single line
     **/
    void queueLineSpellCheck(const KTextEditor::Range& range, const QString& dictionary);
    void queueSpellCheckVisibleRange(const KTextEditor::Range& range);
    void queueSpellCheckVisibleRange(KateView *view, const KTextEditor::Range& range);

    void addToSpellCheckQueue(const KTextEditor::Range& range, const QString& dictionary);
    void addToSpellCheckQueue(KTextEditor::SmartRange *range, const QString& dictionary);

    QTimer *m_viewRefreshTimer;
    QPointer<KateView> m_refreshView;

    virtual void rangeDeleted(KTextEditor::SmartRange *range);
    bool removeRangeFromSpellCheckQueue(KTextEditor::SmartRange *range);
    virtual void rangeEliminated(KTextEditor::SmartRange *range);
    QList<KTextEditor::SmartRange*> m_eliminatedRanges;

    QMap<KTextEditor::View*, KTextEditor::Range> m_displayRangeMap;
    QList<KTextEditor::SmartRange*> m_myranges;
    KTextEditor::Cursor findBeginningOfWord(const KTextEditor::Cursor &cursor,
                                            bool reverse);

  protected Q_SLOTS:
    void performSpellCheck();
    void misspelling(const QString &word, int start);
    void spellCheckDone();

    void viewDestroyed(QObject* obj);
    void addView(KTextEditor::Document *document, KTextEditor::View *view);
    void removeView(KTextEditor::View *view);

    void restartViewRefreshTimer(KateView *view);
    void viewRefreshTimeout();

    void deleteEliminatedRanges();

    void handleModifiedRanges();
    void handleInsertedText(const KTextEditor::Range &range);
    void handleRemovedText(const KTextEditor::Range &range);
    void handleRespellCheckBlock(KateDocument *document, int start, int end);
    bool removeSmartRangeFromModificationList(KTextEditor::SmartRange *range);
    void clearModificationList();
};

#endif
 
// kate: space-indent on; indent-width 2; replace-tabs on;
