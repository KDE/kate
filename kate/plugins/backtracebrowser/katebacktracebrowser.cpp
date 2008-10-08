/* This file is part of the KDE project
   Copyright 2008 Dominik Haumann <dhaumann kde org>

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

//BEGIN Includes
#include "katebacktracebrowser.h"
#include "katebacktracebrowser.moc"

#include "btparser.h"
#include "btfileindexer.h"

#include <klocale.h>          // i18n
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <KStandardDirs>
#include <ktexteditor/view.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kfiledialog.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QTimer>
#include <QClipboard>

//END Includes

K_PLUGIN_FACTORY(KateBtBrowserFactory, registerPlugin<KateBtBrowserPlugin>();)
K_EXPORT_PLUGIN(KateBtBrowserFactory(KAboutData("katebacktracebrowserplugin","katebacktracebrowserplugin",ki18n("Backtrace Browser"), "0.1", ki18n("Browsing backtraces"), KAboutData::License_LGPL_V2)) )


KateBtBrowserPlugin* KateBtBrowserPlugin::s_self = 0L;
static QStringList fileExtensions =
    QStringList() << "*.cpp" << "*.cxx" << "*.c" << "*.cc"
                  << "*.h" << "*.hpp" << "*.hxx"
                  << "*.moc";


KateBtBrowserPlugin::KateBtBrowserPlugin( QObject* parent, const QList<QVariant>&)
  : Kate::Plugin ( (Kate::Application*)parent )
  , Kate::PluginConfigPageInterface()
  , indexer(&db)
{
  s_self = this;
  db.loadFromFile(KStandardDirs::locateLocal( "data", "kate/backtracedatabase"));
}

KateBtBrowserPlugin::~KateBtBrowserPlugin()
{
  if (indexer.isRunning()) {
    indexer.cancel();
    indexer.wait();
  }

  db.saveToFile(KStandardDirs::locateLocal( "data", "kate/backtracedatabase"));

  s_self = 0L;
}

KateBtBrowserPlugin& KateBtBrowserPlugin::self()
{
  return *s_self;
}

Kate::PluginView *KateBtBrowserPlugin::createView (Kate::MainWindow *mainWindow)
{
  KateBtBrowserPluginView* pv = new KateBtBrowserPluginView (mainWindow);
  connect(this, SIGNAL(newStatus(const QString&)),
          pv, SLOT(setStatus(const QString&)));
  pv->setStatus(i18n("Indexed files: %1", db.size()));
  return pv;
}

KateBtDatabase& KateBtBrowserPlugin::database()
{
  return db;
}

BtFileIndexer& KateBtBrowserPlugin::fileIndexer()
{
  return indexer;
}

void KateBtBrowserPlugin::startIndexer()
{
  if (indexer.isRunning()) {
    indexer.cancel();
    indexer.wait();
  }
  KConfigGroup cg(KGlobal::config(), "backtracebrowser");
  indexer.setSearchPaths(cg.readEntry("search-folders", QStringList()));
  indexer.setFilter(cg.readEntry("file-extensions", fileExtensions));
  indexer.start();
  emit newStatus(i18n("Indexing files..."));
}

uint KateBtBrowserPlugin::configPages () const
{
  return 1;
}

Kate::PluginConfigPage* KateBtBrowserPlugin::configPage(uint number, QWidget *parent, const char *name)
{
  if (number == 0) {
    return new KateBtConfigWidget(parent, name);
  }

  return 0L;
}

QString KateBtBrowserPlugin::configPageName (uint number) const
{
  if (number == 0) {
    return i18n("Backtrace Browser");
  }
  return QString();
}

QString KateBtBrowserPlugin::configPageFullName (uint number) const
{
  if (number == 0) {
    return i18n("Backtrace Browser Settings");
  }
  return QString();
}

KIcon KateBtBrowserPlugin::configPageIcon (uint number) const
{
  return KIcon("kbugbuster");
}





KateBtBrowserPluginView::KateBtBrowserPluginView(Kate::MainWindow *mainWindow)
  : Kate::PluginView(mainWindow)
  , mw(mainWindow)
{
  toolView = mainWindow->createToolView("KateBtBrowserPlugin", Kate::MainWindow::Bottom, SmallIcon("kbugbuster"), i18n("Backtrace Browser"));
  QWidget* w = new QWidget(toolView);
  setupUi(w);
  w->show();

  timer.setSingleShot(true);
  connect(&timer, SIGNAL(timeout()), this, SLOT(clearStatus()));

  connect(btnBacktrace, SIGNAL(clicked()), this, SLOT(loadFile()));
  connect(btnClipboard, SIGNAL(clicked()), this, SLOT(loadClipboard()));
  connect(btnConfigure, SIGNAL(clicked()), this, SLOT(configure()));
  connect(lstBacktrace, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(itemActivated(QTreeWidgetItem*, int)));
}

KateBtBrowserPluginView::~KateBtBrowserPluginView ()
{
  delete toolView;
}

void KateBtBrowserPluginView::readSessionConfig(KConfigBase* config, const QString& group)
{
}

void KateBtBrowserPluginView::writeSessionConfig(KConfigBase* config, const QString& group)
{
}

void KateBtBrowserPluginView::loadFile()
{
  QString url = KFileDialog::getOpenFileName(KUrl(), QString(), mw->window(), i18n("Load Backtrace"));
  QFile f(url);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString str = f.readAll();
    loadBacktrace(str);
  }
}

void KateBtBrowserPluginView::loadClipboard()
{
  QString bt = QApplication::clipboard()->text();
  loadBacktrace(bt);
}

void KateBtBrowserPluginView::loadBacktrace(const QString& bt)
{
  QList<BtInfo> infos = KateBtParser::parseBacktrace(bt);

  lstBacktrace->clear();
  foreach (const BtInfo& info, infos) {
    QTreeWidgetItem* it = new QTreeWidgetItem(lstBacktrace);
    it->setData(0, Qt::DisplayRole, QString::number(info.step));
    it->setData(0, Qt::ToolTipRole, QString::number(info.step));
    QFileInfo fi(info.filename);
    it->setData(1, Qt::DisplayRole, fi.fileName());
    it->setData(1, Qt::ToolTipRole, info.filename);

    if (info.type == BtInfo::Source) {
      it->setData(2, Qt::DisplayRole, QString::number(info.line));
      it->setData(2, Qt::ToolTipRole, QString::number(info.line));
      it->setData(2, Qt::UserRole, QVariant(info.line));
    }
    it->setData(3, Qt::DisplayRole, info.function);
    it->setData(3, Qt::ToolTipRole, info.function);

    lstBacktrace->addTopLevelItem(it);
  }
  lstBacktrace->resizeColumnToContents(0);
  lstBacktrace->resizeColumnToContents(1);
  lstBacktrace->resizeColumnToContents(2);

  if (lstBacktrace->topLevelItemCount()) {
    setStatus(i18n("Loading backtrace succeeded"));
  } else {
    setStatus(i18n("Loading backtrace failed"));
  }
}


void KateBtBrowserPluginView::configure()
{
  KateBtConfigDialog dlg(mw->window());
  dlg.exec();
}

void KateBtBrowserPluginView::itemActivated(QTreeWidgetItem* item, int column)
{
  Q_UNUSED(column);

  QVariant variant = item->data(2, Qt::UserRole);
  if (variant.isValid()) {
    int line = variant.toInt();
    QString file = QDir::fromNativeSeparators(item->data(1, Qt::ToolTipRole).toString());
    file = QDir::cleanPath(file);

    QString path = file;
    // if not absolute path + exists, try to find with index
    if (!QFile::exists(path)) {
      // try to match the backtrace forms ".*/foo/bar.txt" and "foo/bar.txt"
      static QRegExp rx1("/([^/]+)/([^/]+)$");
      int idx = rx1.indexIn(file);
      if (idx != -1) {
        file = rx1.cap(1) + '/' + rx1.cap(2);
      } else {
        static QRegExp rx2("([^/]+)/([^/]+)$");
        idx = rx2.indexIn(file);
        if (idx != -1) {
          // file is of correct form
        } else {
          kDebug() << "file patter did not match:" << file;
          setStatus(i18n("File not found: %1", file));
          return;
        }
      }
      path = KateBtBrowserPlugin::self().database().value(file);
    }

    if (!path.isEmpty() && QFile::exists(path)) {
      KUrl url(path);
      KTextEditor::View* kv = mw->openUrl(url);
      kv->setCursorPosition(KTextEditor::Cursor(line - 1, 0));
      kv->setFocus();
      setStatus(i18n("Opened file: %1", file));
    }
  } else {
    setStatus(i18n("No debugging information available"));
  }
}

