/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    Copyright (C) 2007, Joseph Wenninger <jowenn@kde.org>
*/

#include "plugin_katequickdocumentswitcher.h"
#include "plugin_katequickdocumentswitcher.moc"

#include <ktexteditor/document.h>

#include <kgenericfactory.h>

#include <klineedit.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <qlistview.h>
#include <qwidget.h>
#include <qboxlayout.h>
#include <qstandarditemmodel.h>
#include <qpointer.h>
#include <qevent.h>
#include <qlabel.h>
#include <qcoreapplication.h>

Q_DECLARE_METATYPE(QPointer<KTextEditor::Document>)

K_EXPORT_COMPONENT_FACTORY( katequickdocumentswitcherplugin, KGenericFactory<PluginKateQuickDocumentSwitcher>( "katequickdocumentswitcher" ) )

//BEGIN: Plugin

PluginKateQuickDocumentSwitcher::PluginKateQuickDocumentSwitcher( QObject* parent , const QStringList&):
    Kate::Plugin ( (Kate::Application *)parent, "kate-quick-document-switcher-plugin" ) {
}

PluginKateQuickDocumentSwitcher::~PluginKateQuickDocumentSwitcher() {
}

Kate::PluginView *PluginKateQuickDocumentSwitcher::createView (Kate::MainWindow *mainWindow) {
    return new PluginViewKateQuickDocumentSwitcher(mainWindow);
}

//END: Plugin

//BEGIN: View
PluginViewKateQuickDocumentSwitcher::PluginViewKateQuickDocumentSwitcher(Kate::MainWindow *mainwindow):
    Kate::PluginView(mainwindow),KXMLGUIClient() {

    QAction *a = actionCollection()->addAction("documents_quickswitch");
    a->setText(i18n("Quickswitch"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_1) );
    connect( a, SIGNAL( triggered(bool) ), this, SLOT( slotQuickSwitch() ) );

    setComponentData (KComponentData("kate"));
    setXMLFile( "plugins/katequickdocumentswitcher/ui.rc" );
    mainwindow->guiFactory()->addClient (this);

}

PluginViewKateQuickDocumentSwitcher::~PluginViewKateQuickDocumentSwitcher() {
      mainWindow()->guiFactory()->removeClient (this);
}

void PluginViewKateQuickDocumentSwitcher::slotQuickSwitch() {
    KTextEditor::Document *doc=PluginViewKateQuickDocumentSwitcherDialog::document(mainWindow()->window());
    if (doc) {
        mainWindow()->activateView(doc);
    }
}
//END: View

//BEGIN: Dialog
KTextEditor::Document *PluginViewKateQuickDocumentSwitcherDialog::document(QWidget *parent) {
    PluginViewKateQuickDocumentSwitcherDialog dlg(parent);
    if (QDialog::Accepted==dlg.exec()) {
        QModelIndex idx(dlg.m_listView->currentIndex());
        if (idx.isValid()) {
            QVariant _doc=idx.data(Qt::UserRole+1);
            QPointer<KTextEditor::Document> doc=_doc.value<QPointer<KTextEditor::Document> >();
            return (KTextEditor::Document*)doc;
        }
    }
    return 0;
}

PluginViewKateQuickDocumentSwitcherDialog::PluginViewKateQuickDocumentSwitcherDialog(QWidget *parent):
    KDialog(parent) {
    setModal(true);
    setButtons( KDialog::Ok | KDialog::Cancel);
    setButtonGuiItem( KDialog::User1 , KGuiItem("Switch to") );
    showButtonSeparator(true);
    setCaption(i18n("Document Quick Switch"));

    QWidget *mainwidget=new QWidget(this);
    
    QVBoxLayout *layout=new QVBoxLayout(mainwidget);
    layout->setSpacing(spacingHint());
    

    m_inputLine=new KLineEdit(mainwidget);
    

    QLabel *label=new QLabel(i18n("&Filter:"),this);
    label->setBuddy(m_inputLine);

    QHBoxLayout *subLayout=new QHBoxLayout();
    subLayout->setSpacing(spacingHint());

    subLayout->addWidget(label);
    subLayout->addWidget(m_inputLine,1);

    layout->addLayout(subLayout,0);

    m_listView=new QListView(mainwidget);
    layout->addWidget(m_listView,1);

    setMainWidget(mainwidget);
    m_inputLine->setFocus(Qt::OtherFocusReason);

    QStandardItemModel *base_model=new QStandardItemModel(0,1,this);
    QList<KTextEditor::Document*> docs=Kate::application()->documentManager()->documents();
    foreach(KTextEditor::Document *doc,docs) {
        QStandardItem *item=new QStandardItem(i18n("%1: %2",doc->documentName(),doc->url().prettyUrl()));
        item->setData(qVariantFromValue(QPointer<KTextEditor::Document>(doc)));
        base_model->appendRow(item);
    }

    m_model=new QSortFilterProxyModel(this);
    
    connect(m_inputLine,SIGNAL(textChanged(const QString&)),m_model,SLOT(setFilterFixedString(const QString&)));
    connect(m_model,SIGNAL(rowsInserted(const QModelIndex &,int,int)),this,SLOT(reselectFirst()));
    connect(m_model,SIGNAL(rowsRemoved(const QModelIndex &,int,int)),this,SLOT(reselectFirst()));
    m_listView->setModel(m_model);
    m_model->setSourceModel(base_model);
    reselectFirst();
    m_inputLine->installEventFilter(this);
    m_listView->installEventFilter(this);
}

bool PluginViewKateQuickDocumentSwitcherDialog::eventFilter(QObject *obj, QEvent *event) {
    if (event->type()==QEvent::KeyPress) {
        QKeyEvent *keyEvent=static_cast<QKeyEvent*>(event);
        if (obj==m_inputLine) {
            if ( (keyEvent->key()==Qt::Key_Up) || (keyEvent->key()==Qt::Key_Down) ) {
                QCoreApplication::sendEvent(m_listView,event);
                return true;
            }
        } else {
            if ( (keyEvent->key()!=Qt::Key_Up) && (keyEvent->key()!=Qt::Key_Down) && (keyEvent->key()!=Qt::Key_Tab) && (keyEvent->key()!=Qt::Key_Backtab)) {
                QCoreApplication::sendEvent(m_inputLine,event);
                return true;
            }
        }
    }
    return KDialog::eventFilter(obj,event);
}

void PluginViewKateQuickDocumentSwitcherDialog::reselectFirst() {
    QModelIndex index=m_model->index(0,0);
    m_listView->setCurrentIndex(index);
    enableButton(KDialog::Ok,index.isValid());
}

PluginViewKateQuickDocumentSwitcherDialog::~PluginViewKateQuickDocumentSwitcherDialog() {
}


//END: Dialog