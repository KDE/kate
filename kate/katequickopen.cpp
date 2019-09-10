/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
    ---
    Copyright (C) 2007,2009 Joseph Wenninger <jowenn@kde.org>
*/

#include "katequickopen.h"
#include "katequickopenmodel.h"

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kateapp.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <tuple>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KLineEdit>
#include <KActionCollection>
#include <KLocalizedString>

#include <QEvent>
#include <QFileInfo>
#include <QSortFilterProxyModel>
#include <QCoreApplication>
#include <QPointer>
#include <QStandardItemModel>
#include <QDesktopWidget>
#include <QBoxLayout>
#include <QLabel>
#include <QTreeView>
#include <QHeaderView>

Q_DECLARE_METATYPE(QPointer<KTextEditor::Document>)

KateQuickOpen::KateQuickOpen(QWidget *parent, KateMainWindow *mainWindow)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_inputLine = new KLineEdit();
    setFocusProxy(m_inputLine);
    m_inputLine->setPlaceholderText(i18n("Quick Open Search"));

    layout->addWidget(m_inputLine);

    m_listView = new QTreeView();
    layout->addWidget(m_listView, 1);
    m_listView->setTextElideMode(Qt::ElideLeft);

    m_base_model = new KateQuickOpenModel(m_mainWindow, this);

    m_model = new QSortFilterProxyModel(this);
    m_model->setFilterRole(Qt::DisplayRole);
    m_model->setSortRole(Qt::DisplayRole);
    m_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_model->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_model->setFilterKeyColumn(0);

    connect(m_inputLine, &KLineEdit::textChanged, m_model, &QSortFilterProxyModel::setFilterWildcard);
    connect(m_inputLine, &KLineEdit::returnPressed, this, &KateQuickOpen::slotReturnPressed);
    connect(m_model, &QSortFilterProxyModel::rowsInserted, this, &KateQuickOpen::reselectFirst);
    connect(m_model, &QSortFilterProxyModel::rowsRemoved, this, &KateQuickOpen::reselectFirst);

    connect(m_listView, &QTreeView::activated, this, &KateQuickOpen::slotReturnPressed);

    m_listView->setModel(m_model);
    m_model->setSourceModel(m_base_model);

    m_inputLine->installEventFilter(this);
    m_listView->installEventFilter(this);
    m_listView->setHeaderHidden(true);
    m_listView->setRootIsDecorated(false);
}

bool KateQuickOpen::eventFilter(QObject *obj, QEvent *event)
{
    // catch key presses + shortcut overrides to allow to have ESC as application wide shortcut, too, see bug 409856
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == m_inputLine) {
            const bool forward2list = (keyEvent->key() == Qt::Key_Up)
                                      || (keyEvent->key() == Qt::Key_Down)
                                      || (keyEvent->key() == Qt::Key_PageUp)
                                      || (keyEvent->key() == Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_listView, event);
                return true;
            }

            if (keyEvent->key() == Qt::Key_Escape) {
                m_mainWindow->slotWindowActivated();
                m_inputLine->clear();
                keyEvent->accept();
                return true;
            }
        } else {
            const bool forward2input = (keyEvent->key() != Qt::Key_Up)
                                       && (keyEvent->key() != Qt::Key_Down)
                                       && (keyEvent->key() != Qt::Key_PageUp)
                                       && (keyEvent->key() != Qt::Key_PageDown)
                                       && (keyEvent->key() != Qt::Key_Tab)
                                       && (keyEvent->key() != Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_inputLine, event);
                return true;
            }
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_inputLine->hasFocus() || m_listView->hasFocus())) {
        m_mainWindow->slotWindowActivated();
        m_inputLine->clear();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void KateQuickOpen::reselectFirst()
{
    int first = 0;
    if (m_mainWindow->viewManager()->sortedViews().size() > 1)
        first = 1;

    QModelIndex index = m_model->index(first, 0);
    m_listView->setCurrentIndex(index);
}

void KateQuickOpen::update()
{
    m_base_model->refresh();
    m_listView->resizeColumnToContents(0);

    // If we have a very long file name we restrict the size of the first column
    // to take at most half of the space. Otherwise it would look odd.
    int colw0 = m_listView->header()->sectionSize(0); // file name
    int colw1 = m_listView->header()->sectionSize(1); // file path
    if (colw0 > colw1) {
        m_listView->setColumnWidth(0, (colw0 + colw1) / 2);
    }
    reselectFirst();
}

void KateQuickOpen::slotReturnPressed()
{
    const auto index = m_listView->model()->index(m_listView->currentIndex().row(), KateQuickOpenModel::Columns::FilePath);
    auto url = index.data(Qt::UserRole).toUrl();
    m_mainWindow->wrapper()->openUrl(url);
    m_mainWindow->slotWindowActivated();
    m_inputLine->clear();
}


void KateQuickOpen::setMatchMode(int mode)
{
    m_model->setFilterKeyColumn(mode);
}

int KateQuickOpen::matchMode()
{
    return m_model->filterKeyColumn();
}

void KateQuickOpen::setListMode(KateQuickOpenModel::List mode)
{
    m_base_model->setListMode(mode);
}

KateQuickOpenModel::List KateQuickOpen::listMode() const
{
    return m_base_model->listMode();
}
