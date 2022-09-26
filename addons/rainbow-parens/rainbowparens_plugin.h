/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef RAINBOW_PARENS_PLUGIN_H
#define RAINBOW_PARENS_PLUGIN_H

#include <KTextEditor/ConfigPage>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Plugin>
#include <KWidgetsAddons/KColorButton>
#include <KXMLGUIClient>

#include <QPointer>

#include <array>

class RainbowParenPlugin final : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit RainbowParenPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    int configPages() const override
    {
        return 1;
    }

    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    const std::vector<KTextEditor::Attribute::Ptr> colorsList() const
    {
        return attrs;
    }

    void readConfig();

Q_SIGNALS:
    void configUpdated();

private:
    std::vector<KTextEditor::Attribute::Ptr> attrs;
};

class RainbowParenPluginView final : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit RainbowParenPluginView(RainbowParenPlugin *plugin, KTextEditor::MainWindow *mainwindow);

    void rehighlight(KTextEditor::View *view);
    void viewChanged(KTextEditor::View *view);

    Q_SLOT void clearRanges(KTextEditor::Document *doc);
    Q_SLOT void clearSavedRangesForDoc(KTextEditor::Document *doc);

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
    /**
     * Helper to ensure we always use m_lastUserColor + 1
     * for the next brackets we highlight
     */
    size_t m_lastUserColor = 0;
};

class RainbowParenConfigPage final : public KTextEditor::ConfigPage
{
    Q_OBJECT
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
};

#endif
