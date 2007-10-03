/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
 */

//BEGIN Includes
#include "filetemplates.h"


#include <kaboutdata.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kdirwatch.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
// #include <knewstuff2/core/entry.h>
#include <knewstuff2/engine.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kurlrequester.h>
#include <kuser.h>
#include <kxmlguifactory.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qregexp.h>
#include <qstyle.h>
//Added by qt3to4:
#include <QTextStream>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QApplication>
#include <khbox.h>

#include <QWizardPage>
#include <QTreeWidget>

#include <stdlib.h>

#include <kdebug.h>
#include <ktexteditor/templateinterface.h>
#include <krecentfilesaction.h>

#include <kgenericfactory.h>
#include <kmenu.h>
//END Includes

//BEGIN plugin + factory stuff

K_EXPORT_COMPONENT_FACTORY( katefiletemplates,
                            KGenericFactory<KateFileTemplates>( "katefiletemplates" ) )

//END

//BEGIN PluginViewKateFileTemplates
PluginViewKateFileTemplates::PluginViewKateFileTemplates(KateFileTemplates *plugin, Kate::MainWindow *mainwindow):
  Kate::PluginView(mainwindow),KXMLGUIClient(),m_plugin(plugin)
{
  QAction *a = actionCollection()->addAction("settings_manage_templates");
  a->setText(i18n("&Manage Templates..."));
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( slotEditTemplate() ) );

  a=new KActionMenu( i18n("New From &Template"), actionCollection());
  actionCollection()->addAction("file_new_fromtemplate",a);
  refreshMenu();

  setComponentData (KComponentData("kate"));
  setXMLFile("plugins/katefiletemplates/ui.rc");
  mainwindow->guiFactory()->addClient (this);

}

void PluginViewKateFileTemplates::refreshMenu()
{
  m_plugin->refreshMenu( ((KActionMenu*)(actionCollection()->action("file_new_fromtemplate")))->menu());
}

PluginViewKateFileTemplates::~PluginViewKateFileTemplates()
{
}
//END PluginViewKateFileTemplates

//BEGIN TemplateInfo
class TemplateInfo
{
  public:
    TemplateInfo( const QString& fn, const QString &t, const QString &g )
        : filename( fn ), tmplate ( t ), group( g ) { ; }
    ~TemplateInfo() { ; }

    QString filename;
    QString tmplate;
    QString group;
    QString description;
    QString author;
    QString highlight;
    QString icon;
};
Q_DECLARE_METATYPE(TemplateInfo*)
//END TemplateInfo

//BEGIN KateFileTemplates
KateFileTemplates::KateFileTemplates( QObject* parent, const QStringList &dummy)
    : Kate::Plugin ( (Kate::Application*)parent)
{
    Q_UNUSED(dummy)
//   // create actions, so that they are shared.
//   // We plug them into each view's menus, and update them centrally, so that
//   // new plugins can automatically become visible in all windows.
   mActionAny =  new KAction ( i18n("Any File..."), this );
   connect( mActionAny, SIGNAL(triggered(bool)), this, SLOT(slotAny()) );


  // template menu
  m_dw = new KDirWatch( this);
  m_dw->setObjectName( "template_dirwatch" );
  QStringList dirs = KGlobal::dirs()->findDirs("data", "kate/plugins/katefiletemplates/templates");
  for ( QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it )
  {
    m_dw->addDir( *it, KDirWatch::WatchFiles );
  }

  connect( m_dw, SIGNAL(dirty(const QString&)),
           this, SLOT(updateTemplateDirs(const QString&)) );
  connect( m_dw, SIGNAL(created(const QString&)),
           this, SLOT(updateTemplateDirs(const QString&)) );
  connect( m_dw, SIGNAL(deleted(const QString&)),
           this, SLOT(updateTemplateDirs(const QString&)) );

//   m_templates.setAutoDelete( true );
  updateTemplateDirs();

  m_user = 0;
  m_emailstuff = 0;
}

/**
 * Called whenever the template dir is changed. Recreates the templates list.
 */
void KateFileTemplates::updateTemplateDirs(const QString &d)
{
  kDebug()<<"updateTemplateDirs called with arg "<<d;

  QStringList templates = KGlobal::dirs()->findAllResources(
      "data","kate/plugins/katefiletemplates/templates/*.katetemplate",
      KStandardDirs::NoDuplicates);

  m_templates.clear();

  QRegExp re( "\\b(\\w+)\\s*=\\s*(.+)(?:\\s+\\w+=|$)" );
  re.setMinimal( true );

  KConfigGroup cg( KGlobal::config(), "KateFileTemplates" );
  QStringList hidden;
  cg.readEntry( "Hidden", hidden, ';' );

  for ( QStringList::Iterator it=templates.begin(); it != templates.end(); ++it )
  {
    QFile _f( *it );
    if ( _f.open( QIODevice::ReadOnly ) )
    {
      QString fname = (*it).section( '/', -1 );

      // skip if hidden
      if ( hidden.contains( fname ) )
        continue;

      // Read the first line of the file, to get the group/name
      TemplateInfo *tmp = new TemplateInfo( *it, fname, i18nc( "@item:inmenu", "Other" ) );
      bool trymore ( true );
      QTextStream stream(&_f);
      while ( trymore )
      {
        QString _line = stream.readLine();
        trymore = _line.startsWith( "katetemplate:" );
        if ( ! trymore ) break;

        int pos ( 0 );
        while ( ( ( pos = re.search( _line, pos ) ) >= 0 ) )
        {
          pos += re.cap( 1 ).length();
          if ( re.cap( 1 ).lower() == "template" )
            tmp->tmplate = i18nc( "@item:inmenu", re.cap( 2 ).toUtf8() );
          if ( re.cap( 1 ).lower() == "group" )
            tmp->group = i18nc( "@item:inmenu", re.cap( 2 ).toUtf8() );
          if ( re.cap( 1 ).lower() == "description" )
            tmp->description = i18nc( "@info:whatsthis", re.cap( 2 ).toUtf8() );
          if ( re.cap( 1 ).lower() == "author" )
            tmp->author = i18nc( "@info:credit", re.cap( 2 ).toUtf8() );
          if ( re.cap( 1 ).lower() == "highlight" )
            tmp->highlight = re.cap( 2 );
          if ( re.cap( 1 ) == "icon" )
            tmp->icon = re.cap( 2 );
        }
      }

      m_templates.append( tmp );
      _f.close();
    }
  }

  emit triggerMenuRefresh();

}

