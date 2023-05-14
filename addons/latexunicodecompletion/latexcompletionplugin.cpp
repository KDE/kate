/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "completionmodel.h"

#include <KPluginFactory>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KTextEditor/CodeCompletionInterface>
#endif
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

class LatexCompletionPlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    LatexCompletionPlugin(QObject *parent, const QVariantList &)
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

private Q_SLOTS:
    void viewCreated(KTextEditor::View *view)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        view->registerCompletionModel(m_model);
#else
        auto iface = qobject_cast<KTextEditor::CodeCompletionInterfaceV2 *>(view);
        if (iface != nullptr)
            iface->registerCompletionModel(m_model);
#endif
    }

private:
    LatexCompletionModel *m_model;
};
K_PLUGIN_FACTORY_WITH_JSON(TextFilterPluginFactory, "latexcompletionplugin.json", registerPlugin<LatexCompletionPlugin>();)

#include "latexcompletionplugin.moc"
