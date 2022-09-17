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
 * The KateViewManager will ensure we instantiate the welcome view when needed and remove it later on.
 */
class KateWelcomeView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct new output, we do that once per main window
     */
    KateWelcomeView();

public Q_SLOTS:
    /**
     * Welcome view can always be closed.
     * @return can always be closed
     */
    bool shouldClose()
    {
        return true;
    }

private:
    /**
     * Get the view space we belong to.
     * As we can in principle be dragged around.
     * @return view space we belong to
     */
    KateViewSpace *viewSpace();

private:
    // designer UI for this widget
    Ui::WelcomeViewWidget m_ui;
};
