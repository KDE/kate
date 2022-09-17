/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katewelcomeview.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kateviewspace.h"

#include <QPushButton>
#include <QTimer>

KateWelcomeView::KateWelcomeView()
{
    // setup ui & ensure proper title for tab & co.
    m_ui.setupUi(this);
    setWindowTitle(i18n("Welcome"));

    // new file action, view manager will trigger close of the welcome view
    m_ui.fileNew->setText(i18n("New File"));
    m_ui.fileNew->setIcon(QIcon::fromTheme(QStringLiteral("file-new")));
    connect(m_ui.fileNew, &QPushButton::clicked, this, [this]() {
        // we ensure that we use the current view space
        viewSpace()->viewManager()->slotDocumentNew();
    });

    // open file action, view manager will trigger close of the welcome view
    m_ui.fileOpen->setText(i18n("Open File..."));
    m_ui.fileOpen->setIcon(QIcon::fromTheme(QStringLiteral("file-open")));
    connect(m_ui.fileOpen, &QPushButton::clicked, this, [this]() {
        // we ensure that we use the current view space
        viewSpace()->viewManager()->slotDocumentOpen();
    });
}

KateViewSpace *KateWelcomeView::viewSpace()
{
    // this view can be re-parented, search upwards the current view space
    auto p = parent();
    while (p) {
        if (auto vs = qobject_cast<KateViewSpace *>(p)) {
            return vs;
        }
        p = p->parent();
    }
    Q_ASSERT(false);
    return nullptr;
}
