/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2004 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007-2008 Dominik Haumann <dhaumann kde org>
   Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

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

#include "katefinddialog.h"
#include "katefindinfiles.h"
#include "kateresultview.h"
#include "katefindoptions.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kdebug.h>

#include <QClipboard>
#include <QCursor>
#include <QObject>
#include <QRegExp>
#include <QShowEvent>
#include <QToolButton>
#include <QToolTip>
#include <QTreeWidget>
#include <QHeaderView>

#include <kacceleratormanager.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurlcompletion.h>

KateFindDialog::KateFindDialog(Kate::MainWindow *mw, KateFindInFilesView *view)
    : KDialog(mw->window()), m_mw (mw), m_view(view)
    , m_useId(-1)
{
  setCaption(i18n("Find in Files"));
  setButtons(Close | User1);

  setButtonGuiItem(User1, KStandardGuiItem::find());
  setDefaultButton(User1);

  QWidget* widget = new QWidget(this);
  setupUi(widget);
  setMainWidget(widget);

  // if possible, ui properties are set in the .ui-file

  // set sync icon
  btnSync->setIcon(QIcon(SmallIcon("view-refresh")));

  // get url-requester's combo box and sanely initialize
  KComboBox* cmbUrl = cmbDir->comboBox();
  cmbUrl->setDuplicatesEnabled(false);
  cmbUrl->setEditable(true);
  cmbDir->setMode( KFile::Directory | KFile::LocalOnly );
  KUrlCompletion* cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
  cmbUrl->setCompletionObject( cmpl );
  cmbUrl->setAutoDeleteCompletionObject( true );

  updateItems();
  syncDir();

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
  chkRecursive->setWhatsThis(    i18n("Check this box to search in all subfolders."));
  chkCaseSensitive->setWhatsThis(    i18n("If this option is enabled (the default), the search will be case sensitive."));

  connect( this, SIGNAL(user1Clicked()),
           SLOT(slotSearch()) );
  connect( cmbPattern->lineEdit(), SIGNAL(textChanged ( const QString & )),
           SLOT( patternTextChanged( const QString & )));
  connect( btnSync, SIGNAL(clicked()), this, SLOT(syncDir()));

  patternTextChanged( cmbPattern->lineEdit()->text());

  resize(500, 350);
}


KateFindDialog::~KateFindDialog()
{
}

void KateFindDialog::patternTextChanged( const QString & text)
{
  enableButton(User1, !text.isEmpty());
}

void KateFindDialog::slotSearch()
{
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

  // regexps
  QRegExp reg (cmbPattern->currentText(), chkCaseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
  QList<QRegExp> liste;
  liste << reg;

  KateResultView* view = m_view->toolViewFromId(m_useId);
  if (!view) {
    view = new KateResultView(m_mw, m_view);
    m_view->addResultView(view);
  }
  view->makeVisible();
  view->startSearch(KateFindInFilesOptions::self(), liste,
                    cmbDir->url().toLocalFile(), cmbFilter->currentText());

  updateConfig();
  hide();
}

void KateFindDialog::updateConfig()
{
  // update last pattern
  KateFindInFilesOptions::self().addSearchItem(cmbPattern->currentText());
  KateFindInFilesOptions::self().addSearchPath(cmbDir->url().url());
  KateFindInFilesOptions::self().addSearchFilter(cmbFilter->currentText());

  KateFindInFilesOptions::self().setRecursive(chkRecursive->isChecked());
  KateFindInFilesOptions::self().setCaseSensitive(chkCaseSensitive->isChecked());
  KateFindInFilesOptions::self().setRegExp(chkRegExp->isChecked());
}

void KateFindDialog::updateItems()
{
  cmbPattern->clear();
  cmbDir->clear();
  cmbFilter->clear();

  cmbPattern->insertItems(0, KateFindInFilesOptions::self().searchItems());
  cmbDir->comboBox()->insertItems(0, KateFindInFilesOptions::self().searchPaths());
  cmbFilter->insertItems(0, KateFindInFilesOptions::self().searchFilters());

  cmbPattern->setCurrentIndex(0);
  cmbDir->comboBox()->setCurrentIndex(0);
  cmbFilter->setCurrentIndex(0);

  chkRecursive->setChecked(KateFindInFilesOptions::self().recursive());
  chkCaseSensitive->setChecked(KateFindInFilesOptions::self().caseSensitive());
  chkRegExp->setChecked(KateFindInFilesOptions::self().regExp());
}

void KateFindDialog::setPattern(const QList<QRegExp>& pattern)
{
  if (pattern.size())
    cmbPattern->setEditText(pattern[0].pattern());
}

void KateFindDialog::setUrl(const QString& url)
{
  cmbDir->setUrl(url);
}

void KateFindDialog::setFilter(const QString& filter)
{
  cmbFilter->setEditText(filter);
}

void KateFindDialog::setOptions(const KateFindInFilesOptions& options)
{
  chkRecursive->setChecked(options.recursive());
  chkCaseSensitive->setChecked(options.caseSensitive());
  chkRegExp->setChecked(options.regExp());
}

void KateFindDialog::syncDir()
{
  // sync url-requester with active view's path
  KUrl url = m_mw->activeView()->document()->url();
  if (url.isLocalFile())
    cmbDir->setUrl(url.directory());
}

void KateFindDialog::showEvent(QShowEvent* event)
{
  if (event->spontaneous())
    return;

  cmbPattern->setFocus();

  if (!cmbDir->url().isEmpty())
    return;

  syncDir();
}

void KateFindDialog::useResultView(int id)
{
  m_useId = id;
}

#include "katefinddialog.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
