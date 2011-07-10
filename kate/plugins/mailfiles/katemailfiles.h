/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_MAILFILES_H__
#define __KATE_MAILFILES_H__

#include <kate/plugin.h>
#include <kate/mainwindow.h>

class KateMailFilesPlugin: public Kate::Plugin
{
    Q_OBJECT

  public:
    explicit KateMailFilesPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateMailFilesPlugin()
    {}

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);
};

class KateMailFilesPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateMailFilesPluginView (Kate::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateMailFilesPluginView ();

  private Q_SLOTS:
    void slotMail();
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

