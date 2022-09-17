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

KateWelcomeView::KateWelcomeView(KateViewSpace *viewSpace, QWidget *parent)
    : QWidget(parent)
    , m_viewSpace(viewSpace)
{
    m_ui.setupUi(this);

    // ensure proper title for tab & co.
    setWindowTitle(i18n("Welcome"));

    // new file action, closes welcome after creating a new document
    m_ui.fileNew->setText(i18n("New File"));
    m_ui.fileNew->setIcon(QIcon::fromTheme(QStringLiteral("file-new")));
    connect(m_ui.fileNew, &QPushButton::clicked, this, [this]() {
        m_viewSpace->viewManager()->slotDocumentNew();
        QTimer::singleShot(0, this, [this]() {
            m_viewSpace->viewManager()->mainWindow()->removeWidget(this);
        });
    });

    // open file action, closes welcome after creating a new document
    m_ui.fileOpen->setText(i18n("Open File..."));
    m_ui.fileOpen->setIcon(QIcon::fromTheme(QStringLiteral("file-open")));
    connect(m_ui.fileOpen, &QPushButton::clicked, this, [this]() {
        m_viewSpace->viewManager()->slotDocumentOpen();
        QTimer::singleShot(0, this, [this]() {
            m_viewSpace->viewManager()->mainWindow()->removeWidget(this);
        });
    });
}
