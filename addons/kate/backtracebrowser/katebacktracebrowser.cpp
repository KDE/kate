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

#include "btparser.h"
#include "btfileindexer.h"

#include <klocalizedstring.h>          // i18n
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <ktexteditor/view.h>
#include <ksharedconfig.h>
#include <KConfigGroup>
#include <KLineEdit>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDataStream>
#include <QStandardPaths>
#include <QTimer>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QUrl>
//END Includes

K_PLUGIN_FACTORY_WITH_JSON (KateBtBrowserFactory, "katebacktracebrowserplugin.json", registerPlugin<KateBtBrowserPlugin>();)

KateBtBrowserPlugin* KateBtBrowserPlugin::s_self = 0L;
static QStringList fileExtensions =
    QStringList() << QLatin1String("*.cpp") << QLatin1String("*.cxx") <<
    QLatin1String("*.c") << QLatin1String("*.cc") << QLatin1String("*.h") <<
    QLatin1String("*.hpp") << QLatin1String("*.hxx") << QLatin1String("*.moc");

KateBtBrowserPlugin::KateBtBrowserPlugin(QObject* parent, const QList<QVariant>&)
  : KTextEditor::Plugin(parent)
  , KTextEditor::ConfigPageInterface()
  , indexer(&db)
{
  s_self = this;
  db.loadFromFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                         QLatin1String("kate/backtracedatabase")));
}

KateBtBrowserPlugin::~KateBtBrowserPlugin()
{
  if (indexer.isRunning()) {
    indexer.cancel();
    indexer.wait();
  }

  db.saveToFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                       QLatin1String("kate/backtracedatabase")));

  s_self = 0L;
}

KateBtBrowserPlugin& KateBtBrowserPlugin::self()
{
  return *s_self;
}

QObject *KateBtBrowserPlugin::createView (KTextEditor::MainWindow *mainWindow)
{
  KateBtBrowserPluginView * view = new KateBtBrowserPluginView(this, mainWindow);
  return view;
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
  KConfigGroup cg(KSharedConfig::openConfig(), "backtracebrowser");
  indexer.setSearchPaths(cg.readEntry("search-folders", QStringList()));
  indexer.setFilter(cg.readEntry("file-extensions", fileExtensions));
  indexer.start();
  emit newStatus(i18n("Indexing files..."));
}

int KateBtBrowserPlugin::configPages () const
{
  return 1;
}

KTextEditor::ConfigPage* KateBtBrowserPlugin::configPage(int number, QWidget *parent)
{
  if (number == 0) {
    return new KateBtConfigWidget(parent);
  }

  return 0L;
}

QString KateBtBrowserPlugin::configPageName (int number) const
{
  if (number == 0) {
    return i18n("Backtrace Browser");
  }
  return QString();
}

QString KateBtBrowserPlugin::configPageFullName (int number) const
{
  if (number == 0) {
    return i18n("Backtrace Browser Settings");
  }
  return QString();
}

QIcon KateBtBrowserPlugin::configPageIcon (int) const
{
  return QIcon::fromTheme(QLatin1String("kbugbuster"));
}





KateBtBrowserPluginView::KateBtBrowserPluginView(KateBtBrowserPlugin* plugin, KTextEditor::MainWindow *mainWindow)
  : QObject(mainWindow), m_plugin(plugin)
{
  // init console
  QWidget *toolview = mainWindow->createToolView (plugin,
                                                  QLatin1String("kate_private_plugin_katebacktracebrowserplugin"),
                                                  KTextEditor::MainWindow::Bottom,
                                                  QIcon::fromTheme(QLatin1String("kbugbuster")),
                                                  i18n("Backtrace Browser"));
  m_widget = new KateBtBrowserWidget(mainWindow, toolview);

  connect(plugin, SIGNAL(newStatus(QString)),
          this, SLOT(setStatus(QString)));
}

KateBtBrowserPluginView::~KateBtBrowserPluginView ()
{
  // cleanup, kill toolview + widget
  QWidget *toolview = m_widget->parentWidget();
  delete m_widget;
  delete toolview;
}






KateBtBrowserWidget::KateBtBrowserWidget(KTextEditor::MainWindow *mainwindow, QWidget* parent)
  : QWidget(parent)
  , mw(mainwindow)
{
  setupUi(this);

  timer.setSingleShot(true);
  connect(&timer, SIGNAL(timeout()), this, SLOT(clearStatus()));

  connect(btnBacktrace, SIGNAL(clicked()), this, SLOT(loadFile()));
  connect(btnClipboard, SIGNAL(clicked()), this, SLOT(loadClipboard()));
  connect(btnConfigure, SIGNAL(clicked()), this, SLOT(configure()));
  connect(lstBacktrace, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(itemActivated(QTreeWidgetItem*,int)));
}

