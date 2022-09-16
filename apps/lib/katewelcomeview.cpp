/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katewelcomeview.h"

KateWelcomeView::KateWelcomeView(KateViewSpace *viewSpace, QWidget *parent)
    : QWidget(parent)
    , m_viewSpace(viewSpace)
{
    m_ui.setupUi(this);
}
