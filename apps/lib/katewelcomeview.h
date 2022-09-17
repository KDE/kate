/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ui_welcome.h"

class KateViewSpace;

/**
 * Placeholder if a view space has no real views.
 * Allows for a nice welcome experience :P
 */
class KateWelcomeView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct new output, we do that once per main window
     * @param viewSpace parent view space
     * @param parent parent widget
     */
    KateWelcomeView(KateViewSpace *viewSpace, QWidget *parent);

public Q_SLOTS:
    /**
     * Welcome view can always be closed.
     */
    bool shouldClose()
    {
        return true;
    }

private:
    // our viewspace we belong to
    KateViewSpace *const m_viewSpace = nullptr;

    // designer UI for this widget
    Ui::WelcomeViewWidget m_ui;
};
