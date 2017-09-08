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

#include "katesessionmanagedialog.h"

#include "kateapp.h"
#include "katesessionmanager.h"
#include "katesessionchooseritem.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGuiItem>

#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QInputDialog>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

KateSessionManageDialog::KateSessionManageDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Manage Sessions"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    QHBoxLayout *hb = new QHBoxLayout();
    mainLayout->addLayout(hb);

    m_sessions = new QTreeWidget(this);
    m_sessions->setMinimumSize(400, 200);
    hb->addWidget(m_sessions);
    m_sessions->setColumnCount(2);
    QStringList header;
    header << i18n("Session Name");
    header << i18nc("The number of open documents", "Open Documents");
    m_sessions->setHeaderLabels(header);
    m_sessions->setRootIsDecorated(false);
    m_sessions->setItemsExpandable(false);
    m_sessions->setAllColumnsShowFocus(true);
    m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessions->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    updateSessionList();
    m_sessions->resizeColumnToContents(0);

    // right column buttons
    QDialogButtonBox *rightButtons = new QDialogButtonBox(this);
    rightButtons->setOrientation(Qt::Vertical);
    hb->addWidget(rightButtons);

    m_rename = new QPushButton(i18n("&Rename..."));
    connect(m_rename, SIGNAL(clicked()), this, SLOT(rename()));
    rightButtons->addButton(m_rename, QDialogButtonBox::ApplyRole);

    m_del = new QPushButton();
    KGuiItem::assign(m_del, KStandardGuiItem::del());
    connect(m_del, SIGNAL(clicked()), this, SLOT(del()));
    rightButtons->addButton(m_del, QDialogButtonBox::ApplyRole);

    // dialog buttons
    QDialogButtonBox *bottomButtons = new QDialogButtonBox(this);
    mainLayout->addWidget(bottomButtons);

    QPushButton *closeButton = new QPushButton;
    KGuiItem::assign(closeButton, KStandardGuiItem::close());
    closeButton->setDefault(true);
    bottomButtons->addButton(closeButton, QDialogButtonBox::RejectRole);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(slotClose()));

    m_openButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("&Open"));
    bottomButtons->addButton(m_openButton, QDialogButtonBox::AcceptRole);
    connect(m_openButton, SIGNAL(clicked()), this, SLOT(open()));

    // trigger action update
    selectionChanged(nullptr, nullptr);
}

KateSessionManageDialog::~KateSessionManageDialog()
{}

void KateSessionManageDialog::slotClose()
{
    done(0);
}

void KateSessionManageDialog::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    const bool validItem = (current != nullptr);

    m_rename->setEnabled(validItem);
    m_del->setEnabled(validItem && (static_cast<KateSessionChooserItem *>(current))->session != KateApp::self()->sessionManager()->activeSession());
    m_openButton->setEnabled(true);
}

void KateSessionManageDialog::rename()
{
    KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem());

    if (!item) {
        return;
    }

    bool ok = false;
    QString name = QInputDialog::getText(QApplication::activeWindow(), // nasty trick:)
                                         i18n("Specify New Name for Session"), i18n("Session name:"),
                                         QLineEdit::Normal, item->session->name(), &ok);

    if (!ok) {
        return;
    }

    if (name.isEmpty()) {
        KMessageBox::sorry(this, i18n("To save a session, you must specify a name."), i18n("Missing Session Name"));
        return;
    }

    if (KateApp::self()->sessionManager()->renameSession(item->session, name)) {
        updateSessionList();
    }
}

void KateSessionManageDialog::del()
{
    KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem());

    if (!item) {
        return;
    }

    KateApp::self()->sessionManager()->deleteSession(item->session);
    updateSessionList();
}

void KateSessionManageDialog::open()
{
    KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem());

    if (!item) {
        return;
    }

    hide();
    KateApp::self()->sessionManager()->activateSession(item->session);
    done(0);
}

void KateSessionManageDialog::updateSessionList()
{
    m_sessions->clear();

    KateSessionList slist = KateApp::self()->sessionManager()->sessionList();
    qSort(slist.begin(), slist.end(), KateSession::compareByName);

    foreach(const KateSession::Ptr & session, slist) {
        new KateSessionChooserItem(m_sessions, session);
    }
}