KateFileTemplates::~KateFileTemplates()
{
//   m_acRecentTemplates->saveEntries( KGlobal::config(), "Recent Templates" );
  delete m_emailstuff;
  delete m_user;
}

Kate::PluginView *KateFileTemplates::createView (Kate::MainWindow *mainWindow)
{
  PluginViewKateFileTemplates *view=new PluginViewKateFileTemplates(this,mainWindow);
  connect(this,SIGNAL(triggerMenuRefresh()),view,SLOT(refreshMenu()));
  return view;
}

QStringList KateFileTemplates::groups()
{
  QStringList l;
  QString s;

  for ( int i = 0; i < m_templates.count(); i++ )
  {
    s = m_templates[ i ]->group;
    if ( ! l.contains( s ) )
      l.append( s );
  }

  return l;
}

void KateFileTemplates::refreshMenu( KMenu *menu )
{

  // clear the menu for templates
  menu->clear();

  // restore it
  menu->addAction( mActionAny );
  menu->addSeparator();

  QMap<QString, QMenu*> submenus; // ### QMAP
  for ( int i = 0; i < m_templates.count(); i++ )
  {
    if ( ! submenus[ m_templates[ i ]->group ] )
    {
      QMenu *sm=menu->addMenu(m_templates[ i ]->group);
      submenus.insert( m_templates[ i ]->group, sm );
    }
//     kDebug()<<"=== ICON: '"<<m_templates[ i ].icon<<"'";
    QMenu *sm=submenus[m_templates.at(i)->group];
    QAction *a;
    if ( ! m_templates[ i ]->icon.isEmpty() )
      a=sm->addAction(
        KIcon( m_templates[ i ]->icon ),
        m_templates[ i ]->tmplate);
    else
      a=sm->addAction(
        m_templates[ i ]->tmplate);
    a->setData(i);
    connect(a,SIGNAL(triggered(bool)),this,SLOT(slotOpenTemplate()));

    // add whatsthis containing the description and author
    QString w ( m_templates[ i ]->description );
    if( ! m_templates[ i ]->author.isEmpty() )
    {
      w.append( "<p>Author: " );
      w.append( m_templates[ i ]->author );
    }
    if ( ! w.isEmpty() )
      w.prepend( "<p>" );

    if ( ! w.isEmpty() )
      a->setWhatsThis( w );
  }
}

/**
 * Action slot: use any file as a template.
 * Get a URL and pass it on.
 */
void KateFileTemplates::slotAny()
{
  if (!application()->activeMainWindow())
    return;

  // get a URL and pass that to slotOpenTemplate
  QString fn = KFileDialog::getOpenFileName(
                          KUrl(),
                          QString(),
                          application()->activeMainWindow()->activeView(),
                          i18n("Open as Template") );
  if ( ! fn.isEmpty() )
    slotOpenTemplate( KUrl( fn ) );
}

/**
 * converts template [index] to a URL and passes that
 */
void KateFileTemplates::slotOpenTemplate()
{
  int index=((QAction*)sender())->data().toInt();
  kDebug()<<"slotOpenTemplate( "<<index<<" )";
  if ( index < 0 || index > m_templates.count() ) return;
  slotOpenTemplate( KUrl( m_templates.at( index )->filename ) );
}

