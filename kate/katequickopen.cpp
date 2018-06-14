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

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kateapp.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

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

Q_DECLARE_METATYPE(QPointer<KTextEditor::Document>)

static const int DocumentRole = Qt::UserRole + 1;
static const int UrlRole = Qt::UserRole + 2;
static const int SortFilterRole = Qt::UserRole + 3;

KateQuickOpen::KateQuickOpen(QWidget *parent, KateMainWindow *mainWindow)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);

    m_inputLine = new KLineEdit();
    setFocusProxy(m_inputLine);
    m_inputLine->setPlaceholderText(i18n("Quick Open Search"));

    layout->addWidget(m_inputLine);

    m_listView = new QTreeView();
    layout->addWidget(m_listView, 1);
    m_listView->setTextElideMode(Qt::ElideLeft);

    m_base_model = new QStandardItemModel(0, 2, this);

    m_model = new QSortFilterProxyModel(this);
    m_model->setFilterRole(SortFilterRole);
    m_model->setSortRole(SortFilterRole);
    m_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_model->setSortCaseSensitivity(Qt::CaseInsensitive);

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
    if (event->type() == QEvent::KeyPress) {
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
    QModelIndex index = m_model->index(0, 0);
    m_listView->setCurrentIndex(index);
}

void KateQuickOpen::update()
{
    /**
     * new base mode creation
     */
    m_base_model->clear();

    /**
     * remember local file names to avoid dupes with project files
     */
    QSet<QString> alreadySeenFiles;
    QSet<KTextEditor::Document *> alreadySeenDocs;

    /**
     * get views in lru order
     */
    const QList<KTextEditor::View *> sortedViews(m_mainWindow->viewManager()->sortedViews());

    /**
     * now insert them in order
     */
    QModelIndex idxToSelect;
    int linecount = 0;
    foreach (KTextEditor::View *view, sortedViews) {
        KTextEditor::Document *doc = view->document();

        if (alreadySeenDocs.contains(doc)) {
            continue;
        }

        alreadySeenDocs.insert(doc);

        //QStandardItem *item=new QStandardItem(i18n("%1: %2",doc->documentName(),doc->url().toString()));
        QStandardItem *itemName = new QStandardItem(doc->documentName());

        itemName->setData(qVariantFromValue(QPointer<KTextEditor::Document> (doc)), DocumentRole);
        itemName->setData(QString::fromLatin1("%1: %2").arg(doc->documentName()).arg(doc->url().toString()), SortFilterRole);
        itemName->setEditable(false);
        QFont font = itemName->font();
        font.setBold(true);
        itemName->setFont(font);

        QStandardItem *itemUrl = new QStandardItem(doc->url().toString());
        itemUrl->setEditable(false);
        m_base_model->setItem(linecount, 0, itemName);
        m_base_model->setItem(linecount, 1, itemUrl);
        linecount++;

        if (!doc->url().isEmpty() && doc->url().isLocalFile()) {
            alreadySeenFiles.insert(doc->url().toLocalFile());
        }

        // select second document, that is the last used (beside the active one)
        if (linecount == 2) {
            idxToSelect = itemName->index();
        }
    }

    /**
     * get all open documents
     */
    QList<KTextEditor::Document *> docs = KateApp::self()->documentManager()->documentList();
    foreach(KTextEditor::Document * doc, docs) {
        /**
         * skip docs already open
         */
        if (alreadySeenDocs.contains(doc)) {
            continue;
        }

        //QStandardItem *item=new QStandardItem(i18n("%1: %2",doc->documentName(),doc->url().toString()));
        QStandardItem *itemName = new QStandardItem(doc->documentName());

        itemName->setData(qVariantFromValue(QPointer<KTextEditor::Document> (doc)), DocumentRole);
        itemName->setData(QString::fromLatin1("%1: %2").arg(doc->documentName()).arg(doc->url().toString()), SortFilterRole);
        itemName->setEditable(false);
        QFont font = itemName->font();
        font.setBold(true);
        itemName->setFont(font);

        QStandardItem *itemUrl = new QStandardItem(doc->url().toString());
        itemUrl->setEditable(false);
        m_base_model->setItem(linecount, 0, itemName);
        m_base_model->setItem(linecount, 1, itemUrl);
        linecount++;

        if (!doc->url().isEmpty() && doc->url().isLocalFile()) {
            alreadySeenFiles.insert(doc->url().toLocalFile());
        }
    }

    /**
     * insert all project files, if any project around
     */
    if (QObject *projectView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"))) {
        QStringList projectFiles = projectView->property("projectFiles").toStringList();
        foreach(const QString & file, projectFiles) {
            /**
             * skip files already open
             */
            if (alreadySeenFiles.contains(file)) {
                continue;
            }

            QFileInfo fi(file);
            QStandardItem *itemName = new QStandardItem(fi.fileName());

            itemName->setData(qVariantFromValue(QUrl::fromLocalFile(file)), UrlRole);
            itemName->setData(QString::fromLatin1("%1: %2").arg(fi.fileName()).arg(file), SortFilterRole);
            itemName->setEditable(false);

            QStandardItem *itemUrl = new QStandardItem(file);
            itemUrl->setEditable(false);
            m_base_model->setItem(linecount, 0, itemName);
            m_base_model->setItem(linecount, 1, itemUrl);
            linecount++;
        }
    }

    if (idxToSelect.isValid()) {
        m_listView->setCurrentIndex(m_model->mapFromSource(idxToSelect));
    } else {
        reselectFirst();
    }

    /**
     * adjust view
     */
    m_listView->resizeColumnToContents(0);
}

void KateQuickOpen::slotReturnPressed()
{
    /**
     * open document for first element, if possible
     * prefer to use the document pointer
     */
    // our data is in column 0 (clicking on column 1 results in no data, therefore, create new index)
    const QModelIndex index = m_listView->model()->index(m_listView->currentIndex().row(), 0);
    KTextEditor::Document *doc = index.data(DocumentRole).value<QPointer<KTextEditor::Document> >();
    if (doc) {
        m_mainWindow->wrapper()->activateView(doc);
    } else {
        QUrl url = index.data(UrlRole).value<QUrl>();
        if (!url.isEmpty()) {
            m_mainWindow->wrapper()->openUrl(url);
        }
    }

    /**
     * in any case, switch back to view manager
     */
    m_mainWindow->slotWindowActivated();
    m_inputLine->clear();
}
