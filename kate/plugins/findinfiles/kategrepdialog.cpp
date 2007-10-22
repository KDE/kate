/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2004 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Dominik Haumann <dhaumann@kde.org>
   Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>

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

#include "kategrepdialog.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QCursor>
#include <QObject>
#include <QRegExp>
#include <QShowEvent>
#include <QToolButton>
#include <QToolTip>
#include <QTreeWidget>

#include <kacceleratormanager.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurlcompletion.h>

KateGrepDialog::KateGrepDialog(QWidget *parent, Kate::MainWindow *mw)
    : QWidget(parent), m_mw (mw), m_grepThread (0)
{
  setupUi(this);
  setWindowTitle(i18n("Find in Files"));

  cmbPattern->setDuplicatesEnabled(false);
  cmbPattern->setInsertPolicy(QComboBox::NoInsert);
  cmbPattern->setFocus();

  // set sync icon
  btnSync->setIcon(QIcon(SmallIcon("view-refresh")));

  // get url-requester's combo box and sanely initialize
  KComboBox* cmbUrl = cmbDir->comboBox();
  cmbUrl->setDuplicatesEnabled(false);
  cmbUrl->setEditable(true);
  cmbDir->completionObject()->setMode(KUrlCompletion::DirCompletion);
  cmbDir->setMode( KFile::Directory | KFile::LocalOnly );

  cmbFiles->setInsertPolicy(QComboBox::NoInsert);
  cmbFiles->setDuplicatesEnabled(false);

  // buttons find and clear
  btnSearch->setGuiItem(KStandardGuiItem::find());
  btnClear->setGuiItem(KStandardGuiItem::clear());

  // auto-accels
  KAcceleratorManager::manage( this );

  lblPattern->setWhatsThis( i18n("<p>Enter the expression you want to search for here.</p>"
                                 "<p>If 'regular expression' is unchecked, any non-space letters in your "
                                 "expression will be escaped with a backslash character.</p>"
                                 "<p>Possible meta characters are:<br />"
                                 "<b>.</b> - Matches any character<br />"
                                 "<b>^</b> - Matches the beginning of a line<br />"
                                 "<b>$</b> - Matches the end of a line<br />"
                                 "<b>\\&lt;</b> - Matches the beginning of a word<br />"
                                 "<b>\\&gt;</b> - Matches the end of a word</p>"
                                 "<p>The following repetition operators exist:<br />"
                                 "<b>?</b> - The preceding item is matched at most once<br />"
                                 "<b>*</b> - The preceding item is matched zero or more times<br />"
                                 "<b>+</b> - The preceding item is matched one or more times<br />"
                                 "<b>{<i>n</i>}</b> - The preceding item is matched exactly <i>n</i> times<br />"
                                 "<b>{<i>n</i>,}</b> - The preceding item is matched <i>n</i> or more times<br />"
                                 "<b>{,<i>n</i>}</b> - The preceding item is matched at most <i>n</i> times<br />"
                                 "<b>{<i>n</i>,<i>m</i>}</b> - The preceding item is matched at least <i>n</i>, "
                                 "but at most <i>m</i> times.</p>"
                                 "<p>Furthermore, backreferences to bracketed subexpressions are available "
                                 "via the notation <code>\\#</code>.</p>"
                                 "<p>See the grep(1) documentation for the full documentation.</p>"
                                ));
  lblFiles->setWhatsThis(    i18n("Enter the file name pattern of the files to search here.\n"
                                  "You may give several patterns separated by commas."));
  lblFolder->setWhatsThis(    i18n("Enter the folder which contains the files in which you want to search."));
  cbRecursive->setWhatsThis(    i18n("Check this box to search in all subfolders."));
  cbCasesensitive->setWhatsThis(    i18n("If this option is enabled (the default), the search will be case sensitive."));
  lbResult->setWhatsThis(    i18n("The results of the grep run are listed here. Select a\n"
                                  "filename/line number combination and press Enter or doubleclick\n"
                                  "on the item to show the respective line in the editor."));

  // event filter, do something relevant for RETURN
  cmbPattern->installEventFilter( this );
  cmbFiles->installEventFilter( this );
  cmbDir->comboBox()->installEventFilter( this );
  
  
  // add close tab button to tabwidget
  btnCloseTab = new QToolButton( lbResult );
  btnCloseTab->setEnabled(false);
  QToolTip::add(btnCloseTab,i18n("Click to close the current search results."));
  btnCloseTab->setIconSet(QIcon(SmallIcon("tab-close")));
  btnCloseTab->adjustSize();
  connect(btnCloseTab, SIGNAL(clicked()), SLOT(slotCloseResultTab()));
  btnCloseTab->hide();
    
  lbResult->setCornerWidget( btnCloseTab, Qt::BottomRight );
  lbResult->setTabCloseActivatePrevious (true);
  connect(lbResult, SIGNAL(closeRequest( QWidget * )), this,
          SLOT(slotCloseResultTab(QWidget*)));

  connect( btnSearch, SIGNAL(clicked()),
           SLOT(slotSearch()) );
  connect( btnClear, SIGNAL(clicked()),
           SLOT(slotClear()) );
  connect( cmbPattern->lineEdit(), SIGNAL(textChanged ( const QString & )),
           SLOT( patternTextChanged( const QString & )));
  connect( btnSync, SIGNAL(clicked()), this, SLOT(syncDir()));


  patternTextChanged( cmbPattern->lineEdit()->text());
}