void KateFileTemplates::slotOpenTemplate( const KUrl &url )
{
  // check if the file can be opened
  QString tmpfile;
  QString filename = url.fileName();
  kDebug()<<"file: "<<filename;
  if ( KIO::NetAccess::download( url, tmpfile, 0L ) )
  {
    bool isTemplate ( filename.endsWith( ".katetemplate" ) );
    QString docname;

    // open the file and parse for template variables and macros
    QFile file(tmpfile);
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
      KMessageBox::sorry( application()->activeMainWindow()->activeView(),
                          i18n("<qt>Error opening the file<br /><strong>%1</strong><br />for reading. The document will not be created.</qt>", filename),
                          i18n("Template Plugin"), 0 );
      KIO::NetAccess::removeTempFile( tmpfile );
      return;
    }

    // this may take a moment..
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // create a new document
    application()->activeMainWindow()->openUrl( KUrl() );
    KTextEditor::View *view = application()->activeMainWindow()->activeView();
    KTextEditor::Document *doc = view->document();


    QTextStream stream(&file);
    QString str, tmp;
    uint numlines = 0;
    uint doneheader = 0;
    while ( !stream.atEnd() ) {
      tmp = stream.readLine();
      if ( ! numlines && isTemplate && tmp.startsWith( "katetemplate:" ) )
      {
        // look for document name, highlight
        if ( ! (doneheader & 1) )
        {
          QRegExp reName( "\\bdocumentname\\s*=\\s*(.+)(?:\\s+\\w+\\s*=|$)", false );
          reName.setMinimal( true );
          if ( reName.search( tmp ) > -1 )
          {
            docname = reName.cap( 1 );
            docname = docname.replace( "%N", "%1" );
            doneheader |= 1;
          }
        }

        if ( ! (doneheader & 2) )
        {
          QRegExp reHl( "\\bhighlight\\s*=\\s*(.+)(?:\\s+\\w+\\s*=|$)", false );
          reHl.setMinimal( true );
            kDebug()<<"looking for a hl mode";
          if ( reHl.search( tmp ) > -1 )
          {
            kDebug()<<"looking for a hl mode -- "<<reHl.cap();
            // this is overly complex, too bad the interface is
            // not providing a reasonable method..
            QString hlmode = reHl.cap( 1 );
            doc->setMode (hlmode);

            doneheader |= 2;
          }
        }

        continue; // skip this line
      }
      if ( numlines )
        str += "\n";
      str += tmp;
      numlines++;
    }
    file.close();
    KIO::NetAccess::removeTempFile( tmpfile );

    uint line, col;
    line = col = 0;

    if ( ! isTemplate )
    {
      int d = filename.findRev('.');
// ### warning i18n: Hack to have localized number later...
      docname = i18n("Untitled %1", QString("%1"));
      if ( d > 0 ) docname += filename.mid( d );
    } else if ( docname.isEmpty() )
      docname = filename.left( filename.length() - 13 );

    // check for other documents matching this naming scheme,
    // and do a count before choosing a name for this one
    QString p = docname;
    p.replace( "%1", "\\d+" );
    p.replace( ".", "\\." );
    p.prepend( "^" );
    p.append( "$" );
    QRegExp reName( p );

    int count = 1;
    const QList<KTextEditor::Document*> docs=application()->documentManager()->documents();
    foreach(const KTextEditor::Document *doc,docs) {
      if ( ( reName.search ( doc->documentName() ) > -1 ) )
        count++;
    }
    if ( docname.contains( "%1" ) )
//### warning i18n: ...localized number here

      docname = docname.arg( i18n("%1", count) );
//### warning FIXME, setDocName is gone
#if 0
    doc->setDocName( docname );
#endif
    doc->setModified( false );

    QApplication::restoreOverrideCursor();
//     m_acRecentTemplates->addUrl( url );

    // clean up
    delete m_user;
    m_user = 0;
    delete m_emailstuff;
    m_emailstuff = 0;
    if (isTemplate) {
	    KTextEditor::TemplateInterface *ti=qobject_cast<KTextEditor::TemplateInterface*>(doc->activeView());
	    ti->insertTemplateText(KTextEditor::Cursor(0,0),str,QMap<QString,QString>());
    } else {
      doc->insertText( KTextEditor::Cursor(0, 0), str );
      view->setCursorPosition(KTextEditor::Cursor(line, col));
    }
  }
}


QWidget *KateFileTemplates::parentWindow()
{
  return dynamic_cast<QWidget*>(application()->activeMainWindow());
}

// The next part are tools to aid the creation and editing of templates
// /////////////////////////////////////////////////////////////////////
// Steps to produce a template
// * Choose a file to start from (optional)
// * Ask for a location to store the file -- suggesting either the file
//   directory, or the local template directory.
//   Set the URL
// * Get the template properties -- provide a dialog, which has filled in what
//   we already know -- the author name, list of known groups
//
// Combine those data into the editor, and tell the user to position the cursor
// and edit the file as she wants to...
void KateFileTemplates::slotCreateTemplate()
{
  KateTemplateWizard w( parentWindow(), this );
  w.exec();

  updateTemplateDirs();
}

// Tools for editing the existing templates
// Editing a template:
// * Select the template to edit
// * Open the template
// * Set the URL to a writable one if required
void KateFileTemplates::slotEditTemplate()
{
  KDialog dlg( parentWindow());
  dlg.setModal(true);
  dlg.setCaption(i18n("Manage File Templates"));
  dlg.setButtons(KDialog::Close);
  dlg.setMainWidget( new KateTemplateManager( this, &dlg ) );
  dlg.exec();
}
//END KateFileTemplates

