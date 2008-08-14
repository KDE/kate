/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007-2008 Dominik Haumann <dhaumann kde org>

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

#ifndef __KATE_FINDINFILES_H__
#define __KATE_FINDINFILES_H__

#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kxmlguiclient.h>

#include "katefinddialog.h"

class KateFindInFilesView;
class KateResultView;

class KateFindInFilesPlugin: public Kate::Plugin
{
    Q_OBJECT
  public:
    explicit KateFindInFilesPlugin( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KateFindInFilesPlugin()
    {}

    virtual Kate::PluginView *createView (Kate::MainWindow *mainWindow);
    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);
};

/**
 * KateFindInFilesView
 */
class KateFindInFilesView : public Kate::PluginView, public KXMLGUIClient
{
    Q_OBJECT

  public:
    /**
     * constructor
     * @param mw main window
     */
    KateFindInFilesView (Kate::MainWindow *mw);

    /**
     * destructor
     */
    ~KateFindInFilesView ();

    void addResultView(KateResultView* view);
    void removeResultView(KateResultView* view);
    KateResultView* toolViewFromId(int id);
    KateFindDialog* findDialog();

    int freeId();

  public slots:
    void find();

  private:
    Kate::MainWindow* m_mw;
    KateFindDialog* m_findDialog;
    QList<KateResultView*> m_resultViews;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
