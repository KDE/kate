/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Cursor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/MovingInterface>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QPointer>
#include <QVariant>

#include <unordered_map>
#include <utility>

namespace KTextEditor
{
class MovingRange;
}
using MovingRangeList = std::vector<std::unique_ptr<KTextEditor::MovingRange>>;

class OpenLinkPlugin final : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit OpenLinkPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>())
        : KTextEditor::Plugin(parent)
    {
    }
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class OpenLinkPluginView final : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit OpenLinkPluginView(OpenLinkPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~OpenLinkPluginView();

private:
    void onActiveViewChanged(KTextEditor::View *);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void highlightIfLink(KTextEditor::Cursor c, QWidget *viewInternal);
    void gotoLink();
    void onTextInserted(KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text);
    void onViewScrolled();
    void highlightLinks(KTextEditor::Cursor pos);
    Q_SLOT void clear(KTextEditor::Document *doc);

    QPointer<KTextEditor::View> m_activeView;
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<class GotoLinkHover> m_ctrlHoverFeedback;
    std::unordered_map<KTextEditor::Document *, MovingRangeList> m_docHighligtedLinkRanges;
};
