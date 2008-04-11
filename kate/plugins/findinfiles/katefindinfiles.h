/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Dominik Haumann <dhaumann@kde.org>

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
#include <kurl.h>
#include <kxmlguiclient.h>

#include <kvbox.h>

#include <kategrepdialog.h>


namespace KParts
{
  }

namespace KateMDI
{
  }

class KateFindInFilesView;

class KateFindInFilesPlugin: public Kate::Plugin
{
    Q_OBJECT
  public:
    explicit KateFindInFilesPlugin( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KateFindInFilesPlugin()
    {}

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);
};

/**
 * KateFindInFilesView
 * This class is used for the internal terminal emulator
 * It uses internally the konsole part, thx to konsole devs :)
 */
class KateFindInFilesView : public Kate::PluginView
{
    Q_OBJECT

  public:
    /**
     * construct us
     * @param mw main window
     * @param parent toolview
     */
    KateFindInFilesView (Kate::MainWindow *mw);

    /**
     * destruct us
     */
    ~KateFindInFilesView ();

    // overwritten: fread and write session config
    void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

    QWidget *toolView() const;

  private:
    /**
     * toolview for this console
     */
    QWidget *m_toolView;

    /**
     * Grepdialog
     */
    KateGrepDialog *m_grepDialog;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
