/*

    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katesessionmanagedialog.h"

#include "kateapp.h"
#include "katesessionchooseritem.h"
#include "katesessionmanager.h"

#include <KColorScheme>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KStandardGuiItem>

#include <QKeyEvent>
#include <QPointer>
#include <QTimer>

KateSessionManageDialog::KateSessionManageDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(i18n("Manage Sessions"));
    m_dontAskCheckBox->hide();

    m_sessionList->installEventFilter(this);
    connect(m_sessionList, &QTreeWidget::currentItemChanged, this, &KateSessionManageDialog::selectionChanged);
    connect(m_sessionList, &QTreeWidget::itemDoubleClicked, this, &KateSessionManageDialog::openSession);
    m_sessionList->header()->moveSection(0, 1); // Re-order columns to "Files, Sessions"

    m_filterBox->installEventFilter(this);
    connect(m_filterBox, &QLineEdit::textChanged, this, &KateSessionManageDialog::filterChanged);

    m_newButton->setShortcut(QKeySequence::New);
    connect(m_newButton, &QPushButton::clicked, this, &KateSessionManageDialog::openNewSession);

    KGuiItem::assign(m_openButton, KStandardGuiItem::open());
    m_openButton->setDefault(true);
    m_openButton->setShortcut(QKeySequence::Open);
    connect(m_openButton, &QPushButton::clicked, this, &KateSessionManageDialog::openSession);

    connect(m_templateButton, &QPushButton::clicked, this, &KateSessionManageDialog::openSessionAsTemplate);

    connect(m_copyButton, &QPushButton::clicked, this, &KateSessionManageDialog::copySession);

    connect(m_renameButton, &QPushButton::clicked, this, &KateSessionManageDialog::editBegin);

    connect(m_deleteButton, &QPushButton::clicked, this, &KateSessionManageDialog::updateDeleteList);

    KGuiItem::assign(m_closeButton, KStandardGuiItem::close());
    m_closeButton->setShortcut(QKeySequence::Close);
    connect(m_closeButton, &QPushButton::clicked, this, &KateSessionManageDialog::closeDialog);

    connect(KateApp::self()->sessionManager(), &KateSessionManager::sessionListChanged, this, &KateSessionManageDialog::updateSessionList);

    updateSessionList();
}

KateSessionManageDialog::KateSessionManageDialog(QWidget *parent, const QString &lastSession)
    : KateSessionManageDialog(parent)
{
    setWindowTitle(i18n("Session Chooser"));
    m_dontAskCheckBox->show();
    m_chooserMode = true;
    connect(m_dontAskCheckBox, &QCheckBox::toggled, this, &KateSessionManageDialog::dontAskToggled);

    m_preferredSession = lastSession;
    updateSessionList();
}

void KateSessionManageDialog::dontAskToggled()
{
    m_templateButton->setEnabled(!m_dontAskCheckBox->isChecked());
}

void KateSessionManageDialog::filterChanged()
{
    static QPointer<QTimer> delay;

    if (!delay) {
        delay = new QTimer(this); // Should be auto cleard by Qt when we die
        delay->setSingleShot(true);
        delay->setInterval(400);
        connect(delay, &QTimer::timeout, this, &KateSessionManageDialog::updateSessionList);
    }

    delay->start();
}

void KateSessionManageDialog::done(int result)
{
    for (const auto &session : std::as_const(m_deleteList)) {
        KateApp::self()->sessionManager()->deleteSession(session);
        KateApp::self()->stashManager()->clearStashForSession(session);
    }
    m_deleteList.clear(); // May not needed, but anyway

    if (ResultQuit == result) {
        QDialog::done(0);
        return;
    }

    if (m_chooserMode && m_dontAskCheckBox->isChecked()) {
        // write back our nice boolean :)
        KConfigGroup generalConfig(KSharedConfig::openConfig(), QStringLiteral("General"));
        switch (result) {
        case ResultOpen:
            generalConfig.writeEntry("Startup Session", "last");
            break;
        case ResultNew:
            generalConfig.writeEntry("Startup Session", "new");
            break;
        default:
            break;
        }
        generalConfig.sync();
    }

    QDialog::done(1);
}

void KateSessionManageDialog::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);

    if (m_editByUser) {
        editDone(); // Field was left unchanged, no need to apply
        return;
    }

    if (!current) {
        m_openButton->setEnabled(false);
        m_templateButton->setEnabled(false);
        m_copyButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        return;
    }

    const KateSession::Ptr activeSession = KateApp::self()->sessionManager()->activeSession();
    const bool notActiveSession = !KateApp::self()->sessionManager()->sessionIsActive(currentSelectedSession()->name());

    m_deleteButton->setEnabled(notActiveSession);

    if (m_deleteList.contains(currentSelectedSession())) {
        m_deleteButton->setText(i18n("Restore"));
        m_openButton->setEnabled(false);
        m_templateButton->setEnabled(false);
        m_copyButton->setEnabled(true); // Looks a little strange but is OK
        m_renameButton->setEnabled(false);
    } else {
        KGuiItem::assign(m_deleteButton, KStandardGuiItem::del());
        m_openButton->setEnabled(currentSelectedSession() != activeSession);
        m_templateButton->setEnabled(true);
        m_copyButton->setEnabled(true);
        m_renameButton->setEnabled(true);
    }
}

void KateSessionManageDialog::disableButtons()
{
    m_openButton->setEnabled(false);
    m_newButton->setEnabled(false);
    m_templateButton->setEnabled(false);
    m_dontAskCheckBox->setEnabled(false);
    m_copyButton->setEnabled(false);
    m_renameButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_closeButton->setEnabled(false);
    m_filterBox->setEnabled(false);
}

void KateSessionManageDialog::editBegin()
{
    if (m_editByUser) {
        return;
    }

    KateSessionChooserItem *item = currentSessionItem();

    if (!item) {
        return;
    }

    disableButtons();

    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_sessionList->clearSelection();
    m_sessionList->editItem(item, 0);

    // Always apply changes user did, like Dolphin
    connect(m_sessionList, &QTreeWidget::itemChanged, this, &KateSessionManageDialog::editApply);
    connect(m_sessionList->itemWidget(item, 0), &QObject::destroyed, this, &KateSessionManageDialog::editApply);

    m_editByUser = item; // Do it last to block eventFilter() actions until we are ready
}

void KateSessionManageDialog::editDone()
{
    m_editByUser = nullptr;
    disconnect(m_sessionList, &QTreeWidget::itemChanged, this, &KateSessionManageDialog::editApply);

    // avoid crash, see bug 142127
    QTimer::singleShot(0, this, &KateSessionManageDialog::updateSessionList);
    m_sessionList->setFocus();

    m_newButton->setEnabled(true);
    m_dontAskCheckBox->setEnabled(true);
    m_closeButton->setEnabled(true);
    m_filterBox->setEnabled(true);
}

void KateSessionManageDialog::editApply()
{
    if (!m_editByUser) {
        return;
    }

    KateApp::self()->sessionManager()->renameSession(m_editByUser->session, m_editByUser->text(0));
    editDone();
}

void KateSessionManageDialog::copySession()
{
    KateSessionChooserItem *item = currentSessionItem();

    if (!item) {
        return;
    }

    m_preferredSession = KateApp::self()->sessionManager()->copySession(item->session);
    m_sessionList->setFocus(); // Only needed when user abort
}

void KateSessionManageDialog::openSession()
{
    KateSessionChooserItem *item = currentSessionItem();

    if (!item) {
        return;
    }

    // ensure we don't crash when the parent window is deleted, bug 508894
    hide();
    setParent(nullptr);

    // this might fail, e.g. if session is in use, then e.g. end kate, bug 390740
    const bool success = KateApp::self()->sessionManager()->activateSession(item->session);
    done(success ? ResultOpen : ResultQuit);
}

void KateSessionManageDialog::openSessionAsTemplate()
{
    KateSessionChooserItem *item = currentSessionItem();

    if (!item) {
        return;
    }

    // ensure we don't crash when the parent window is deleted, bug 508894
    hide();
    setParent(nullptr);

    KateSessionManager *sm = KateApp::self()->sessionManager();
    KateSession::Ptr ns = KateSession::createAnonymousFrom(item->session, sm->anonymousSessionFile());
    sm->activateSession(ns);

    done(ResultOpen);
}

void KateSessionManageDialog::openNewSession()
{
    // ensure we don't crash when the parent window is deleted, bug 508894
    hide();
    setParent(nullptr);

    KateApp::self()->sessionManager()->sessionNew();
    done(ResultNew);
}

void KateSessionManageDialog::updateDeleteList()
{
    KateSessionChooserItem *item = currentSessionItem();

    if (!item) {
        return;
    }

    const KateSession::Ptr session = item->session;
    if (m_deleteList.contains(session)) {
        m_deleteList.remove(session);
        item->setForeground(0, QBrush(KColorScheme(QPalette::Active).foreground(KColorScheme::NormalText).color()));
        item->setIcon(0, QIcon());
        item->setToolTip(0, QString());
    } else {
        m_deleteList.insert(session);
        markItemAsToBeDeleted(item);
    }

    // To ease multiple deletions, move the selection
    QTreeWidgetItem *newItem = m_sessionList->itemBelow(item) ? m_sessionList->itemBelow(item) : m_sessionList->topLevelItem(0);
    m_sessionList->setCurrentItem(newItem);
    m_sessionList->setFocus();
}

void KateSessionManageDialog::markItemAsToBeDeleted(QTreeWidgetItem *item)
{
    item->setForeground(0, QBrush(KColorScheme(QPalette::Active).foreground(KColorScheme::InactiveText).color()));
    item->setIcon(0, QIcon::fromTheme(QStringLiteral("emblem-warning")));
    item->setToolTip(0, i18n("Session will be deleted on dialog close"));
}

void KateSessionManageDialog::closeDialog()
{
    done(ResultQuit);
}

void KateSessionManageDialog::updateSessionList()
{
    if (m_editByUser) {
        // Don't crash accidentally an ongoing edit
        return;
    }

    KateSession::Ptr currSelSession = currentSelectedSession();
    KateSession::Ptr activeSession = KateApp::self()->sessionManager()->activeSession();

    m_sessionList->clear();

    KateSessionList slist = KateApp::self()->sessionManager()->sessionList();

    KateSessionChooserItem *preferredItem = nullptr;
    KateSessionChooserItem *currSessionItem = nullptr;
    KateSessionChooserItem *activeSessionItem = nullptr;

    KConfigGroup generalConfig(KSharedConfig::openConfig(), QStringLiteral("General"));
    int col = generalConfig.readEntry("Session Manager Sort Column", 0);
    int order = generalConfig.readEntry("Session Manager Sort Order", (int)Qt::AscendingOrder);
    if (size_t(col) > 2) {
        col = 0;
    }
    if (order != Qt::AscendingOrder && order != Qt::DescendingOrder) {
        order = Qt::AscendingOrder;
    }

    for (const KateSession::Ptr &session : std::as_const(slist)) {
        if (!m_filterBox->text().isEmpty()) {
            if (!session->name().contains(m_filterBox->text(), Qt::CaseInsensitive)) {
                continue;
            }
        }

        auto *item = new KateSessionChooserItem(m_sessionList, session);
        if (session == currSelSession) {
            currSessionItem = item;
        } else if (session == activeSession) {
            activeSessionItem = item;
        } else if (session->name() == m_preferredSession) {
            preferredItem = item;
            m_preferredSession.clear();
        }

        if (m_deleteList.contains(session)) {
            markItemAsToBeDeleted(item);
        }
    }

    connect(m_sessionList->header(), &QHeaderView::sortIndicatorChanged, this, [](int logIdx, Qt::SortOrder order) {
        KConfigGroup generalConfig(KSharedConfig::openConfig(), QStringLiteral("General"));
        generalConfig.writeEntry("Session Manager Sort Column", logIdx);
        generalConfig.writeEntry("Session Manager Sort Order", (int)order);
    });

    m_sessionList->header()->setStretchLastSection(false);
    m_sessionList->header()->setSectionResizeMode(0, QHeaderView::Stretch); // stretch "Session Name" column
    m_sessionList->resizeColumnToContents(1); // Fit "Files" column
    m_sessionList->resizeColumnToContents(2); // Fit "Last update" column
    m_sessionList->sortByColumn(col, Qt::SortOrder(order)); // sort by "Session Name" column.. don't worry, it only sorts when the model data changes.

    if (!preferredItem) {
        preferredItem = currSessionItem ? currSessionItem : activeSessionItem;
    }

    if (preferredItem) {
        m_sessionList->setCurrentItem(preferredItem);
        m_sessionList->scrollToItem(preferredItem);
    } else if (m_sessionList->topLevelItemCount() > 0) {
        m_sessionList->setCurrentItem(m_sessionList->topLevelItem(0));
    }

    if (m_filterBox->hasFocus()) {
        return;
    }

    if (m_sessionList->topLevelItemCount() == 0) {
        m_openButton->setEnabled(false);
        m_templateButton->setEnabled(false);
        m_dontAskCheckBox->setEnabled(false);
        m_copyButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_filterBox->setEnabled(false);

        m_newButton->setFocus();
    } else {
        m_sessionList->setFocus();
    }
}

KateSessionChooserItem *KateSessionManageDialog::currentSessionItem() const
{
    return static_cast<KateSessionChooserItem *>(m_sessionList->currentItem());
}

KateSession::Ptr KateSessionManageDialog::currentSelectedSession() const
{
    KateSessionChooserItem *item = currentSessionItem();

    if (!item) {
        return KateSession::Ptr();
    }

    return item->session;
}

bool KateSessionManageDialog::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_sessionList) {
        if (!m_editByUser) { // No need for further action
            return false;
        }
        if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(event);
            switch (ke->key()) {
            // Avoid to apply changes with untypical keys/don't left edit field this way
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
                return true;
            default:
                break;
            }

        } else if (event->type() == QEvent::KeyRelease) {
            auto *ke = static_cast<QKeyEvent *>(event);
            switch (ke->key()) {
            case Qt::Key_Escape:
                editDone(); // Abort edit
                break;
            case Qt::Key_Return:
                editApply();
                break;
            default:
                break;
            }
        }

    } else if (object == m_filterBox && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        // Catch Return key to avoid to finish the dialog
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            // avoid crash, see bug 142127
            QTimer::singleShot(0, this, &KateSessionManageDialog::updateSessionList);
            m_sessionList->setFocus();
            return true;
        }
    }

    return false;
}

void KateSessionManageDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    if (m_editByUser) {
        // We must catch closeEvent here due to connected signal of QLineEdit::destroyed->editApply()->crash!
        editDone(); // editApply() don't work, m_editByUser->text(0) will not updated from QLineEdit
    }
}
