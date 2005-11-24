/*
    This library is free software you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    file: autobookmarker.cpp

    KTextEditor plugin to add bookmarks to documents.
    Copyright Anders Lund <anders.lund@lund.tdcadsl.dk>, 2003
*/

//BEGIN includes
#include "autobookmarker.h"

#include <ktexteditor/markinterfaceextension.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/documentinfo.h>
#include <ktexteditor/document.h>

#include <kaction.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kmimetypechooser.h>
#include <kprocess.h>
#include <krun.h>
#include <kstaticdeleter.h>
#include <kurl.h>

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <q3listview.h>
#include <q3popupmenu.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qregexp.h>
//Added by qt3to4:
#include <QPixmap>
#include <QGridLayout>
#include <Q3Frame>
#include <QHBoxLayout>
#include <QVBoxLayout>

//#include <kdebug.h>
//END includes

//BEGIN AutoBookmarker
K_EXPORT_COMPONENT_FACTORY( ktexteditor_autobookmarker, KGenericFactory<AutoBookmarker>( "ktexteditor_autobookmarker" ) )

AutoBookmarker::AutoBookmarker( QObject *parent,
                            const char* name,
                            const QStringList& /*args*/ )
        : KTextEditor::Plugin ( (KTextEditor::Document*) parent, name ),
          KTextEditor::ConfigInterfaceExtension()
{
  if ( parent )
    connect( parent, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
}

void AutoBookmarker::addView(KTextEditor::View */*view*/)
{
}

void AutoBookmarker::removeView(KTextEditor::View */*view*/)
{
}

KTextEditor::ConfigPage * AutoBookmarker::configPage( uint /*number*/, QWidget *parent, const char *name )
{
  return new AutoBookmarkerConfigPage( parent, name );
}

QString AutoBookmarker::configPageName( uint /*p*/ ) const
{
//   switch (p)
//   {
//     case 0:
      return i18n("AutoBookmarks");
//     default:
//       return "";
//   }
}

QString AutoBookmarker::configPageFullName( uint /*p*/ ) const
{
//   switch (p)
//   {
//     case 0:
      return i18n("Configure AutoBookmarks");
//     default:
//       return "";
//   }
}

QPixmap AutoBookmarker::configPagePixmap( uint /*p*/, int size ) const
{
  return UserIcon("kte_bookmark", size);
}

void AutoBookmarker::slotCompleted()
{
  // get the document info
  KTextEditor::DocumentInfoInterface *di =
      static_cast<KTextEditor::DocumentInfoInterface*>(document()->
          qt_cast("KTextEditor::DocumentInfoInterface"));
  QString mt;
  if ( di ) // we can still try match the URL otherwise
    mt = di->mimeType();

  QString fileName;
  if ( document()->url().isValid() )
    fileName = document()->url().fileName();

  ABEntityList *l = ABGlobal::self()->entities();
  // for each item, if either mask matches
  // * apply if onLoad is true
  ABEntityListIterator it( *l );
  int n( 0 );
  bool found;
  AutoBookmarkEnt *e;
  while ( ( e = it.current() ) != 0 )
  {
    found = ( !e->mimemask.count() && !e->filemask.count() ); // no preferences
    if ( ! found )
      found = ( ! mt.isEmpty() && e->mimemask.contains( mt ) );
    if ( ! found )
      for( QStringList::Iterator it1 = e->filemask.begin(); it1 != e->filemask.end(); ++it1 )
      {
        QRegExp re(*it1, true, true);
        if ( ( found = ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) ) ) )
         break;
      }

    if ( found )
        applyEntity( e );

    n++;
    ++it;
  }

}

void AutoBookmarker::applyEntity( AutoBookmarkEnt *e )
{
  KTextEditor::Document *doc = document();
  KTextEditor::EditInterface *ei = KTextEditor::editInterface( doc );
  KTextEditor::MarkInterface *mi = KTextEditor::markInterface( doc );

  if ( ! ( ei && mi ) ) return;

  QRegExp re( e->pattern, e->flags & AutoBookmarkEnt::CaseSensitive );
  re.setMinimal( e->flags & AutoBookmarkEnt::MinimalMatching );

  for ( uint l( 0 ); l < ei->numLines(); l++ )
    if ( re.search( ei->textLine( l ) ) > -1 )
      mi->setMark( l, KTextEditor::MarkInterface::Bookmark );
}

