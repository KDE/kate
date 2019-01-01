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

#ifndef __KATE_EXTERNALTOOLS_H__
#define __KATE_EXTERNALTOOLS_H__

#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>
#include <kate/mainwindow.h>

#include <kxmlguiclient.h>

#include "kateexternaltools.h"

class KateExternalToolsPluginView;

class KateExternalToolsPlugin
  : public Kate::Plugin
  , public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    explicit KateExternalToolsPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateExternalToolsPlugin();
    

    void reload();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);
    KateExternalToolsPluginView *extView(QWidget *widget);
  private: 
    QList<KateExternalToolsPluginView*> m_views;
    KateExternalToolsCommand *m_command;
  private Q_SLOT:
    void viewDestroyed(QObject *view);
  //
  // ConfigInterface
  //
  public:
      virtual uint configPages() const;
      virtual Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0 );
      virtual QString configPageName (uint number = 0) const;
      virtual QString configPageFullName (uint number = 0) const;
      virtual KIcon configPageIcon (uint number = 0) const;
           
};

class KateExternalToolsPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateExternalToolsPluginView (Kate::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateExternalToolsPluginView ();
 
    void rebuildMenu();

    KateExternalToolsMenuAction *externalTools;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

