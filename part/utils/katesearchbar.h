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

#include <kcolorscheme.h>

#include <QtCore/QRegExp>



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
    explicit KateSearchBar(bool initAsPower, KateView* view, QWidget* parent=0);
    ~KateSearchBar();

public Q_SLOTS:
    // Called for <F3> and <Shift>+<F3>
    void findNext();
    void findPrevious();

    void onIncPatternChanged(const QString & pattern, bool invokedByUserAction = true);
    void onIncNext();
    void onIncPrev();
    void onIncMatchCaseToggle(bool invokedByUserAction = true);
    void onIncHighlightAllToggle(bool checked, bool invokedByUserAction = true);
    void onIncFromCursorToggle(bool invokedByUserAction = true);

    void onForAll(const QString & pattern, KTextEditor::Range inputRange,
            KTextEditor::Search::SearchOptions enabledOptions,
            const QString * replacement);
    bool onStep(bool replace, bool forwards = true);
    void onReturnPressed();
    void onSelectionChanged();
    void onCursorPositionChanged();

    void onPowerPatternChanged(const QString & pattern);
    void onPowerFindNext();
    void onPowerFindPrev();
    void onPowerReplaceNext();
    void onPowerReplaceAll();
    void onPowerMatchCaseToggle(bool invokedByUserAction = true);
    void onPowerHighlightAllToggle(bool checked, bool invokedByUserAction = true);
    void onPowerFromCursorToggle(bool invokedByUserAction = true);
    void onPowerModeChangedPlainText();
    void onPowerModeChangedWholeWords();
    void onPowerModeChangedEscapeSequences();
    void onPowerModeChangedRegularExpression();
private:
    void onPowerModeChanged();
public Q_SLOTS:
    void onPowerModeChanged(int index, bool invokedByUserAction = true);
    void onPowerPatternContextMenuRequest();
    void onPowerReplacmentContextMenuRequest();

public:
    // Only used by KateView
    static void nextMatchForSelection(KateView * view, bool forwards);

public Q_SLOTS:
    // Also used by KateView
    void onMutatePower();
    void onMutateIncremental();

private:
    // Helpers
    bool isChecked(QCheckBox * checkbox);
    bool isChecked(QAction * menuAction);
    void setChecked(QCheckBox * checkbox, bool checked);
    void setChecked(QAction * menuAction, bool checked);
    void enableHighlights(bool enable);
    void resetHighlights();

    void highlight(const KTextEditor::Range & range, const QColor & color);
    void highlightMatch(const KTextEditor::Range & range);
    void highlightReplacement(const KTextEditor::Range & range);
    void highlightAllMatches(const QString & pattern,
            KTextEditor::Search::SearchOptions searchOptions);
    void adjustBackground(QPalette & palette, KColorScheme::BackgroundRole newRole);
    void neutralMatch();
    void indicateMatch(bool wrapped);
    void indicateMismatch();
    void indicateNothing();
    static void selectRange(KateView * view, const KTextEditor::Range & range);
    void buildReplacement(QString & output, QList<ReplacementPart> & parts,
            const QVector<KTextEditor::Range> & details, int replacementCounter);
    void replaceMatch(const QVector<KTextEditor::Range> & match, const QString & replacement,
            int replacementCounter = 1);

    QVector<QString> getCapturePatterns(const QString & pattern);
    void showExtendedContextMenu(bool forPattern);

    void givePatternFeedback(const QString & pattern);
    void addCurrentTextToHistory(QComboBox * combo);
    void backupConfig(bool ofPower);
    void sendConfig();
    void fixForSingleLine(KTextEditor::Range & range, bool forwards);

private:
    // Overridden
    void showEvent(QShowEvent * event);
    //void hideEvent(QHideEvent * event);
public:
    void closed();

private:
    // Shared by both dialogs
    KTextEditor::SmartRange * m_topRange;
    QVBoxLayout * m_layout;
    QWidget * m_widget;
    QRegExp m_patternTester;

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

