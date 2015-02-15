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
#include "katequickopen.moc"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

#include <klineedit.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <qtreeview.h>
#include <qwidget.h>
#include <qboxlayout.h>
#include <qstandarditemmodel.h>
#include <qpointer.h>
#include <qevent.h>
#include <qlabel.h>
#include <qcoreapplication.h>
#include <QDesktopWidget>
#include <QFileInfo>

Q_DECLARE_METATYPE(QPointer<KTextEditor::Document>)

const int DocumentRole=Qt::UserRole+1;
const int UrlRole=Qt::UserRole+2;
const int SortFilterRole=Qt::UserRole+3;

KateQuickOpen::KateQuickOpen(QWidget *parent, KateMainWindow *mainWindow)
    : QWidget(parent)
    , m_mainWindow (mainWindow)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout (layout);

    m_inputLine = new KLineEdit();
    setFocusProxy (m_inputLine);
    m_inputLine->setClickMessage (i18n ("Quick Open Search"));

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

    connect(m_inputLine, SIGNAL(textChanged(QString)), m_model, SLOT(setFilterWildcard(QString)));
    connect(m_inputLine, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(reselectFirst()));
    connect(m_model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(reselectFirst()));

    connect(m_listView, SIGNAL(activated(QModelIndex)), this, SLOT(slotReturnPressed()));

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
        QKeyEvent *keyEvent=static_cast<QKeyEvent*>(event);
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
              m_mainWindow->slotWindowActivated ();
              m_inputLine->clear ();
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
    return QWidget::eventFilter(obj, event);
}

void KateQuickOpen::reselectFirst() {
    QModelIndex index = m_model->index(0, 0);
    m_listView->setCurrentIndex(index);
}

void KateQuickOpen::update ()
{
  /**
   * new base mode creation
   */
  QStandardItemModel *base_model = new QStandardItemModel(0, 2, this);

  /**
   * remember local file names to avoid dupes with project files
   */
  QSet<QString> alreadySeenFiles;
  QSet<KTextEditor::Document *> alreadySeenDocs;

  /**
   * get views in lru order
   */
  QMap<qint64, KTextEditor::View *> sortedViews;
  QHashIterator<KTextEditor::View *, qint64> i(m_mainWindow->viewManager()->lruViews());
  while (i.hasNext()) {
    i.next ();
    sortedViews[i.value()] = i.key();
  }

  /**
   * now insert them in order
   */
  QModelIndex idxToSelect;
  int linecount = 0;
  QMapIterator<qint64, KTextEditor::View *> i2(sortedViews);
  while (i2.hasNext()) {
        i2.next();

        KTextEditor::Document *doc = i2.value()->document();

        if (alreadySeenDocs.contains(doc))
          continue;

        alreadySeenDocs.insert (doc);

        //QStandardItem *item=new QStandardItem(i18n("%1: %2",doc->documentName(),doc->url().pathOrUrl()));
        QStandardItem *itemName = new QStandardItem(doc->documentName());

        itemName->setData(qVariantFromValue(QPointer<KTextEditor::Document> (doc)), DocumentRole);
        itemName->setData(QString("%1: %2").arg(doc->documentName()).arg(doc->url().pathOrUrl()), SortFilterRole);
        itemName->setEditable(false);
        QFont font = itemName->font();
        font.setBold(true);
        itemName->setFont(font);

        QStandardItem *itemUrl = new QStandardItem(doc->url().pathOrUrl());
        itemUrl->setEditable(false);
        base_model->setItem(linecount, 0, itemName);
        base_model->setItem(linecount, 1, itemUrl);
        linecount++;

        if (!doc->url().isEmpty() && doc->url().isLocalFile())
          alreadySeenFiles.insert (doc->url().toLocalFile());

        // select second document, that is the last used (beside the active one)
        if (linecount == 2)
          idxToSelect = itemName->index();
    }

  /**
   * get all open documents
   */
  QList<KTextEditor::Document*> docs = Kate::application()->documentManager()->documents();
    foreach(KTextEditor::Document *doc, docs) {
        /**
         * skip docs already open
         */
        if (alreadySeenDocs.contains (doc))
          continue;

        //QStandardItem *item=new QStandardItem(i18n("%1: %2",doc->documentName(),doc->url().pathOrUrl()));
        QStandardItem *itemName = new QStandardItem(doc->documentName());

        itemName->setData(qVariantFromValue(QPointer<KTextEditor::Document> (doc)), DocumentRole);
        itemName->setData(QString("%1: %2").arg(doc->documentName()).arg(doc->url().pathOrUrl()), SortFilterRole);
        itemName->setEditable(false);
        QFont font = itemName->font();
        font.setBold(true);
        itemName->setFont(font);

        QStandardItem *itemUrl = new QStandardItem(doc->url().pathOrUrl());
        itemUrl->setEditable(false);
        base_model->setItem(linecount, 0, itemName);
        base_model->setItem(linecount, 1, itemUrl);
        linecount++;

        if (!doc->url().isEmpty() && doc->url().isLocalFile())
          alreadySeenFiles.insert (doc->url().toLocalFile());
    }

    /**
     * insert all project files, if any project around
     */
    if (Kate::PluginView *projectView = m_mainWindow->mainWindow()->pluginView ("kateprojectplugin")) {
      QStringList projectFiles = projectView->property ("projectFiles").toStringList();
      foreach (const QString &file, projectFiles) {
        /**
         * skip files already open
         */
        if (alreadySeenFiles.contains (file))
          continue;

        QFileInfo fi (file);
        QStandardItem *itemName = new QStandardItem(fi.fileName());

        itemName->setData(qVariantFromValue(KUrl::fromPath (file)), UrlRole);
        itemName->setData(QString("%1: %2").arg(fi.fileName()).arg(file), SortFilterRole);
        itemName->setEditable(false);

        QStandardItem *itemUrl = new QStandardItem(file);
        itemUrl->setEditable(false);
        base_model->setItem(linecount, 0, itemName);
        base_model->setItem(linecount, 1, itemUrl);
        linecount++;
      }
    }

    /**
     * swap models and kill old one
     */
    m_model->setSourceModel (base_model);
    delete m_base_model;
    m_base_model = base_model;

    if(idxToSelect.isValid())
        m_listView->setCurrentIndex(m_model->mapFromSource(idxToSelect));
    else
        reselectFirst();

    /**
     * adjust view
     */
    m_listView->resizeColumnToContents(0);
}

void KateQuickOpen::slotReturnPressed ()
{
  /**
   * open document for first element, if possible
   * prefer to use the document pointer
   */
  KTextEditor::Document *doc = m_listView->currentIndex().data (DocumentRole).value<QPointer<KTextEditor::Document> >();
  if (doc) {
    m_mainWindow->mainWindow()->activateView (doc);
  } else {
    KUrl url = m_listView->currentIndex().data (UrlRole).value<KUrl>();
    if (!url.isEmpty())
      m_mainWindow->mainWindow()->openUrl (url);
  }

  /**
   * in any case, switch back to view manager
   */
  m_mainWindow->slotWindowActivated ();
  m_inputLine->clear ();
}
