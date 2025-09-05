/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "completionmodel.h"

#include <KPluginFactory>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

class LatexCompletionPlugin : public KTextEditor::Plugin
{
public:
    LatexCompletionPlugin(QObject *parent)
        : KTextEditor::Plugin(parent)
        , m_model(new LatexCompletionModel(this))
    {
    }

    QObject *createView(KTextEditor::MainWindow *mainWindow) override
    {
        const auto views = mainWindow->views();
        for (auto view : views)
            viewCreated(view);
        connect(mainWindow, &KTextEditor::MainWindow::viewCreated, this, &LatexCompletionPlugin::viewCreated);
        return nullptr;
    }

private:
    void viewCreated(KTextEditor::View *view)
    {
        view->registerCompletionModel(m_model);
    }

private:
    LatexCompletionModel *m_model;
};
K_PLUGIN_FACTORY_WITH_JSON(TextFilterPluginFactory, "latexcompletionplugin.json", registerPlugin<LatexCompletionPlugin>();)

#include "latexcompletionplugin.moc"
