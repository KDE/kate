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

#include "katesessionchooser.h"

#include "kateapp.h"
#include "katesessionmanager.h"
#include "katesessionchooseritem.h"
#include "katedebug.h"

#include <KLocalizedString>
#include <KStandardGuiItem>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>

//BEGIN CHOOSER DIALOG

KateSessionChooser::KateSessionChooser(QWidget *parent, const QString &lastSession)
    : QDialog(parent)
{
    setWindowTitle(i18n("Session Chooser"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_sessions = new QTreeWidget(this);
    m_sessions->setMinimumSize(400, 200);
    mainLayout->addWidget(m_sessions);
    QStringList header;
    header << i18n("Session Name");
    header << i18nc("The number of open documents", "Open Documents");
    header << QString();
    m_sessions->setHeaderLabels(header);
    m_sessions->header()->setStretchLastSection(false);
    m_sessions->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_sessions->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_sessions->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_sessions->header()->resizeSection(2, 32);
    m_sessions->setRootIsDecorated(false);
    m_sessions->setItemsExpandable(false);
    m_sessions->setAllColumnsShowFocus(true);
    m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessions->setSelectionMode(QAbstractItemView::SingleSelection);

    qCDebug(LOG_KATE) << "Last session is:" << lastSession;

    KateSessionList slist = KateApp::self()->sessionManager()->sessionList();
    qSort(slist.begin(), slist.end(), KateSession::compareByName);

    foreach(const KateSession::Ptr & session, slist) {
        KateSessionChooserItem *item = new KateSessionChooserItem(m_sessions, session);
        QPushButton *tmp = new QPushButton(QIcon::fromTheme(QStringLiteral("document")), QString(), m_sessions);
        QMenu *popup = new QMenu(tmp);
        QAction *a = popup->addAction(i18n("Clone session settings"));
        a->setData(QVariant::fromValue((void *)item));
        connect(a, SIGNAL(triggered()), this, SLOT(slotCopySession()));
        a = popup->addAction(i18n("Delete this session"));
        a->setData(QVariant::fromValue((void *)item));
        connect(a, SIGNAL(triggered()), this, SLOT(slotDeleteSession()));
        tmp->setMenu(popup);
        m_sessions->setItemWidget(item, 2, tmp);

        if (session->name() == lastSession) {
            m_sessions->setCurrentItem(item);
        }
    }

    connect(m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(slotOpen()));

    // bottom box
    QHBoxLayout *hb = new QHBoxLayout();
    hb->setMargin(0);
    mainLayout->addLayout(hb);

    m_useLast = new QCheckBox(i18n("&Always use this choice"), this);
    hb->addWidget(m_useLast);

    // buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    hb->addWidget(buttonBox);

    QPushButton *cancelButton = new QPushButton();
    KGuiItem::assign(cancelButton, KStandardGuiItem::quit());
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()));
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    m_openButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open Session"));
    m_openButton->setEnabled(m_sessions->currentIndex().isValid());
    m_openButton->setDefault(true);
    m_openButton->setFocus();
    buttonBox->addButton(m_openButton, QDialogButtonBox::ActionRole);
    connect(m_openButton, SIGNAL(clicked()), this, SLOT(slotOpen()));

    QPushButton *newButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-new")), i18n("New Session"));
    buttonBox->addButton(newButton, QDialogButtonBox::ActionRole);
    connect(newButton, SIGNAL(clicked()), this, SLOT(slotNew()));

    setResult(resultNone);
    selectionChanged(nullptr, nullptr);
}

KateSessionChooser::~KateSessionChooser()
{}

void KateSessionChooser::slotCopySession()
{
    m_sessions->setCurrentItem((KateSessionChooserItem *)((QAction *)sender())->data().value<void *>());
    Q_ASSERT(static_cast<KateSessionChooserItem *>(m_sessions->currentItem()));
    done(resultCopy);
}

void KateSessionChooser::slotDeleteSession()
{
    KateSessionChooserItem *item = (KateSessionChooserItem *)((QAction *)sender())->data().value<void *>();
    if (!item) {
        return;
    }

    KateApp::self()->sessionManager()->deleteSession(item->session);
    m_sessions->removeItemWidget(item, 2);
    delete item;

}

KateSession::Ptr KateSessionChooser::selectedSession()
{
    KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem());

    Q_ASSERT(item || ((result() != resultOpen) && (result() != resultCopy)));

    if (!item) {
        return KateSession::Ptr();
    }

    return item->session;
}

bool KateSessionChooser::reopenLastSession()
{
    return m_useLast->isChecked();
}

void KateSessionChooser::slotOpen()
{
    Q_ASSERT(static_cast<KateSessionChooserItem *>(m_sessions->currentItem()));
    done(resultOpen);
}

void KateSessionChooser::slotNew()
{
    done(resultNew);
}

void KateSessionChooser::slotCancel()
{
    done(resultQuit);
}

void KateSessionChooser::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    Q_UNUSED(current);
    m_openButton->setEnabled(true);
}

//END CHOOSER DIALOG