//BEGIN KateTemplateInfoWidget
// This widget can be used to change the data of a TemplateInfo object
KateTemplateInfoWidget::KateTemplateInfoWidget( QWidget *parent, TemplateInfo *info, KateFileTemplates *kft )
  : QWidget( parent ),
    info( info ),
    kft( kft )
{
  QGridLayout *lo = new QGridLayout( this, 6, 2 );
  lo->setAutoAdd( true );
  lo->setSpacing( KDialog::spacingHint() );

  QLabel *l = new QLabel( i18n("&Template:"), this );
  KHBox *hb = new KHBox( this );
  hb->setSpacing( KDialog::spacingHint() );
  leTemplate = new QLineEdit( hb );
  l->setBuddy( leTemplate );
  leTemplate->setToolTip( i18n("<p>This string is used as the template's name "
      "and is displayed, for example, in the Template menu. It should describe the "
      "meaning of the template, for example 'HTML Document'.</p>") );
  ibIcon = new KIconButton( hb );
  ibIcon->setToolTip(i18n(
      "Press to select or change the icon for this template") );

  l = new QLabel( i18n("&Group:"), this );
  cmbGroup = new QComboBox( true, this );
  cmbGroup->insertStringList( kft->groups() );
  l->setBuddy( cmbGroup );
  cmbGroup->setToolTip(i18n("<p>The group is used for choosing a "
      "submenu for the plugin. If it is empty, 'Other' is used.</p>"
      "<p>You can type any string to add a new group to your menu.</p>") );

  l = new QLabel( i18n("Document &name:"), this );
  leDocumentName = new QLineEdit( this );
  l->setBuddy( leDocumentName );
  leDocumentName->setToolTip( i18n("<p>This string will be used to set a name "
      "for the new document, to display in the title bar and file list.</p>"
      "<p>If the string contains '%N', that will be replaced with a number "
      "increasing with each similarly named file.</p><p> For example, if the "
      "Document Name is 'New shellscript (%N).sh', the first document will be "
      "named 'New shellscript (1).sh', the second 'New shellscipt (2).sh', and "
      "so on.</p>") );

  l = new QLabel( i18n( "&Highlight:"), this );
  btnHighlight = new QPushButton( i18n("None"), this );
  l->setBuddy( btnHighlight );
  btnHighlight->setToolTip( i18n("<p>Select the highlight to use for the "
    "template. If 'None' is chosen, the property will not be set.</p>") );

  l = new QLabel( i18n("&Description:"), this );
  leDescription = new QLineEdit( this );
  l->setBuddy( leDescription );
  leDescription->setToolTip(i18n("<p>This string is used, for example, as "
      "context help for this template (such as the 'whatsthis' help for the "
      "menu item.)</p>") );

  l = new QLabel( i18n("&Author:"), this );
  leAuthor = new QLineEdit( this );
  l->setBuddy( leAuthor );
  leAuthor->setToolTip(i18n("<p>You can set this if you want to share your "
      "template with other users.</p>"
      "<p>the recommended form is like an Email "
      "address: 'Anders Lund &lt;anders@alweb.dk&gt;'</p>") );

  // if we have a object ! null
  if ( info )
  {
    if ( ! info->icon.isEmpty() )
      ibIcon->setIcon( info->icon );
    leTemplate->setText( info->tmplate );
    cmbGroup->setCurrentText( info->group );
    leDescription->setText( info->description );
    leAuthor->setText( info->author );
    if ( ! info->highlight.isEmpty() )
      btnHighlight->setText( info->highlight );
  }

  // fill in the Hl menu
  KTextEditor::Document *doc = kft->application()->activeMainWindow()->activeView()->document();
  if ( doc )
  {
    QStringList highlightModes = doc->highlightingModes();
      QMenu *m = new QMenu( btnHighlight );
      connect( m, SIGNAL( triggered( QAction* ) ), this, SLOT( slotHlSet( QAction* ) ) );
      QMap<QString, QMenu*> submenus;
      for ( int n = 0; n < highlightModes.count(); n++ )
      {
// ### I can't access the hl groups through the current interfaces :-(
//         // create the sub menu if it does not exist
//         QString text( highlightModes[ i ] );
//         if ( ! text.isEmpty() )
//         {
//           if ( ! submenus[ text ] )
//           {
//             QMenu *sm = m.addMenu( text );
//             connect( sm, SIGNAL( triggered( QAction* ) ), this, SLOT( slotHlSet( QAction* ) ) );
//             submenus->insert( text, sm );
//           }
//           // create the item
//           QAction *a = submenus[ text ]->addAction( hi->hlModeName( n ) );
//           a->setData( n );
//         }
//         else {
          QAction *a = m->addAction( highlightModes[ n ] );
          a->setData( n );
//         }
      }
      btnHighlight->setPopup( m );
  }
}

void KateTemplateInfoWidget::slotHlSet( QAction *action )
{
  KTextEditor::Document *doc=kft->application()->activeMainWindow()->activeView()->document();
  if (doc)
    btnHighlight->setText( action->text() ); // fixme
}
//END KateTemplateInfoWidget

