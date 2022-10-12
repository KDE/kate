/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WELCOMEVIEW_H
#define WELCOMEVIEW_H

#include "ui_welcomeview.h"

class KateViewManager;
class RecentFilesModel;
class SavedSessionsModel;

class WelcomeView : public QScrollArea, Ui::WelcomeView
{
    Q_OBJECT

public:
    explicit WelcomeView(KateViewManager *viewManager, QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void onPluginViewChanged(const QString &pluginName = QString());
    void onRecentFilesContextMenuRequested(const QPoint &pos);

private:
    bool updateLayout();

    KateViewManager *m_viewManager = nullptr;
    RecentFilesModel *m_recentFilesModel = nullptr;
    SavedSessionsModel *m_savedSessionsModel = nullptr;
};

#endif // WELCOMEVIEW_H
