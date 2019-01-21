/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KTEXTEDITOR_EXTERNALTOOLS_PLUGIN_H
#define KTEXTEDITOR_EXTERNALTOOLS_PLUGIN_H

#include <KTextEditor/Plugin>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KXMLGUIClient>

#include "externaltools.h"
#include "kateexternaltool.h"

class KateExternalToolsPluginView;
class KateExternalTool;

class KateExternalToolsPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateExternalToolsPlugin(QObject* parent = nullptr, const QList<QVariant>& = QList<QVariant>());
    virtual ~KateExternalToolsPlugin();

    int configPages() const override;
    KTextEditor::ConfigPage* configPage(int number = 0, QWidget* parent = nullptr) override;

    QObject* createView(KTextEditor::MainWindow* mainWindow) override;
    KateExternalToolsPluginView* extView(QWidget* widget);

    void reload();
    QStringList commands() const;
    const KateExternalTool * toolForCommand(const QString & cmd) const;

    void runTool(const KateExternalTool & tool, KTextEditor::View * view);

private:
    QList<KateExternalToolsPluginView*> m_views;
    QVector<KateExternalTool> m_tools;
    QStringList m_commands;
    KateExternalToolsCommand* m_command = nullptr;
private Q_SLOT:
    void viewDestroyed(QObject* view);
};

class KateExternalToolsPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    KateExternalToolsPluginView(KTextEditor::MainWindow* mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateExternalToolsPluginView();

    /**
     * Returns the associated mainWindow
     */
    KTextEditor::MainWindow* mainWindow() const;

    void rebuildMenu();

    KateExternalToolsMenuAction* externalTools = nullptr;

private:
    KTextEditor::MainWindow* m_mainWindow;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