//BEGIN KateTemplateWizard
// A simple wizard to help create a new template :-)
KateTemplateWizard::KateTemplateWizard( QWidget *parent, KateFileTemplates *kft )
  : QWizard( parent ),
    kft( kft )
{
  // 1) Optionally chose a file or existing template to start from
  QWizardPage *page = new QWizardPage;
  page->setTitle("Template Origin");
  page->setSubTitle("If you want to base this "
    "template on an existing file or template, select the appropriate option "
    "below.");
    
  addPage( page );

  QGridLayout *glo = new QGridLayout( page );
  //lo->setAutoAdd( true );
  glo->setSpacing( KDialog::spacingHint() );

  bgOrigin = new QButtonGroup( page );
//   bgOrigin->hide();
  bgOrigin->setExclusive( true );

  QRadioButton *rb = new QRadioButton( i18n("Start with an &empty document" ), page );
  bgOrigin->addButton( rb, 1 );
  glo->addWidget( rb, 1, 1, 1, 2 );
  rb->setChecked( true );

  rb = new QRadioButton( i18n("Use an existing file:"), page );
  bgOrigin->addButton( rb, 2 );
  glo->addWidget( rb, 2, 1, 1, 2 );
// ### warning 0 could be wrong here: it crashes with Plastik
  // int marg = rb->style()->subElementRect( QStyle::SE_RadioButtonIndicator, 0,rb ).width();
  int marg = KDialog::marginHint();
  glo->addItem( new QSpacerItem( marg, 1, QSizePolicy::Fixed ), 3, 1 );
  urOrigin = new KUrlRequester( page );
  glo->addWidget( urOrigin, 3, 2 );

  rb = new QRadioButton( i18n("Use an existing template:"), page );
  bgOrigin->addButton( rb, 3 );
  glo->addWidget( rb, 4, 1, 1, 2 );
  glo->addItem( new QSpacerItem( marg, 1, QSizePolicy::Fixed ), 5, 1 );
  btnTmpl = new QPushButton( page );
  glo->addWidget( btnTmpl, 5, 2 );
  QMenu *m = new QMenu( btnTmpl );
  connect( m, SIGNAL( triggered( QAction* ) ), this, SLOT( slotTmplateSet( QAction* ) ) );

  QMap<QString, QMenu*> submenus;
  for ( int i = 0; i < kft->templates().count(); i++ )
  {
    if ( ! submenus[ kft->templates()[ i ]->group ] )
    {
      QMenu *sm = m->addMenu( kft->templates()[ i ]->group );
      connect( sm, SIGNAL( triggered( QAction* ) ), this, SLOT( slotTmplateSet( QAction* ) ) );
      submenus.insert( kft->templates()[ i ]->group, sm );
    }

    QAction *a = submenus[kft->templates()[ i ]->group]->addAction(
        kft->templates()[ i ]->tmplate );
    a->setData( i );
  }
  btnTmpl->setPopup( m );

  connect( bgOrigin, SIGNAL(buttonClicked(int)), this, SLOT(slotStateChanged()) );
  connect( urOrigin, SIGNAL(textChanged(const QString&)), this, SLOT(slotStateChanged()) );

  glo->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ), 7, 7, 1, 2 );

  // 2) edit the template properties
  page = new QWizardPage;
  page->setTitle( i18n("Edit Template Properties") );
  page->setSubTitle( i18n("Specify the main properties of your plugin. You can leave fields empty for which you have no meaningful value.") );
  glo = new QGridLayout( page );
  kti = new KateTemplateInfoWidget( page, 0, kft );
  glo->addWidget( kti, 1, 1 );
  addPage( page );
  
  // get liekly values from KTE
  QMap<QString, QString> map;
  map[ "fullname" ] = "";
  map[ "email" ] = "";

  KTextEditor::TemplateInterface::expandMacros( map, parent );
  QString sFullname = map["fullname"];
  QString sEmail = map["email"];
  QString _s = sFullname;
  if ( ! sEmail.isEmpty() )
    _s += " <" + sEmail + ">";
  kti->leAuthor->setText( _s );

  // 3) chose a location - either the template directory (default) or
  // a custom location
  page = new QWizardPage;
  page->setTitle( i18n("Choose Location") );
  page->setSubTitle( i18n("<p>Choose a location for the "
    "template. If you store it in the template directory, it will "
    "automatically be added to the template menu.</p>") );
  glo = new QGridLayout( page, 7, 2 );
  glo->setSpacing( KDialog::spacingHint() );

  bgLocation = new QButtonGroup( page );
//   bgLocation->hide();
  bgLocation->setExclusive( true );

  rb = new QRadioButton( i18n("Template directory"), page );
  bgLocation->addButton( rb, 1 );
  glo->addMultiCellWidget( rb, 2, 2, 1, 2 );
  rb->setChecked( true );

  glo->addMultiCell( new QSpacerItem( marg, 1, QSizePolicy::Fixed ), 3, 4, 1, 1 );
  leTemplateFileName = new QLineEdit( page );
  QLabel *l = new QLabel( leTemplateFileName, i18n("Template &file name:"), page );

  glo->addWidget( l, 3, 2 );
  glo->addWidget( leTemplateFileName, 4, 2 );

  rb = new QRadioButton( i18n("Custom location:"), page );
  bgLocation->addButton( rb, 2 );
  glo->addMultiCellWidget( rb, 5, 5, 1, 2 );

  glo->addItem( new QSpacerItem( marg, 1, QSizePolicy::Fixed ), 6, 1 );
  urLocation = new KUrlRequester( page );
  glo->addWidget( urLocation, 6, 2 );

  connect( bgLocation, SIGNAL(buttonClicked(int)), this, SLOT(slotStateChanged()) );
  connect( urLocation, SIGNAL(textChanged(const QString&)), this, SLOT(slotStateChanged()) );
  connect( leTemplateFileName, SIGNAL(textChanged(const QString &)), this, SLOT(slotStateChanged()) );

  glo->addMultiCell( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ), 7, 7, 1, 2 );

  addPage( page );
  // 4) Should we edit the text to add some macros, replacing username etc?
  // This is *only* relevant if the origin is a non-template file.
  page = new QWizardPage;
  page->setTitle(  i18n("Autoreplace Macros") );
  page->setSubTitle( i18n( "You can replace certain strings in the text with "
      "template macros. If any of the data below is incorrect or missing, "
      "edit the data in the personal kaddressbook entry.") );
  QVBoxLayout *lo = new QVBoxLayout( page );
  lo->setSpacing( KDialog::spacingHint() );

  cbRRealname = new QCheckBox( i18n("Replace full name '%1' with the "
    "'%{fullname}' macro", sFullname ), page );
  cbRRealname->setEnabled( ! sFullname.isEmpty() );
  lo->addWidget( cbRRealname );

  cbREmail = new QCheckBox( i18n("Replace email address '%1' with the "
      "'%email' macro", sEmail ), page);
  cbREmail->setEnabled( ! sEmail.isEmpty() );
  lo->addWidget( cbREmail );

  lo->addStretch();

  addPage( page );

  // 5) Display a summary
  page = new QWizardPage;
  page->setTitle( i18n("Create Template") );
  page->setSubTitle( i18n("The template will now be created and saved to the chosen "
      "location. To position the cursor put the string ${|} where you "
      "want it in files created from the template.") );
  lo = new QVBoxLayout( page );
  lo->setSpacing( KDialog::spacingHint() );

  cbOpenTemplate = new QCheckBox( i18n("Open the template for editing in Kate"), page );

  lo->addWidget( cbOpenTemplate );

  lo->addStretch();

  addPage( page );
   connect( this, SIGNAL(currentIdChanged(int)), this, SLOT(slotStateChanged()) );
}

