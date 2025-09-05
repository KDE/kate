/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KColorButton>
#include <KTextEditor/ConfigPage>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QPointer>
#include <QTimer>

#include <array>

class RainbowParenPlugin final : public KTextEditor::Plugin
{
public:
    explicit RainbowParenPlugin(QObject *parent);

    int configPages() const override
    {
        return 1;
    }

    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    const std::vector<KTextEditor::Attribute::Ptr> &colorsList() const
    {
        return attrs;
    }

    void readConfig();

private:
    std::vector<KTextEditor::Attribute::Ptr> attrs;
};

class RainbowParenPluginView final : public QObject, public KXMLGUIClient
{
public:
    explicit RainbowParenPluginView(RainbowParenPlugin *plugin, KTextEditor::MainWindow *mainwindow);

    void onTextInserted(KTextEditor::Document *doc, KTextEditor::Cursor pos, const QString &text);
    void onTextRemoved(KTextEditor::Document *doc, KTextEditor::Range range, const QString &text);
    void onScrollChanged();
    void requestRehighlight(int delay = 200);
    void rehighlight(KTextEditor::View *view);
    void viewChanged(KTextEditor::View *view);

    void clearRanges(KTextEditor::Document *doc);
    void clearSavedRangesForDoc(KTextEditor::Document *doc);

    struct SavedRanges {
        QPointer<KTextEditor::Document> doc;
        std::vector<std::unique_ptr<KTextEditor::MovingRange>> ranges;
    };
    std::vector<SavedRanges> savedRanges;

private:
    RainbowParenPlugin *const m_plugin;
    std::vector<std::unique_ptr<KTextEditor::MovingRange>> ranges;
    QPointer<KTextEditor::View> m_activeView;
    KTextEditor::MainWindow *m_mainWindow;
    QTimer m_rehighlightTimer;
    /**
     * Helper to ensure we always use m_lastUserColor + 1
     * for the next brackets we highlight
     */
    size_t m_lastUserColor = 0;
};

class RainbowParenConfigPage final : public KTextEditor::ConfigPage
{
public:
    explicit RainbowParenConfigPage(QWidget *parent, RainbowParenPlugin *plugin);

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void reset() override;
    void defaults() override;

private:
    std::array<KColorButton, 5> m_btns;
    RainbowParenPlugin *const m_plugin;
    QIcon m_icon;
};
