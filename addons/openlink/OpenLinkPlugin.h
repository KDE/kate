/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Cursor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QPointer>
#include <QVariant>

#include <unordered_map>

namespace KTextEditor
{
class MovingRange;
}
class KateTextHintProvider;
using MovingRangeList = std::vector<std::unique_ptr<KTextEditor::MovingRange>>;

class OpenLinkPlugin final : public KTextEditor::Plugin
{
public:
    explicit OpenLinkPlugin(QObject *parent)
        : KTextEditor::Plugin(parent)
    {
    }
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class OpenLinkPluginView final : public QObject, public KXMLGUIClient
{
public:
    explicit OpenLinkPluginView(OpenLinkPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~OpenLinkPluginView();

private:
    void onActiveViewChanged(KTextEditor::View *);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void highlightIfLink(KTextEditor::Cursor c, QWidget *viewInternal);
    void gotoLink();
    void onTextRemoved(KTextEditor::Document *, KTextEditor::Range range, const QString &text);
    void onTextInserted(KTextEditor::Document *, KTextEditor::Cursor pos, const QString &text);
    void onViewScrolled();
    void highlightLinks(KTextEditor::Range range);
    void clear(KTextEditor::Document *doc);

    QPointer<KTextEditor::View> m_activeView;
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<class GotoLinkHover> m_ctrlHoverFeedback;
    std::unordered_map<KTextEditor::Document *, MovingRangeList> m_docHighligtedLinkRanges;
    class OpenLinkTextHint *m_textHintProvider;
    friend class OpenLinkTextHint;
};
