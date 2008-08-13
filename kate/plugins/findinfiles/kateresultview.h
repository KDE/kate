/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007-2008 Dominik Haumann <dhaumann kde org>
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

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

#ifndef _GREPDIALOG_H_
#define _GREPDIALOG_H_

#include <kate/mainwindow.h>

#include "ui_resultwidget.h"
#include "katefindoptions.h"

#include <QEvent>

class QToolButton;
class QShowEvent;
class QTreeWidgetItem;
class KConfig;
class KateFindInFilesView;
class KateGrepThread;

class KateResultView : public QWidget, private Ui::ResultWidget
{
    Q_OBJECT

  public:
    KateResultView(Kate::MainWindow *mw, KateFindInFilesView *view);
    virtual ~KateResultView();

    void makeVisible();
    QWidget* toolView();
    int id() const;

    void startSearch(KateFindInFilesOptions options,
                     const QList<QRegExp>& pattern,
                     const QString& url,
                     const QString& filter);

  protected:
    void keyPressEvent(QKeyEvent *event);
    void setStatusVisible(bool visible);
    void layoutColumns();

  private Q_SLOTS:
    void itemSelected(QTreeWidgetItem *item, int column);
    void searchFinished ();
    void searchMatchFound(const QString &filename, const QString &relname, const QList<int> &lines, const QList<int> &columns, const QString &basename, const QStringList &lineContent);
    void killThread ();
    void deleteToolview();
    void refineSearch();

  private:

    Kate::MainWindow* m_mw;
    QWidget* m_toolView;
    KateFindInFilesView* m_view;
    int m_id;

    KateGrepThread* m_grepThread;

    KateFindInFilesOptions m_options;
    QList<QRegExp> m_lastPatterns;
    QString m_lastUrl;
    QString m_lastFilter;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
