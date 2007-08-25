/* ##################################################################
## 
##  PLEASE LEAVE THE OLD CODE BELOW: I WILL DO CLEANUP LATER,
##  I PROMISE ;-)
##
##  Sebastian Pipping (sping), webmaster@hartwork.org
##
##  TODO:
##  * D pointer!
##  * Match/wrap indication
##  * Search/replace history
##  * Highlight all with background thread
##  * Fix regex backward search?
##  * Fix match/replacement highlighting?
##  * "Add..." buttons
##  * Proper loading/saving of search settings
##  * Disabled/enable buttons live as needed
##
################################################################## */


/* This file is part of the KDE libraries
   Copyright (C) 2006 Andreas Kling <kling@impul.se>
   Copyright (C) 2007 SebastianPipping <webmaster@hartwork.org>

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



class KateView;
class QVBoxLayout;
class QCheckBox;

namespace Ui {
    class IncrementalSearchBar;
    class PowerSearchBar;
}



class KateSearchBar : public KateViewBarWidget {
    Q_OBJECT

public:
    explicit KateSearchBar(KateViewBar * viewBar);
    ~KateSearchBar();

private: // helpers
    void highlightMatch(const KTextEditor::Range & range);
    void highlightReplacement(const KTextEditor::Range & range);
    void selectRange(const KTextEditor::Range & range);
    void buildReplacement(QString & output, QList<ReplacementPart> & parts,
            const QVector<KTextEditor::Range> & details);
    void replaceMatch(const QVector<KTextEditor::Range> & match, const QString & replacement);

public Q_SLOTS:
    void findNext();
    void findPrevious();

    void onIncPatternChanged(const QString &);
    void onIncNext();
    void onIncPrev();
    void onStep(bool replace, bool forwards = true);
    void onPowerFindNext();
    void onPowerFindPrev();
    void onPowerReplaceNext();
    void onPowerReplaceAll();

public Q_SLOTS: // Also toggles for KateView
    void onMutatePower();
    void onMutateIncremental();

private: // helpers
    bool isChecked(QCheckBox * checkbox);
    bool isChecked(QAction * menuAction);
    void enableHighlights(bool enable);
    void resetHighlights();

private: // override    
    void showEvent(QShowEvent * event);
    void hideEvent(QHideEvent * event);

private:
    KateView * m_view;

    QVBoxLayout * m_layout;
    QWidget * m_widget;

    // Incremental stuff
    Ui::IncrementalSearchBar * m_incUi;
    QMenu * m_incMenu;
    QAction * m_incMenuMatchCase;
    QAction * m_incMenuFromCursor;
    QAction * m_incMenuHighlightAll;
    KTextEditor::Cursor m_incInitCursor;

    // Power stuff
    Ui::PowerSearchBar * m_powerUi;

    KTextEditor::SmartRange * m_topRange;

};




#include <QtGui/QWidget>
#include <klineedit.h>

class KateView;

class OLD_KateSearchBar_OLD : public KateViewBarWidget
{
    Q_OBJECT

public:
    explicit OLD_KateSearchBar_OLD(KateViewBar *viewBar);
    ~OLD_KateSearchBar_OLD();

public Q_SLOTS:
    void findNext();
    void findPrevious();
    void replaceOnce();
    void replaceAll();

private Q_SLOTS:
    void slotSearch();
    void slotSpecialOptionTogled();

protected:
    void showEvent( QShowEvent * );
    void hideEvent( QHideEvent * );

private:
    void doSearch(const QString &expression, bool init = false, bool backwards = false );

    KateView *m_view;
    class Private;
    Private * const d;
};

class OLD_KateSearchBar_OLDEdit : public KLineEdit
{
    Q_OBJECT

public:
    OLD_KateSearchBar_OLDEdit(QWidget *parent = 0L);

    enum Status { Normal, NotFound, SearchWrapped };
    const Status status() const { return m_status; }
    void setStatus(Status status);

Q_SIGNALS:
    void findNext();
    void findPrevious();
    void escapePressed();
    void returnPressed();

protected:
    bool event(QEvent *);
    void showEvent(QShowEvent *);

private:
    Status m_status;
};

#endif // KATE_SEARCH_BAR_H