KateBtBrowserWidget::~KateBtBrowserWidget ()
{
}

void KateBtBrowserWidget::loadFile()
{
  QString url = QFileDialog::getOpenFileName(mw->window(), i18n("Load Backtrace"));
  QFile f(url);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString str = QString::fromUtf8(f.readAll());
    loadBacktrace(str);
  }
}

void KateBtBrowserWidget::loadClipboard()
{
  QString bt = QApplication::clipboard()->text();
  loadBacktrace(bt);
}

void KateBtBrowserWidget::loadBacktrace(const QString& bt)
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


void KateBtBrowserWidget::configure()
{
  KateBtConfigDialog dlg(mw->window());
  dlg.exec();
}

void KateBtBrowserWidget::itemActivated(QTreeWidgetItem* item, int column)
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
      static QRegExp rx1(QLatin1String("/([^/]+)/([^/]+)$"));
      int idx = rx1.indexIn(file);
      if (idx != -1) {
        file = rx1.cap(1) + QLatin1Char('/') + rx1.cap(2);
      } else {
        static QRegExp rx2(QLatin1String("([^/]+)/([^/]+)$"));
        idx = rx2.indexIn(file);
        if (idx != -1) {
          // file is of correct form
        } else {
          qDebug() << "file patter did not match:" << file;
          setStatus(i18n("File not found: %1", file));
          return;
        }
      }
      path = KateBtBrowserPlugin::self().database().value(file);
    }

    if (!path.isEmpty() && QFile::exists(path)) {
      KTextEditor::View* kv = mw->openUrl(QUrl(path));
      kv->setCursorPosition(KTextEditor::Cursor(line - 1, 0));
      kv->setFocus();
      setStatus(i18n("Opened file: %1", file));
    }
  } else {
    setStatus(i18n("No debugging information available"));
  }
}

void KateBtBrowserWidget::setStatus(const QString& status)
{
  lblStatus->setText(status);
  timer.start(10*1000);
}

void KateBtBrowserWidget::clearStatus()
{
  lblStatus->setText(QString());
}




KateBtConfigWidget::KateBtConfigWidget(QWidget* parent)
  : KTextEditor::ConfigPage(parent)
{
  setupUi(this);
  edtUrl->setMode(KFile::Directory);
  edtUrl->setUrl(QUrl(QDir().absolutePath()));

  reset();

  connect(btnAdd, SIGNAL(clicked()), this, SLOT(add()));
  connect(btnRemove, SIGNAL(clicked()), this, SLOT(remove()));
  connect(edtExtensions, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

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
    KConfigGroup cg(KSharedConfig::openConfig(), "backtracebrowser");
    cg.writeEntry("search-folders", sl);

    QString filter = edtExtensions->text();
    filter.replace(QLatin1Char(','), QLatin1Char(' ')).replace(QLatin1Char(';'), QLatin1Char(' '));
    cg.writeEntry("file-extensions", filter.split(QLatin1Char(' '), QString::SkipEmptyParts));

    KateBtBrowserPlugin::self().startIndexer();
    m_changed = false;
  }
}

void KateBtConfigWidget::reset()
{
  KConfigGroup cg(KSharedConfig::openConfig(), "backtracebrowser");
  lstFolders->clear();
  lstFolders->addItems(cg.readEntry("search-folders", QStringList()));
  edtExtensions->setText(cg.readEntry("file-extensions", fileExtensions).join(QLatin1String(" ")));
}

void KateBtConfigWidget::defaults()
{
  lstFolders->clear();
  edtExtensions->setText(fileExtensions.join(QLatin1String(" ")));

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
  : QDialog(parent)
{
  setWindowTitle(i18n("Backtrace Browser Settings"));


  m_configWidget = new KateBtConfigWidget(this);

  QVBoxLayout * layout = new QVBoxLayout(this);

  QDialogButtonBox * box = new QDialogButtonBox(this);
  box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  layout->addWidget(m_configWidget);
  layout->addWidget(box);

  connect(this, SIGNAL(accepted()), m_configWidget, SLOT(apply()));
  connect(box, SIGNAL(accepted()), this, SLOT(accept()));
  connect(box, SIGNAL(rejected()), this, SLOT(reject()));
}

KateBtConfigDialog::~KateBtConfigDialog()
{
}

#include "katebacktracebrowser.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
