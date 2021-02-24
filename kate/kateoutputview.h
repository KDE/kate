/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __KATE_OUTPUT_VIEW_H__
#define __KATE_OUTPUT_VIEW_H__

#include <QWidget>

class KateMainWindow;

/**
 * Widget to output stuff e.g. for plugins.
 */
class KateOutputView : public QWidget
{
    Q_OBJECT

public:
    KateOutputView(KateMainWindow *mainWindow, QWidget *parent);

private:
    /**
     * the main window we belong to
     * each main window has exactly one KateOutputView
     */
    KateMainWindow *const m_mainWindow = nullptr;
};

#endif