KateGrepDialog::~KateGrepDialog()
{
  killThread ();
}

void KateGrepDialog::readSessionConfig (const KConfigGroup& config)
{
  // first clear everything (could be session switch)
  cmbPattern->clear();
  cmbDir->comboBox()->clear();
  cmbFiles->clear();

  // now restore new session settings
  lastSearchItems = config.readEntry("LastSearchItems", QStringList());
  lastSearchPaths = config.readEntry("LastSearchPaths", QStringList());
  lastSearchFiles = config.readEntry("LastSearchFiles", QStringList());

  if( lastSearchFiles.isEmpty() )
  {
    // if there are no entries, most probably the first Kate start.
    // Initialize with default values.
    lastSearchFiles << "*"
    << "*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c"
    << "*.cpp,*.cc,*.C,*.cxx,*.c"
    << "*.h,*.hxx,*.idl";
  }

  cmbPattern->insertItems(0, lastSearchItems);
  cmbPattern->setEditText(QString());

  cbCasesensitive->setChecked(config.readEntry("CaseSensitive", true));
  cbRecursive->setChecked(config.readEntry("Recursive", true));

  cmbDir->comboBox()->insertItems(0, lastSearchPaths);
  cmbFiles->insertItems(0, lastSearchFiles);
}

void KateGrepDialog::writeSessionConfig (KConfigGroup& config)
{
  config.writeEntry("LastSearchItems", lastSearchItems);
  config.writeEntry("LastSearchPaths", lastSearchPaths);
  config.writeEntry("LastSearchFiles", lastSearchFiles);

  config.writeEntry("Recursive", cbRecursive->isChecked());
  config.writeEntry("CaseSensitive", cbCasesensitive->isChecked());
}

void KateGrepDialog::killThread ()
{
  if (m_grepThread)
  {
    m_grepThread->cancel();
    m_grepThread->wait ();
    delete m_grepThread;
    m_grepThread = 0;
  }
}

void KateGrepDialog::patternTextChanged( const QString & _text)
{
  btnSearch->setEnabled( !_text.isEmpty() );
}

void KateGrepDialog::itemSelected(QTreeWidgetItem *item, int)
{
  // get stuff
  const QString filename = item->data(0, Qt::UserRole).toString();
  const int linenumber = item->data(1, Qt::UserRole).toInt();
  const int column = item->data(2, Qt::UserRole).toInt();

  // open file (if needed, otherwise, this will activate only the right view...)
  KUrl fileURL;
  fileURL.setPath( filename );
  m_mw->openUrl( fileURL );

  // any view active?
  if ( !m_mw->activeView() )
    return;

  // do it ;)
  m_mw->activeView()->setCursorPosition( KTextEditor::Cursor (linenumber, column) );
}

