/* This file is part of the KDE libraries
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

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

#ifndef KATE_SEARCH_BAR_H
#define KATE_SEARCH_BAR_H 1

#include "kateviewhelpers.h"
#include "katesmartrange.h"
#include "katedocument.h"
#include "katehistorymodel.h"
#include "kateescapedtextsearch.h"

#include <kcolorscheme.h>


namespace KTextEditor {
    class SmartRangeNotifier;
}

class KateView;
class QVBoxLayout;
class QCheckBox;
class QComboBox;
class QStringListModel;

namespace Ui {
    class IncrementalSearchBar;
    class PowerSearchBar;
}



class KateSearchBar : public KateViewBarWidget {
    Q_OBJECT

private:
    enum SearchMode {
        // NOTE: Concrete values are important here
        // to work with the combobox index!
        MODE_PLAIN_TEXT = 0,
        MODE_WHOLE_WORDS = 1,
        MODE_ESCAPE_SEQUENCES = 2,
        MODE_REGEX = 3
    };

public:
    enum SearchDirection {
      SearchForward,
      SearchBackward
    };

public:
    explicit KateSearchBar(bool initAsPower, KateView* view, QWidget* parent=0);
    ~KateSearchBar();

    bool isPower() const;

    // Only used by KateView
    static void nextMatchForSelection(KateView * view, SearchDirection searchDirection);

public Q_SLOTS:
    // Called for <F3> and <Shift>+<F3>
    void findNext();
    void findPrevious();

    // Also used by KateView
    void onMutatePower();
    void onMutateIncremental();

    void enableHighlights(bool enable);

protected:
    // Overridden
    virtual void showEvent(QShowEvent * event);
    virtual void closed();

private Q_SLOTS:
    void onIncPatternChanged(const QString & pattern, bool invokedByUserAction = true);
    void onMatchCaseToggled(bool matchCase);
    void onHighlightAllToggled(bool checked);
    void onFromCursorToggled(bool fromCursor);

    void onReturnPressed();
    void onSelectionChanged();
    void onCursorPositionChanged();

    void onPowerPatternChanged(const QString & pattern);
    void onPowerReplaceNext();
    void onPowerReplaceAll();

    void onPowerModeChanged(int index, bool invokedByUserAction = true);
    void onPowerPatternContextMenuRequest();
    void onPowerPatternContextMenuRequest(const QPoint&);
    void onPowerReplacmentContextMenuRequest();
    void onPowerReplacmentContextMenuRequest(const QPoint&);

    void onRangeContentsChanged(KTextEditor::SmartRange* range);

private:
    // Helpers
    bool find(bool replace, SearchDirection searchDirection = SearchForward);
    void findAll(KTextEditor::Range inputRange,
            KTextEditor::Search::SearchOptions enabledOptions,
            const QString * replacement);

    bool isChecked(QCheckBox * checkbox);
    bool isChecked(QAction * menuAction);
    void setChecked(QCheckBox * checkbox, bool checked);
    void setChecked(QAction * menuAction, bool checked);

    QString searchPattern() const;
    bool isPatternValid() const;

    void resetHighlights();

    void highlight(const KTextEditor::Range & range, const QColor & color);
    void highlightMatch(const KTextEditor::Range & range);
    void highlightReplacement(const KTextEditor::Range & range);
    void highlightAllMatches(KTextEditor::Search::SearchOptions searchOptions);
    void neutralMatch();
    void indicateMatch(bool wrapped);
    void indicateMismatch();
    void indicateNothing();
    static void selectRange(KateView * view, const KTextEditor::Range & range);
    void nonstatic_selectRange(KateView * view, const KTextEditor::Range & range);
    void nonstatic_selectRange2(KateView * view, const KTextEditor::Range & range);
    void buildReplacement(QString & output, QList<ReplacementPart> & parts,
            const QVector<KTextEditor::Range> & details, int replacementCounter);
    void replaceMatch(const QVector<KTextEditor::Range> & match, const QString & replacement,
            int replacementCounter = 1);

    QVector<QString> getCapturePatterns(const QString & pattern) const;
    void showExtendedContextMenu(bool forPattern, const QPoint& pos);

    void givePatternFeedback();
    void addCurrentTextToHistory(QComboBox * combo);
    void backupConfig(bool ofPower);
    void sendConfig();
    void fixForSingleLine(KTextEditor::Range & range, SearchDirection searchDirection);

private:
    // Shared by both dialogs
    KTextEditor::SmartRange * m_topRange;
    KTextEditor::SmartRangeNotifier *m_rangeNotifier;
    QVBoxLayout *const m_layout;
    QWidget * m_widget;

    // Incremental search related
    Ui::IncrementalSearchBar * m_incUi;
    QMenu * m_incMenu;
    QAction * m_incMenuMatchCase;
    QAction * m_incMenuFromCursor;
    QAction * m_incMenuHighlightAll;
    KTextEditor::Cursor m_incInitCursor;

    // Power search related
    Ui::PowerSearchBar * m_powerUi;
    QMenu * m_powerMenu;
    QAction * m_powerMenuFromCursor;
    QAction * m_powerMenuHighlightAll;
    QAction * m_powerMenuSelectionOnly;

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

// kate: space-indent on; indent-width 2; replace-tabs on;