void KateTemplateWizard::slotTmplateSet( QAction *action )
{
  int idx = action->data().toInt();
  btnTmpl->setText( kft->templates().at( idx )->tmplate );
  selectedTemplateIdx = idx;
  slotStateChanged();
}

/**
 * When the state of any button in any setup page is changed, set the
 * enabled state of the next button accordingly.
 *
 * Origin:
 * if file is chosen, the URLRequester must have a valid URL in it
 * if template is chosen, one must be selected in the menu button.
 *
 * Props:
 * anything goes, but if the user wants to store the template in the template
 * directory, she should be encouraged to fill in information.
*/
void KateTemplateWizard::slotStateChanged()
{
  bool sane( true );
  switch ( currentId() )
  {
    case 0: // origin
    {
      int _t = bgOrigin->checkedId();
      kDebug()<<"selected button:"<<_t;
      sane = ( _t == 1 ||
               ( _t == 2 && ! urOrigin->url().isEmpty() ) ||
               ( _t == 3 && ! btnTmpl->text().isEmpty() ) );
//       setAppropriate( page(3), _t == 2 );
    }
    break;
    case 1: // template properties
      // if origin is a existing template, let us try setting some of the properties
      if ( bgOrigin->checkedId() == 3 )
      {
        TemplateInfo *info = kft->templateInfo( selectedTemplateIdx );
        kti->cmbGroup->setCurrentText( info->group );
      }
    break;
    case 2: // location
    {
      // If there is a template name, and the user did not enter text into
      // the template file name entry, we will construct the name from the
      // template name.
      int _t = bgLocation->checkedId();
      sane = ( ( _t == 1 && (! leTemplateFileName->text().isEmpty() || ! kti->leTemplate->text().isEmpty() ) ) ||
          ( _t == 2 && ! urLocation->url().isEmpty() ) );
    }
    break;
    default:
    break;
  }
  kDebug()<<"enabling 'next' button:"<<sane;
  button( QWizard::NextButton )->setEnabled( sane );
}

int KateTemplateWizard::nextId()
{
  switch ( currentId() )
  {
//     case 0:
//       if ( bgOrigin->checkedId() == 2 ) return 2;
//       else return 1;
    default:
      return QWizard::nextId();
  }
}
/**
 * This will create the new template based on the collected information.
 */
