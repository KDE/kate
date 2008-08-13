/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KATE_FIND_DIALOG_H
#define KATE_FIND_DIALOG_H

#include <kate/mainwindow.h>

#include "kategrepthread.h"
#include "ui_findwidget.h"

#include <kdialog.h>

#include <QEvent>

class QToolButton;
class QShowEvent;
class QTreeWidgetItem;
class KConfig;
class KateFindInFilesView;
class KateFindInFilesOptions;

class KateFindDialog : public KDialog, private Ui::FindWidget
{
    Q_OBJECT

  public:
    KateFindDialog(Kate::MainWindow *mw, KateFindInFilesView *view);
    virtual ~KateFindDialog();

    void setPattern(const QList<QRegExp>& pattern);
    void setUrl(const QString& url);
    void setFilter(const QString& filter);
    void setOptions(const KateFindInFilesOptions& options);

    void useResultView(int id);

  protected:
    void showEvent(QShowEvent* event);

    void updateConfig();
    void updateItems();

  private Q_SLOTS:
    void slotSearch();
    void patternTextChanged( const QString &);
    void syncDir();

  private:
    Kate::MainWindow *m_mw;
    KateFindInFilesView *m_view;
    int m_useId;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