//END

//BEGIN ABGlobal
ABGlobal *ABGlobal::s_self = 0;

ABGlobal::ABGlobal()
{
  m_ents = new ABEntityList;
  readConfig();
}

ABGlobal::~ABGlobal()
{
  delete m_ents;
}

static KStaticDeleter<ABGlobal> sdSelf;

ABGlobal *ABGlobal::self()
{
  if ( ! s_self )
    sdSelf.setObject(s_self, new ABGlobal());

  return s_self;
}

void ABGlobal::readConfig()
{
  if ( ! m_ents )
    m_ents = new ABEntityList;
  else
    m_ents->clear();
  KConfig *config = new KConfig("ktexteditor_autobookmarkerrc");

  uint n( 0 );
  while ( config->hasGroup( QString("autobookmark%1").arg( n ) ) )
  {
    config->setGroup( QString("autobookmark%1").arg( n ) );
    QStringList filemask = config->readListEntry( "filemask", ';' );
    QStringList mimemask = config->readListEntry( "mimemask", ';' );
    int flags = config->readNumEntry( "flags", 1 );
    AutoBookmarkEnt *e = new AutoBookmarkEnt(
        config->readEntry( "pattern", "" ),
        filemask,
        mimemask,
        flags
        );

    m_ents->append( e );

    ++n;
  }

  delete config;
}

void ABGlobal::writeConfig()
{
  KConfig *config = new KConfig("ktexteditor_autobookmarkerrc");

  // clean the config object
  QStringList l = config->groupList();
  for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
    config->deleteGroup( *it );

  // fill in the current list
  for ( uint i = 0; i < m_ents->count(); i++ )
  {
    AutoBookmarkEnt *e = m_ents->at( i );
    config->setGroup( QString("autobookmark%1").arg( i ) );
    config->writeEntry( "pattern", e->pattern );
    config->writeEntry( "filemask", e->filemask, ';' );
    config->writeEntry( "mimemask", e->mimemask, ';' );
    config->writeEntry( "flags", e->flags );
  }

  config->sync(); // explicit -- this is supposedly handled by the d'tor
  delete config;
}
//END ABGlobal

//BEGIN AutoBookmarkEntItem
// A QListviewItem which can hold a AutoBookmarkEnt pointer
class AutoBookmarkEntItem : public Q3ListViewItem
{
  public:
    AutoBookmarkEntItem( KListView *lv, AutoBookmarkEnt *e )
        : Q3ListViewItem( lv ),
        ent( e )
      {
        redo();
      };
    ~AutoBookmarkEntItem(){};
    void redo()
    {
        setText( 0, ent->pattern );
        setText( 1, ent->mimemask.join("; ") );
        setText( 2, ent->filemask.join("; ") );
    }
    AutoBookmarkEnt *ent;
};
//END

