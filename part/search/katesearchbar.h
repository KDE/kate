/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
 *  Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>
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

#ifndef KATE_SEARCH_BAR_H
#define KATE_SEARCH_BAR_H 1

#include "kateviewhelpers.h"
#include "katepartprivate_export.h"

#include <ktexteditor/attribute.h>
#include <ktexteditor/searchinterface.h>

class KateView;
class KateViewConfig;
class QVBoxLayout;
class QComboBox;

namespace Ui {
    class IncrementalSearchBar;
    class PowerSearchBar;
}

namespace KTextEditor {
    class MovingRange;
}


class KATEPART_TESTS_EXPORT KateSearchBar : public KateViewBarWidget {
    Q_OBJECT

public:
    enum SearchMode {
        // NOTE: Concrete values are important here
        // to work with the combobox index!
        MODE_PLAIN_TEXT = 0,
        MODE_WHOLE_WORDS = 1,
        MODE_ESCAPE_SEQUENCES = 2,
        MODE_REGEX = 3
    };

    enum MatchResult {
        MatchFound,
        MatchWrappedForward,
        MatchWrappedBackward,
        MatchMismatch,
        MatchNothing,
        MatchNeutral
    };

    enum SearchDirection {
      SearchForward,
      SearchBackward
    };

public:
    explicit KateSearchBar(bool initAsPower, KateView* view, KateViewConfig *config);
    ~KateSearchBar();

    bool isPower() const;

    QString searchPattern() const;
    QString replacementPattern() const;

    bool selectionOnly() const;
    bool matchCase() const;

    // Only used by KateView
    static void nextMatchForSelection(KateView * view, SearchDirection searchDirection);

public Q_SLOTS:
    void setSearchPattern(const QString &searchPattern);
    void setReplacePattern(const QString &replacePattern);
    void setSearchMode(SearchMode mode);

    void setSelectionOnly(bool selectionOnly);
    void setMatchCase(bool matchCase);

    // Called for <F3> and <Shift>+<F3>
    void findNext();
    void findPrevious();
    void findAll();

    void replaceNext();
    void replaceAll();

    // Also used by KateView
    void enterPowerMode();
    void enterIncrementalMode();

    void clearHighlights();

protected:
    // Overridden
    virtual void showEvent(QShowEvent * event);
    virtual void closed();

private Q_SLOTS:
    void onIncPatternChanged(const QString & pattern);
    void onMatchCaseToggled(bool matchCase);

    void onReturnPressed();
    void updateSelectionOnly();
    void updateIncInitCursor();

    void onPowerPatternChanged(const QString & pattern);

    void onPowerModeChanged(int index);
    void onPowerPatternContextMenuRequest();
    void onPowerPatternContextMenuRequest(const QPoint&);
    void onPowerReplacmentContextMenuRequest();
    void onPowerReplacmentContextMenuRequest(const QPoint&);

private:
    // Helpers
    bool find(SearchDirection searchDirection = SearchForward, const QString * replacement = 0);
    int findAll(KTextEditor::Range inputRange, const QString * replacement);

    bool isPatternValid() const;

    KTextEditor::Search::SearchOptions searchOptions(SearchDirection searchDirection = SearchForward) const;

    void highlightMatch(const KTextEditor::Range & range);
    void highlightReplacement(const KTextEditor::Range & range);
    void indicateMatch(MatchResult matchResult);
    static void selectRange(KateView * view, const KTextEditor::Range & range);
    void selectRange2(const KTextEditor::Range & range);

    QVector<QString> getCapturePatterns(const QString & pattern) const;
    void showExtendedContextMenu(bool forPattern, const QPoint& pos);

    void givePatternFeedback();
    void addCurrentTextToHistory(QComboBox * combo);
    void backupConfig(bool ofPower);
    void sendConfig();
    void fixForSingleLine(KTextEditor::Range & range, SearchDirection searchDirection);

protected:
    KateView *const m_view;
    KateViewConfig *const m_config;
    QList<KTextEditor::MovingRange*> m_hlRanges;

private:
    // Shared by both dialogs
    QVBoxLayout *const m_layout;
    QWidget * m_widget;

    // Incremental search related
    Ui::IncrementalSearchBar * m_incUi;
    KTextEditor::Cursor m_incInitCursor;

    // Power search related
    Ui::PowerSearchBar * m_powerUi;

    // attribute to highlight matches with
    KTextEditor::Attribute::Ptr highlightMatchAttribute;
    KTextEditor::Attribute::Ptr highlightReplacementAttribute;

    // Status backup
    bool m_incHighlightAll : 1;
    bool m_incFromCursor : 1;
    bool m_incMatchCase : 1;
    bool m_powerMatchCase : 1;
    bool m_powerFromCursor : 1;
    bool m_powerHighlightAll : 1;
    unsigned int m_powerMode : 2;
};



#endif // KATE_SEARCH_BAR_H

// kate: space-indent on; indent-width 4; replace-tabs on;