void KateTemplateWizard::accept()
{
  // TODO check that everything is kosher, so that we can get a save location
  // etc.

  // check that we can combine a valid URL
  KUrl templateUrl;
  if ( bgLocation->checkedId() == 1 )
  {
    QString suggestion;
    if ( ! leTemplateFileName->text().isEmpty() )
      suggestion = leTemplateFileName->text();
    else
      suggestion = kti->leTemplate->text();

    suggestion.replace(" ", "");

    if ( ! suggestion.endsWith(".katetemplate") )
      suggestion.append(".katetemplate");

    QString dir = KGlobal::dirs()->saveLocation( "data", "kate/plugins/katefiletemplates/templates/", true );

    templateUrl = dir + suggestion;

    if ( QFile::exists( templateUrl.path() ) )
    {
      if ( KMessageBox::warningContinueCancel( this, i18n(
          "<p>The file <br /><strong>'%1'</strong><br /> already exists; if you "
          "do not want to overwrite it, change the template file name to "
          "something else.</p>", templateUrl.prettyUrl() ),
      i18n("File Exists"), KGuiItem(i18n("Overwrite") ))
           == KMessageBox::Cancel )
        return;
    }
  }
  else
  {
    templateUrl = urLocation->url();
  }

  QWizard::accept();
  // The following must be done:
  // 1) add the collected template information to the top
  uint ln = 0;
  QString s, str;
  if ( ! kti->leTemplate->text().isEmpty() )
    s += " Template=" + kti->leTemplate->text();
  if ( ! kti->cmbGroup->currentText().isEmpty() )
    s += " Group=" + kti->cmbGroup->currentText();
  if ( ! kti->leDocumentName->text().isEmpty() )
    s += " Documentname=" + kti->leDocumentName->text();
  if ( ! kti->ibIcon->icon().isEmpty() )
    s += " Icon=" + kti->ibIcon->icon();
  if ( ! kti->btnHighlight->text().isEmpty() )
    s += " Highlight=" + kti->btnHighlight->text();

  str = "katetemplate:" + s;

  if ( ! (s = kti->leAuthor->text()).isEmpty() )
    str += "\nkatetemplate: Author=" + s;

  if ( ! (s = kti->leDescription->text()).isEmpty() )
    str += "\nkatetemplate: Description=" + s;

  //   2) If a file or template is chosen, open that. and fill the data into a string
  int toid = bgOrigin->checkedId(); // 1 = blank, 2 = file, 3 = template
  kDebug()<<"=== create template: origin type "<<toid;
  if ( toid > 1 )
  {
    KUrl u;
    if ( toid  == 2 ) // file
      u = KUrl( urOrigin->url() );
    else // template
      u = KUrl( kft->templates().at( selectedTemplateIdx )->filename );

    QString tmpfile, tmp;
    if ( KIO::NetAccess::download( u, tmpfile, 0L ) )
    {
      QFile file(tmpfile);
      if ( ! file.open( QIODevice::ReadOnly ) )
      {
        KMessageBox::sorry( this, i18n(
            "<qt>Error opening the file<br /><strong>%1</strong><br />for reading. "
            "The document will not be created</qt>", u.prettyUrl()),
            i18n("Template Plugin"), 0 );

        KIO::NetAccess::removeTempFile( tmpfile );
        return;
      }

      QTextStream stream(&file);
      QString ln;
      bool trymore = true;
      while ( !stream.atEnd() )
      {
        // skip template headers
        ln = stream.readLine();
        if ( trymore && ln.startsWith("katetemplate:") )
          continue;

        trymore = false;
        tmp += "\n" + ln;
      }

      file.close();
      KIO::NetAccess::removeTempFile( tmpfile );
    }

    if ( toid == 2 ) // file
    {
      //   3) if the file is not already a template, escape any "%" and "^" in it,
      //      and try do do some replacement of the authors username, name and email.
            tmp.replace( QRegExp("%(?=\\{[^}]+\\})"), "\\%" );
            tmp.replace( QRegExp("\\$(?=\\{[^}]+\\})"), "\\$" );
      //tmp.replace( "^", "\\^" );

      if ( cbRRealname->isChecked() && ! sFullname.isEmpty() )
              tmp.replace( sFullname, "%{realname}" );


      if ( cbREmail->isChecked() && ! sEmail.isEmpty() )
              tmp.replace( sEmail, "%{email}" );
    }

    str += tmp;
  }

  // 4) Save the document to the suggested URL if possible

  bool succes = false;

  if ( templateUrl.isValid() )
  {
    if ( templateUrl.isLocalFile() )
    {
      QFile file( templateUrl.path() );
         if ( file.open(QIODevice::WriteOnly) )
      {
        kDebug()<<"file opened with succes";
        QTextStream stream( &file );
        stream << str;
        file.close();
        succes = true;
      }
    }
    else
    {
      KTemporaryFile tmp;
      tmp.open();
      QString fn = tmp.fileName();
      QTextStream stream( &tmp );
      stream << str;
      stream.flush();

      succes = KIO::NetAccess::upload( fn, templateUrl, 0 );
    }
  }

  if ( ! succes )
  {
    KMessageBox::sorry( this, i18n(
        "Unable to save the template to '%1'.\n\nThe template will be opened, "
        "so you can save it from the editor.", templateUrl.prettyUrl() ),
        i18n("Save Failed") );

    kft->application()->activeMainWindow()->openUrl( KUrl() );
    KTextEditor::View *view = kft->application()->activeMainWindow()->activeView();
    KTextEditor::Document *doc = view->document();
    doc->insertText( KTextEditor::Cursor(ln++, 0), str );
  }
  else if ( cbOpenTemplate->isChecked() )
    kft->application()->activeMainWindow()->openUrl( templateUrl );
}
//END KateTemplateWizard

//BEGIN KateTemplateManager
KateTemplateManager::KateTemplateManager( KateFileTemplates *kft, QWidget *parent, const char *name )
  : QWidget( parent, name )
  , kft( kft )
{
  QGridLayout *lo = new QGridLayout( this, 2, 6 );
  lo->setSpacing( KDialog::spacingHint() );
  lvTemplates = new QTreeWidget( this );
  lvTemplates->setHeaderLabel( i18n("Template") );
  lvTemplates->setSelectionMode( QAbstractItemView::SingleSelection );
  lo->addMultiCellWidget( lvTemplates, 1, 1, 1, 6 );
  connect( lvTemplates, SIGNAL(itemSelectionChanged()), this, SLOT(slotUpdateState()) );

  btnNew = new QPushButton( i18nc("@action:button Template", "New..."), this );
  connect( btnNew, SIGNAL(clicked()), kft, SLOT(slotCreateTemplate()) );
  lo->addWidget( btnNew, 2, 2 );

  btnEdit = new QPushButton( i18nc("@action:button Template", "Edit..."), this );
  connect( btnEdit, SIGNAL(clicked()), this, SLOT( slotEditTemplate()) );
  lo->addWidget( btnEdit, 2, 3 );

  btnRemove = new QPushButton( i18nc("@action:button Template", "Remove"), this );
  connect( btnRemove, SIGNAL(clicked()), this, SLOT(slotRemoveTemplate()) );
  lo->addWidget( btnRemove, 2, 4 );

  btnUpload = new QPushButton( i18nc("@action:button Template", "Upload..."), this );
  connect( btnUpload, SIGNAL(clicked()), this, SLOT(slotUpload()) );
  lo->addWidget( btnUpload, 2, 5 );

  btnDownload = new QPushButton( i18nc("@action:button Template", "Download..."), this );
  connect( btnDownload, SIGNAL(clicked()), this, SLOT(slotDownload()) );
  lo->addWidget( btnDownload, 2, 6 );

  lo->setColumnStretch( 1, 1 );

  reload();
  slotUpdateState();
}

