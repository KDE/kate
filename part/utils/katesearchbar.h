/* This file is part of the KDE libraries
   Copyright (C) 2006 Andreas Kling <kling@impul.se>

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



class KateView;
class QVBoxLayout;



class KateSearchBar : public KateViewBarWidget {
    Q_OBJECT

public:
    explicit KateSearchBar(KateViewBar * viewBar);
    ~KateSearchBar();

private:
    void highlightMatch(const KTextEditor::Range & range);

public Q_SLOTS:
    void findNext();
    void findPrevious();

    void onMutate();
    void onIncPatternChanged(const QString &);
    void onIncNext();
    void onIncPrev();
    void onPowerFindNext();
    void onPowerFindPrev();
    void onPowerReplaceNext();
    void onPowerReplaceAll();

private:
    void mutatePower();
    void mutateIncremental();

private:
    KateView * view;
    QVBoxLayout * layout;
    QWidget * incrementalParent;
    QWidget * powerParent;
    KTextEditor::SmartRange * topRange;

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