//BEGIN AutoBookmarkerEntEditor
// Dialog for editing a single autobookmark entity
// * edit the pattern
// * set the file/mime type masks
AutoBookmarkerEntEditor::AutoBookmarkerEntEditor( QWidget *parent, AutoBookmarkEnt *e )
        : KDialogBase( parent, "autobookmark_ent_editor",
                       true, i18n("Edit Entry"),
                       KDialogBase::Ok|KDialogBase::Cancel ),
          e( e )
{
  QFrame *w = makeMainWidget();
  QGridLayout * lo = new QGridLayout( w, 5, 3 );
  lo->setSpacing( KDialogBase::spacingHint() );

  QLabel *l = new QLabel( i18n("&Pattern:"), w );
  lePattern = new QLineEdit( e->pattern, w );
  l->setBuddy( lePattern );
  lo->addWidget( l, 0, 0 );
  lo->addMultiCellWidget(  lePattern, 0, 0, 1, 2 );
  lePattern->setWhatsThis(i18n(
      "<p>A regular expression. Matching lines will be bookmarked.</p>" ) );

  connect( lePattern, SIGNAL(textChanged ( const QString & ) ),this, SLOT( slotPatternChanged( const QString& ) ) );

  cbCS = new QCheckBox( i18n("Case &sensitive"), w );
  lo->addMultiCellWidget( cbCS, 1, 1, 0, 2 );
  cbCS->setChecked( e->flags & AutoBookmarkEnt::CaseSensitive );
  cbCS->setWhatsThis(i18n(
      "<p>If enabled, the pattern matching will be case sensitive, otherwise "
      "not.</p>") );

  cbMM = new QCheckBox( i18n("&Minimal matching"), w );
  lo->addMultiCellWidget( cbMM, 2, 2, 0 ,2 );
  cbMM->setChecked( e->flags & AutoBookmarkEnt::MinimalMatching );
  cbMM->setWhatsThis(i18n(
      "<p>If enabled, the pattern matching will use minimal matching; if you "
      "do not know what that is, please read the appendix on regular expressions "
      "in the kate manual.</p>") );

  l = new QLabel( i18n("&File mask:"), w );
  leFileMask = new QLineEdit( e->filemask.join( "; " ), w );
  l->setBuddy( leFileMask );
  lo->addWidget( l, 3, 0 );
  lo->addMultiCellWidget( leFileMask, 3, 3, 1, 2 );
  leFileMask->setWhatsThis(i18n(
      "<p>A list of filename masks, separated by semicolons. This can be used "
      "to limit the usage of this entity to files with matching names.</p>"
      "<p>Use the wizard button to the right of the mimetype entry below to "
      "easily fill out both lists.</p>" ) );

  l = new QLabel( i18n("MIME &types:"), w );
  leMimeTypes = new QLineEdit( e->mimemask.join( "; " ), w );
  l->setBuddy( leMimeTypes );
  lo->addWidget( l, 4, 0 );
  lo->addWidget( leMimeTypes, 4, 1 );
  leMimeTypes->setWhatsThis(i18n(
      "<p>A list of mime types, separated by semicolon. This can be used to "
      "limit the usage of this entity to files with matching mime types.</p>"
      "<p>Use the wizard button on the right to get a list of existing file "
      "types to choose from, using it will fill in the file masks as well.</p>" ) );

  QToolButton *btnMTW = new QToolButton(w);
  lo->addWidget( btnMTW, 4, 2 );
  btnMTW->setIcon(QIcon(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));
  btnMTW->setWhatsThis(i18n(
      "<p>Click this button to display a checkable list of mimetypes available "
      "on your system. When used, the file masks entry above will be filled in "
      "with the corresponding masks.</p>") );
  slotPatternChanged( lePattern->text() );
}

void AutoBookmarkerEntEditor::slotPatternChanged( const QString&_pattern )
{
    enableButtonOK( !_pattern.isEmpty() );
}

void AutoBookmarkerEntEditor::apply()
{
  if ( lePattern->text().isEmpty() ) return;

  e->pattern = lePattern->text();
  e->filemask = QStringList::split( QRegExp("\\s*;\\s*"), leFileMask->text() );
  e->mimemask = QStringList::split( QRegExp("\\s*;\\s*"), leMimeTypes->text() );
  e->flags = 0;
  if ( cbCS->isOn() ) e->flags |= AutoBookmarkEnt::CaseSensitive;
  if ( cbMM->isOn() ) e->flags |= AutoBookmarkEnt::MinimalMatching;
}

void AutoBookmarkerEntEditor::showMTDlg()
{
  QString text = i18n("Select the MimeTypes for this pattern.\nPlease note that this will automatically edit the associated file extensions as well.");
  QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), leMimeTypes->text() );
  KMimeTypeChooserDialog *d = new KMimeTypeChooserDialog( i18n("Select Mime Types"), text, list, "text", this );
  if ( d->exec() == KDialogBase::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    leFileMask->setText(d->chooser()->patterns().join("; "));
    leMimeTypes->setText(d->chooser()->mimeTypes().join("; "));
  }
}
//END