void KateTemplateManager::apply()
{
  // if any files were removed, delete them unless they are not writeable, in
  // which case a link .filename should be put in the writable directory.
}

#define KATETEMPLATEITEM 1001
void KateTemplateManager::reload()
{
  lvTemplates->clear();

  QMap<QString, QTreeWidgetItem*> groupitems;
  for ( int i = 0; i < kft->templates().count(); i++ )
  {
    if ( ! groupitems[ kft->templates()[ i ]->group ] )
    {
      groupitems.insert( kft->templates()[ i ]->group , new QTreeWidgetItem( lvTemplates ) );
      groupitems[ kft->templates()[ i ]->group ]->setText( 0, kft->templates()[ i ]->group );
       groupitems[ kft->templates()[ i ]->group ]->setExpanded( true );
    }
    QTreeWidgetItem *item = new QTreeWidgetItem( groupitems[ kft->templates()[ i ]->group ], KATETEMPLATEITEM );
    item->setText( 0, kft->templates()[ i ]->tmplate );
    item->setData( 0, Qt::UserRole, QVariant::fromValue( kft->templates()[ i ] ) );
  }
}

void KateTemplateManager::slotUpdateState()
{
  // enable/disable buttons wrt the current item in the list view.
  // we are in single selection mode, so currentItem() is selected.
  QTreeWidgetItem *item = lvTemplates->currentItem();
  bool cool = item && item->type() == KATETEMPLATEITEM;

  btnEdit->setEnabled( cool );
  btnRemove->setEnabled( cool );
  btnUpload->setEnabled( cool );
}

void KateTemplateManager::slotEditTemplate()
{
  // open the template file in kate
  // TODO show the properties dialog, and modify the file if the data was changed.
  QList<QTreeWidgetItem*> selection = lvTemplates->selectedItems();
  
   if ( selection.count() ) {
     QTreeWidgetItem *item = selection[ 0 ];
     if ( item->type() != KATETEMPLATEITEM )
       return;
     TemplateInfo *info = item->data(0, Qt::UserRole ).value<TemplateInfo*>();
     kft->application()->activeMainWindow()->openUrl( info->filename );
   }
}

void KateTemplateManager::slotRemoveTemplate()
{
  QTreeWidgetItem *item = lvTemplates->selectedItems().first();
  if ( item && item->type() == KATETEMPLATEITEM )
  {
    // Find all instances of filename, and try to delete them.
    // If it fails (there was a global, unwritable instance), add to a
    // list of removed templates
    KSharedConfig::Ptr config = KGlobal::config();
    TemplateInfo *info =  item->data(0, Qt::UserRole).value<TemplateInfo*>();
    QString fname = info->filename.section( '/', -1 );
    QStringList templates = KGlobal::dirs()->findAllResources(
        "data", fname.prepend( "kate/plugins/katefiletemplates/templates/" ),KStandardDirs::NoDuplicates);
    int failed = 0;
    int removed = 0;
    for ( QStringList::Iterator it=templates.begin(); it!=templates.end(); ++it )
    {
      if ( ! QFile::remove(*it) )
        failed++;
      else
        removed++;
    }

    if ( failed )
    {
      KConfigGroup cg( config, "KateFileTemplates" );
      QStringList l;
      cg.readEntry( "Hidden", l, ';' );
      l << fname;
      cg.writeEntry( "Hidden", l, ';' );
    }

    // If we removed any files, we should delete a KNewStuff key
    // for this template, so the template is installable again.
    // ### This assumes that the knewstuff name is similar to the template name.
    kDebug()<<"trying to remove knewstuff key '"<<info->tmplate<<"'";
    config->group("KNewStuffStatus").deleteEntry( info->tmplate );

    kft->updateTemplateDirs();
    reload();
  }
}

// KNewStuff upload
void KateTemplateManager::slotUpload()
{
  // TODO something nicer, like preparing the meta data from the template info.
//   KateTemplateItem *item = dynamic_cast<KateTemplateItem*>( lvTemplates->currentItem() );
//   if ( item )
//   {
//     KFTNewStuff *ns = new KFTNewStuff( "katefiletemplates/template", this );
//     ns->upload( item->templateinfo->filename, QString() );
//   }
//### warning ERROR HANDLING
//   KateTemplateItem *item = dynamic_cast<KateTemplateItem*>( lvTemplates->currentItem() );
//   if (!item) return;
//   KNS::Engine *engine=new KNS::Engine(this);
//   bool success=engine->init("katefiletemplates.knsrc");
//   if (!success)
//   {
//     delete engine;
//     return;
//   }
//   engine->uploadDialogModal(item->templateinfo->filename);
//   delete engine;

}

// KNewStuff download
void KateTemplateManager::slotDownload()
{
//   KFTNewStuff *ns = new KFTNewStuff( "katefiletemplates/template", this );
//   ns->download();
//### warning ERROR HANDLING
  KNS::Engine *engine=new KNS::Engine(this);
  bool success=engine->init("katefiletemplates.knsrc");
  if (!success)
  {
    delete engine;
    return;
  }
  engine->downloadDialogModal();
  delete engine;

  kft->updateTemplateDirs();
  reload();
}

//END KateTemplateManager

// kate: space-indent on; indent-width 2; replace-tabs on;

#include "filetemplates.moc"
