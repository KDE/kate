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
#include <ktexteditor/commandinterface.h>
#include <kxmlguiclient.h>

#include "katefinddialog.h"

class KateFindInFilesView;
class KateResultView;
class KateGrepCommand;

class KateFindInFilesPlugin: public Kate::Plugin
{
    Q_OBJECT

    static KateFindInFilesPlugin* s_self;
  public:
    explicit KateFindInFilesPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateFindInFilesPlugin();

    static KateFindInFilesPlugin* self();

    virtual Kate::PluginView *createView (Kate::MainWindow *mainWindow);
    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

  public:
    KateFindInFilesView* viewForMainWindow(Kate::MainWindow* mw);

  public Q_SLOTS:
    void removeView(KateFindInFilesView* view);

  private:
    KateGrepCommand* m_grepCommand;
    QList<KateFindInFilesView*> m_views;
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

  Q_SIGNALS:
    void aboutToBeRemoved(KateFindInFilesView* view);

  private:
    Kate::MainWindow* m_mw;
    KateFindDialog* m_findDialog;
    QList<KateResultView*> m_resultViews;
};

class KateGrepCommand : public KTextEditor::Command
{
  public:
    KateGrepCommand();
    virtual ~KateGrepCommand();

  //
  // KTextEditor::Command
  //
  public:
    virtual const QStringList &cmds ();
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg);
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