//BEGIN AutoBookmarkerConfigPage
// TODO allow custom mark types with icons
AutoBookmarkerConfigPage::AutoBookmarkerConfigPage( QWidget *parent, const char *name )
  : KTextEditor::ConfigPage( parent, name )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  lo->setSpacing( KDialogBase::spacingHint() );

  QLabel *l = new QLabel( i18n("&Patterns"), this );
  lo->addWidget( l );
  lvPatterns = new KListView( this );
  lvPatterns->addColumn( i18n("Pattern") );
  lvPatterns->addColumn( i18n("Mime Types") );
  lvPatterns->addColumn( i18n("File Masks") );
  lo->addWidget( lvPatterns );
  l->setBuddy( lvPatterns );
  lvPatterns->setWhatsThis(i18n(
      "<p>This list shows your configured autobookmark entities. When a document "
      "is opened, each entity is used in the following way: "
      "<ol>"
      "<li>The entity is dismissed, if a mime and/or filename mask is defined, "
      "and neither matches the document.</li>"
      "<li>Otherwise each line of the document is tried against the pattern, "
      "and a bookmark is set on matching lines.</li></ul>"
      "<p>Use the buttons below to manage your collection of entities.</p>") );

  QHBoxLayout *lo1 = new QHBoxLayout ( lo );
  lo1->setSpacing( KDialogBase::spacingHint() );

  btnNew = new QPushButton( i18n("&New..."), this );
  lo1->addWidget( btnNew );
  btnNew->setWhatsThis(i18n(
      "Press this button to create a new autobookmark entity.") );

  btnDel = new QPushButton( i18n("&Delete"), this );
  lo1->addWidget( btnDel );
  btnDel->setWhatsThis(i18n(
      "Press this button to delete the currently selected entity.") );

  btnEdit = new QPushButton( i18n("&Edit..."), this );
  lo1->addWidget( btnEdit );
  btnEdit->setWhatsThis(i18n(
      "Press this button to edit the currently selected entity.") );

  lo1->addStretch( 1 );

  connect( btnNew, SIGNAL(clicked()), this, SLOT(slotNew()) );
  connect( btnDel, SIGNAL(clicked()), this, SLOT(slotDel()) );
  connect( btnEdit, SIGNAL(clicked()), this, SLOT(slotEdit()) );
  connect( lvPatterns, SIGNAL(doubleClicked(Q3ListViewItem *)), this, SLOT(slotEdit()) );

  m_ents = new ABEntityList();
  m_ents->setAutoDelete( true );
  reset();
}

// replace the global list with the new one
void AutoBookmarkerConfigPage::apply()
{
  ABGlobal::self()->entities()->clear();

  ABEntityListIterator it ( *m_ents );
  AutoBookmarkEnt *e;

  while ( (e = it.current()) != 0 )
  {
    ABGlobal::self()->entities()->append( e );
    ++it;
  }

  ABGlobal::self()->writeConfig();

  // TODO -- how do i refresh all the view menus
}

// renew our copy of the global list
void AutoBookmarkerConfigPage::reset()
{
  m_ents->clear(); // unused - no reset button currently

  ABEntityListIterator it ( *ABGlobal::self()->entities() );
  AutoBookmarkEnt *e;
  while ( (e = it.current()) != 0 )
  {
    AutoBookmarkEnt *me = new AutoBookmarkEnt( *e );
    m_ents->append( me );
    new AutoBookmarkEntItem( lvPatterns, me );
    ++it;
  }
}

// TODO (so far not used) we have no defaults (except deleting all items??)
void AutoBookmarkerConfigPage::defaults()
{
  // if KMessageBox::warningYesNo()
  // clear all
}

// open the edit dialog with a new entity,
// and add it if the dialog is accepted
void AutoBookmarkerConfigPage::slotNew()
{
  AutoBookmarkEnt *e = new AutoBookmarkEnt();
  AutoBookmarkerEntEditor dlg( this, e );
  if ( dlg.exec() )
  {
    dlg.apply();
    new AutoBookmarkEntItem( lvPatterns, e );
    m_ents->append( e );
  }
}

// delete the selected item and remove it from the list view and internal list
void AutoBookmarkerConfigPage::slotDel()
{
  AutoBookmarkEntItem *i = (AutoBookmarkEntItem*)lvPatterns->currentItem();
  int idx = m_ents->findRef( i->ent );
  m_ents->remove( idx );
  delete i;
}

// open the edit dialog with the selected item
void AutoBookmarkerConfigPage::slotEdit()
{
  AutoBookmarkEnt *e = ((AutoBookmarkEntItem*)lvPatterns->currentItem())->ent;
  AutoBookmarkerEntEditor dlg( this, e );
  if ( dlg.exec() )
  {
    dlg.apply();
    ((AutoBookmarkEntItem*)lvPatterns->currentItem())->redo();
  }
}
//END AutoBookmarkerConfigPage

//BEGIN AutoBookmarkEnt
AutoBookmarkEnt::AutoBookmarkEnt( const QString &p, const QStringList &f, const QStringList &m, int fl )
  : pattern( p ),
    filemask( f ),
    mimemask( m ),
    flags( fl )
{;
}
//END
//
#include "autobookmarker.moc"
