/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WELCOMEVIEW_H
#define WELCOMEVIEW_H

#include "ui_welcomeview.h"

class KateViewManager;
class Placeholder;
class RecentItemsModel;
class SavedSessionsModel;

class WelcomeView : public QScrollArea, Ui::WelcomeView
{
    Q_OBJECT

public:
    explicit WelcomeView(KateViewManager *viewManager, QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void onPluginViewChanged(const QString &pluginName = QString());
    void onRecentItemsContextMenuRequested(const QPoint &pos);
    bool shouldClose()
    {
        return true;
    }

private:
    void updateButtons();
    void updateFonts();
    bool updateLayout();

    KateViewManager *m_viewManager = nullptr;
    RecentItemsModel *m_recentItemsModel = nullptr;
    SavedSessionsModel *m_savedSessionsModel = nullptr;
    Placeholder *m_placeholderRecentItems = nullptr;
    Placeholder *m_placeholderSavedSessions = nullptr;
};

#endif // WELCOMEVIEW_H