void KateGrepDialog::slotSearch()
{
  // already running, cancel...
  if (m_grepThread)
  {
    killThread ();
    return;
  }

  // no pattern set...
  if ( cmbPattern->currentText().isEmpty() )
  {
    cmbPattern->setFocus();
    return;
  }

  // dir does not exist...
  if ( cmbDir->url().isEmpty() || ! QDir(cmbDir->url().toLocalFile ()).exists() )
  {
    cmbDir->setFocus();
    KMessageBox::information( this, i18n(
                                "You must enter an existing local folder in the 'Folder' entry."),
                              i18n("Invalid Folder"), "Kate grep tool: invalid folder" );
    return;
  }

  // switch the button + cursor...
  lbResult->setCursor( QCursor(Qt::WaitCursor) );
  btnClear->setEnabled( false );
  btnSearch->setGuiItem( KStandardGuiItem::cancel() );

  
  // add a tab for the new search
  QTreeWidget* w = new QTreeWidget();
  connect( w, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
           SLOT(itemSelected(QTreeWidgetItem *, int)) );
  
  // result view, list all matches....
  QStringList headers;
  headers << i18n("File") << i18n("Line") << i18n("Text");
  w->setHeaderLabels(headers);
  w->setIndentation(0);
  
  lbResult->insertTab( w, QIcon(SmallIcon ("system-search")),
                       cmbPattern->currentText(), 0);
  lbResult->setCurrentWidget (w);

  //
  // init the grep thread
  //

  // wildcards
  QStringList wildcards = cmbFiles->currentText().split(QRegExp("[,;]"), QString::SkipEmptyParts);

  // regexps
  QRegExp reg (cmbPattern->currentText(), cbCasesensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
  QList<QRegExp> liste;
  liste << reg;

  // create the thread object
  m_grepThread = new KateGrepThread (this, cmbDir->url().toLocalFile (), cbRecursive->isChecked(), wildcards, liste);

  // connect feedback signals
  connect (m_grepThread, SIGNAL(finished()), this, SLOT(searchFinished()));
  connect (m_grepThread, SIGNAL(foundMatch (const QString &, int, int, const QString &, const QString &)),
           this, SLOT(searchMatchFound(const QString &, int, int, const QString &, const QString &)));

  // grep
  m_grepThread->start();
}

void KateGrepDialog::searchFinished ()
{
  lbResult->unsetCursor();
  btnClear->setEnabled( true );
  btnSearch->setGuiItem( KStandardGuiItem::find() );
  btnCloseTab->setEnabled(true);
  btnCloseTab->show();

  addItems();

  delete m_grepThread;
  m_grepThread = 0;
}

void KateGrepDialog::addItems()
{
  // update last pattern
  QString cmbText = cmbPattern->currentText();
  bool itemsRemoved = lastSearchItems.removeAll(cmbText) > 0;
  lastSearchItems.prepend(cmbText);
  if (itemsRemoved)
    cmbPattern->removeItem(cmbPattern->findText(cmbText));
  cmbPattern->insertItem(0, cmbText);
  cmbPattern->setCurrentIndex(0);
  if (lastSearchItems.count() > 10)
  {
    lastSearchItems.pop_back();
    cmbPattern->removeItem(cmbPattern->count() - 1);
  }


  // update last search path
  cmbText = cmbDir->url().url();
  itemsRemoved = lastSearchPaths.removeAll(cmbText) > 0;
  lastSearchPaths.prepend(cmbText);
  if (itemsRemoved)
  {
    cmbDir->comboBox()->removeItem(cmbDir->comboBox()->findText(cmbText));
  }
  cmbDir->comboBox()->insertItem(0, cmbText);
  cmbDir->comboBox()->setCurrentIndex(0);
  if (lastSearchPaths.count() > 10)
  {
    lastSearchPaths.pop_back();
    cmbDir->comboBox()->removeItem(cmbDir->comboBox()->count() - 1);
  }


  // update last filter
  cmbText = cmbFiles->currentText();
  // remove and prepend, so that the mose recently used item is on top
  itemsRemoved = lastSearchFiles.removeAll(cmbText) > 0;
  lastSearchFiles.prepend(cmbText);
  if (itemsRemoved) // combo box already contained item -> remove it first
  {
    cmbFiles->removeItem(cmbFiles->findText(cmbText));
  }
  cmbFiles->insertItem(0, cmbText);
  cmbFiles->setCurrentIndex(0);
  if (lastSearchFiles.count() > 10)
  {
    lastSearchFiles.pop_back();
    cmbFiles->removeItem(cmbFiles->count() - 1);
  }
}

void KateGrepDialog::searchMatchFound(const QString &filename, int line, int column, const QString &basename, const QString &lineContent)
{
  QTreeWidget* w = (QTreeWidget*) lbResult->currentWidget();
  
  QTreeWidgetItem* item = new QTreeWidgetItem(w);
  item->setText(0, basename);
  item->setText(1, QString::number (line + 1));
  item->setText(2, lineContent.trimmed());
  item->setData(0, Qt::UserRole, filename);
  item->setData(1, Qt::UserRole, line);
  item->setData(2, Qt::UserRole, column);
}

void KateGrepDialog::slotClear()
{
  lbResult->clear();
}

void KateGrepDialog::slotCloseResultTab()
{
  slotCloseResultTab (lbResult->currentPage());
}

void KateGrepDialog::slotCloseResultTab(QWidget* widget)
{
  lbResult->removePage (widget);
    
  widget->hide();
  delete widget;
    
  if (lbResult->count() == 0)
  {
    btnCloseTab->setEnabled(false);
    btnCloseTab->hide();
  }
}

bool KateGrepDialog::eventFilter( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::KeyPress && (
         ((QKeyEvent*)e)->key() == Qt::Key_Return ||
         ((QKeyEvent*)e)->key() == Qt::Key_Enter ) )
  {
    slotSearch();
    return true;
  }

  return QWidget::eventFilter( o, e );
}

void KateGrepDialog::syncDir()
{
  // sync url-requester with active view's path
  KUrl url = m_mw->activeView()->document()->url();
  if (url.isLocalFile())
    cmbDir->setUrl(url.directory());
}

void KateGrepDialog::showEvent(QShowEvent* event)
{
  if (event->spontaneous())
    return;

  // thread is running -> the toolview was closed and opened again
  // in this case, do not change the url
  if (!cmbDir->url().isEmpty() || m_grepThread)
    return;

  syncDir();
}


#if 0

void KateMainWindow::slotKateGrepDialogItemSelected(const QString &filename, int linenumber)
{
}
#endif

#include "kategrepdialog.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
