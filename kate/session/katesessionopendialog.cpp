/* This file is part of the KDE project
 *
 *  Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katesessionopendialog.h"

#include "kateapp.h"
#include "katesessionmanager.h"
#include "katesessionchooseritem.h"

#include <KLocalizedString>
#include <KStandardGuiItem>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

KateSessionOpenDialog::KateSessionOpenDialog(QWidget *parent)
    : QDialog(parent)

{
    setWindowTitle(i18n("Open Session"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    m_sessions = new QTreeWidget(this);
    m_sessions->setMinimumSize(400, 200);
    mainLayout->addWidget(m_sessions);

    QStringList header;
    header << i18n("Session Name");
    header << i18nc("The number of open documents", "Open Documents");
    m_sessions->setHeaderLabels(header);
    m_sessions->setRootIsDecorated(false);
    m_sessions->setItemsExpandable(false);
    m_sessions->setAllColumnsShowFocus(true);
    m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessions->setSelectionMode(QAbstractItemView::SingleSelection);

    KateSessionList slist = KateApp::self()->sessionManager()->sessionList();
    qSort(slist.begin(), slist.end(), KateSession::compareByName);

    foreach(const KateSession::Ptr & session, slist) {
        new KateSessionChooserItem(m_sessions, session);
    }

    m_sessions->resizeColumnToContents(0);

    connect(m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(slotOpen()));

    // buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    mainLayout->addWidget(buttons);

    QPushButton *cancelButton = new QPushButton;
    KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(slotCanceled()));
    buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);

    m_openButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("&Open"));
    m_openButton->setDefault(true);
    m_openButton->setEnabled(false);
    connect(m_openButton, SIGNAL(clicked()), this, SLOT(slotOpen()));
    buttons->addButton(m_openButton, QDialogButtonBox::AcceptRole);

    setResult(resultCancel);
}

KateSessionOpenDialog::~KateSessionOpenDialog()
{}

KateSession::Ptr KateSessionOpenDialog::selectedSession()
{
    KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem());

    if (!item) {
        return KateSession::Ptr();
    }

    return item->session;
}

void KateSessionOpenDialog::slotCanceled()
{
    done(resultCancel);
}

void KateSessionOpenDialog::slotOpen()
{
    done(resultOk);
}

void KateSessionOpenDialog::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    Q_UNUSED(current);
    m_openButton->setEnabled(true);
}