void KateBtBrowserPluginView::setStatus(const QString& status)
{
  lblStatus->setText(status);
  timer.start(10*1000);
}

void KateBtBrowserPluginView::clearStatus()
{
  lblStatus->setText(QString());
}




KateBtConfigWidget::KateBtConfigWidget(QWidget* parent, const char* name)
  : Kate::PluginConfigPage(parent, name)
{
  setupUi(this);
  edtUrl->setMode(KFile::Directory);
  edtUrl->setUrl(KUrl(QDir().absolutePath()));

  reset();

  connect(btnAdd, SIGNAL(clicked()), this, SLOT(add()));
  connect(btnRemove, SIGNAL(clicked()), this, SLOT(remove()));
  connect(edtExtensions, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

  m_changed = false;
}

KateBtConfigWidget::~KateBtConfigWidget()
{
}

void KateBtConfigWidget::apply()
{
  if (m_changed) {
    QStringList sl;
    for (int i = 0; i < lstFolders->count(); ++i) {
      sl << lstFolders->item(i)->data(Qt::DisplayRole).toString();
    }
    KConfigGroup cg(KGlobal::config(), "backtracebrowser");
    cg.writeEntry("search-folders", sl);

    QString filter = edtExtensions->text();
    filter.replace(',', ' ').replace(';', ' ');
    cg.writeEntry("file-extensions", filter.split(' ', QString::SkipEmptyParts));

    KateBtBrowserPlugin::self().startIndexer();
    m_changed = false;
  }
}

void KateBtConfigWidget::reset()
{
  KConfigGroup cg(KGlobal::config(), "backtracebrowser");
  lstFolders->clear();
  lstFolders->addItems(cg.readEntry("search-folders", QStringList()));
  edtExtensions->setText(cg.readEntry("file-extensions", fileExtensions).join(" "));
}

void KateBtConfigWidget::defaults()
{
  lstFolders->clear();
  edtExtensions->setText(fileExtensions.join(" "));

  m_changed = true;
}

void KateBtConfigWidget::add()
{
  QDir url(edtUrl->lineEdit()->text());
  if (url.exists())
  if (lstFolders->findItems(url.absolutePath(), Qt::MatchExactly).size() == 0) {
    lstFolders->addItem(url.absolutePath());
    emit changed();
    m_changed = true;
  }
}

void KateBtConfigWidget::remove()
{
  QListWidgetItem* item = lstFolders->currentItem();
  if (item) {
    delete item;
    emit changed();
    m_changed = true;
  }
}

void KateBtConfigWidget::textChanged()
{
  emit changed();
  m_changed = true;
}





KateBtConfigDialog::KateBtConfigDialog(QWidget* parent)
  : KDialog(parent)
{
  setCaption(i18n("Backtrace Browser Settings"));
  setButtons(KDialog::Ok | KDialog::Cancel);

  m_configWidget = new KateBtConfigWidget(this, "kate_bt_config_widget");
  setMainWidget(m_configWidget);

  connect(this, SIGNAL(applyClicked()), m_configWidget, SLOT(apply()));
  connect(this, SIGNAL(okClicked()), m_configWidget, SLOT(apply()));
  connect(m_configWidget, SIGNAL(changed()), this, SLOT(changed()));
}

KateBtConfigDialog::~KateBtConfigDialog()
{
}

void KateBtConfigDialog::changed()
{
  enableButtonApply(true);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
