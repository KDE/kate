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

#include <KTextEditor/Mainwindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/PluginConfigPageInterface>

#include <kxmlguiclient.h>

#include "kateexternaltools.h"

class KateExternalToolsPluginView;

class KateExternalToolsPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateExternalToolsPlugin(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~KateExternalToolsPlugin();

    int configPages() const override;
    KTextEditor::ConfigPage* configPage(int number = 0, QWidget* parent = nullptr) override;

    void reload();

    Kate::PluginView* createView(Kate::MainWindow* mainWindow);
    KateExternalToolsPluginView* extView(QWidget* widget);

private:
    QList<KateExternalToolsPluginView*> m_views;
    KateExternalToolsCommand* m_command;
private
    Q_SLOT : void viewDestroyed(QObject* view);

public:
    /*
          virtual QString configPageName (uint number = 0) const;
          virtual QString configPageFullName (uint number = 0) const;
          virtual KIcon configPageIcon (uint number = 0) const;
      */
};

class KateExternalToolsPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    KateExternalToolsPluginView(Kate::MainWindow* mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateExternalToolsPluginView();

    void rebuildMenu();

    KateExternalToolsMenuAction* externalTools;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
