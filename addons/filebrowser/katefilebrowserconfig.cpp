/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

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
*/

#include "katefilebrowserconfig.h"

#include "katefilebrowser.h"

#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QListWidget>
#include <QRegExp>
#include <QStyle>
#include <QVBoxLayout>

#include <KActionCollection>
#include <KActionSelector>
#include <KConfigGroup>
#include <KDirOperator>
#include <KLocalizedString>
#include <KSharedConfig>

//BEGIN ACtionLBItem
/*
  QListboxItem that can store and return a string,
  used for the toolbar action selector.
*/
class ActionLBItem : public QListWidgetItem
{
  public:
    ActionLBItem( QListWidget *lb = nullptr,
                  const QIcon &pm = QIcon(),
                  const QString &text = QString(),
                  const QString &str = QString() ) :
        QListWidgetItem(pm, text, lb, 0 ),
        _str(str)
    {}
    QString idstring()
    {
      return _str;
    }
  private:
    QString _str;
};
//END ActionLBItem



//BEGIN KateFileBrowserConfigPage
KateFileBrowserConfigPage::KateFileBrowserConfigPage( QWidget *parent, KateFileBrowser *kfb )
    : KTextEditor::ConfigPage( parent ),
    fileBrowser( kfb ),
    m_changed( false )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  int spacing =  QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
  lo->setSpacing( spacing );

  // Toolbar - a lot for a little...
  QGroupBox *gbToolbar = new QGroupBox(i18n("Toolbar"), this );
  acSel = new KActionSelector( gbToolbar );
  acSel->setAvailableLabel( i18n("A&vailable actions:") );
  acSel->setSelectedLabel( i18n("S&elected actions:") );

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(acSel);
  gbToolbar->setLayout(vbox);

  lo->addWidget( gbToolbar );
  connect(acSel, &KActionSelector::added, this, &KateFileBrowserConfigPage::slotMyChanged);
  connect(acSel, &KActionSelector::removed, this, &KateFileBrowserConfigPage::slotMyChanged);
  connect(acSel, &KActionSelector::movedUp, this, &KateFileBrowserConfigPage::slotMyChanged);
  connect(acSel, &KActionSelector::movedDown, this, &KateFileBrowserConfigPage::slotMyChanged);

  // make it look nice
  lo->addStretch( 1 );

  init();
}

QString KateFileBrowserConfigPage::name() const
{
  return i18n("Filesystem Browser");
}

QString KateFileBrowserConfigPage::fullName() const
{
  return i18n("Filesystem Browser Settings");
}

QIcon KateFileBrowserConfigPage::icon() const
{
  return QIcon::fromTheme(QStringLiteral("document-open"));
}

void KateFileBrowserConfigPage::apply()
{
  if ( ! m_changed )
    return;

  m_changed = false;

  KConfigGroup config(KSharedConfig::openConfig(), "filebrowser");
  QStringList l;
  ActionLBItem *aItem;
  QList<QListWidgetItem *> list = acSel->selectedListWidget()->findItems(QStringLiteral("*"), Qt::MatchWildcard);
  foreach(QListWidgetItem *item, list)
  {
    aItem = static_cast<ActionLBItem*>(item);
    l << aItem->idstring();
  }
  config.writeEntry( "toolbar actions", l );

  fileBrowser->setupToolbar();
}

void KateFileBrowserConfigPage::reset()
{
  // hmm, what is this supposed to do, actually??
  init();
  m_changed = false;
}

void KateFileBrowserConfigPage::init()
{
  KConfigGroup config(KSharedConfig::openConfig(), "filebrowser");
  // toolbar
  QStringList l = config.readEntry( "toolbar actions", QStringList() );
  if ( l.isEmpty() ) // default toolbar
    l << QStringLiteral("back") << QStringLiteral("forward") << QStringLiteral("bookmarks") << QStringLiteral("sync_dir") << QStringLiteral("configure");

  // actions from diroperator + two of our own
  QStringList allActions;
  allActions << QStringLiteral("up") << QStringLiteral("back") << QStringLiteral("forward") << QStringLiteral("home")
             << QStringLiteral("reload") << QStringLiteral("mkdir") << QStringLiteral("delete")
             << QStringLiteral("short view") << QStringLiteral("detailed view")
             << QStringLiteral("tree view") << QStringLiteral("detailed tree view")
             << QStringLiteral("show hidden") /*<< QStringLiteral("view menu") << QStringLiteral("properties")*/
             << QStringLiteral("bookmarks") << QStringLiteral("sync_dir") << QStringLiteral("configure");
  QRegExp re(QStringLiteral ("&(?=[^&])"));
  QAction *ac = nullptr;
  QListWidget *lb;
  for ( QStringList::Iterator it = allActions.begin(); it != allActions.end(); ++it )
  {
    lb = l.contains( *it ) ? acSel->selectedListWidget() : acSel->availableListWidget();

    if ( *it == QStringLiteral ("bookmarks") || *it == QStringLiteral ("sync_dir") || *it == QStringLiteral ("configure") )
      ac = fileBrowser->actionCollection()->action( *it );
    else
      ac = fileBrowser->dirOperator()->actionCollection()->action( *it );

    if ( ac )
    {
      QString text = ac->text().remove( re );
      // CJK languages need a filtering message for action texts in lists,
      // to remove special accelerators that they use.
      // The exact same filtering message exists in kdelibs; hence,
      // avoid extraction here and let it be sourced from kdelibs.
      #define i18ncX i18nc
      text = i18ncX( "@item:intable Action name in toolbar editor", "%1", text );
      new ActionLBItem( lb, ac->icon(), text, *it );
    }
  }
}

void KateFileBrowserConfigPage::slotMyChanged()
{
  m_changed = true;
  emit changed();
}
//END KateFileBrowserConfigPage

// kate: space-indent on; indent-width 2; replace-tabs on;
