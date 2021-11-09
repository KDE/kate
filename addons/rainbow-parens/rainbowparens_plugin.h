/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef RAINBOW_PARENS_PLUGIN_H
#define RAINBOW_PARENS_PLUGIN_H

#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QPointer>
#include <QVariantList>

class RainbowParenPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit RainbowParenPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class RainbowParenPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit RainbowParenPluginView(RainbowParenPlugin *plugin, KTextEditor::MainWindow *mainwindow);

    void updateColors(KTextEditor::Editor *editor);
    void rehighlight(KTextEditor::View *view);
    void viewChanged(KTextEditor::View *view);

    Q_SLOT void clearRanges(KTextEditor::Document *doc);

    struct ViewSavedRanges {
        QPointer<KTextEditor::View> view;
        std::vector<std::unique_ptr<KTextEditor::MovingRange>> ranges;
    };
    std::vector<ViewSavedRanges> savedRanges;

private:
    std::vector<std::unique_ptr<KTextEditor::MovingRange>> ranges;
    std::vector<KTextEditor::Attribute::Ptr> attrs;
    QPointer<KTextEditor::View> m_activeView;
    KTextEditor::MainWindow *m_mainWindow;
    /**
     * Helper to ensure we always use m_lastUserColor + 1
     * for the next brackets we highlight
     */
    size_t m_lastUserColor = 0;
};

#endif
