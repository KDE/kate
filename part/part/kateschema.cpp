/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "kateschema.h"
#include "kateschema.moc"

#include "kateconfig.h"
#include "katefactory.h"
#include "kateview.h"
#include "katerenderer.h"

#include <klocale.h>
#include <kdialog.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <klineeditdlg.h>
#include <kfontdialog.h>
#include <kdebug.h>
#include <kpopupmenu.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qptrcollection.h>
#include <qdialog.h>
#include <qgrid.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qtextcodec.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qhbox.h>
#include <qobjectlist.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>

KateSchemaManager::KateSchemaManager ()
  : m_config ("kateschemarc", false, false)
{
  update ();
}

KateSchemaManager::~KateSchemaManager ()
{
}

//
// read the types from config file and update the internal list
//
void KateSchemaManager::update (bool readfromfile)
{
  if (readfromfile)
    m_config.reparseConfiguration ();

  m_schemas = m_config.groupList();
  m_schemas.sort ();

  m_schemas.remove ("Kate Printing Schema");
  m_schemas.remove ("Kate Normal Schema");
  m_schemas.prepend (i18n("Printing"));
  m_schemas.prepend (i18n("Normal"));
}

//
// get the right group
// special handling of the default schemas ;)
//
KConfig *KateSchemaManager::schema (uint number)
{
  if ((number>1) && (number < m_schemas.count()))
    m_config.setGroup (m_schemas[number]);
  else if (number == 1)
    m_config.setGroup ("Kate Printing Schema");
  else
    m_config.setGroup ("Kate Normal Schema");

  return &m_config;
}

void KateSchemaManager::addSchema (const QString &t)
{
  m_config.setGroup (t);
  m_config.writeEntry("Color Background", KGlobalSettings::baseColor());

  update (false);
}

void KateSchemaManager::removeSchema (uint number)
{
  if (number >= m_schemas.count())
    return;

  if (number < 2)
    return;

  m_config.deleteGroup (name (number));

  update (false);
}

bool KateSchemaManager::validSchema (uint number)
{
  if (number < m_schemas.count())
    return true;
    
  return false;
}

uint KateSchemaManager::number (const QString &name)
{
  if (name == "Kate Normal Schema")
    return 0;

  if (name == "Kate Printing Schema")
    return 1;

  int i;
  if ((i = m_schemas.findIndex(name)) > -1)
    return i;

  return 0;
}

QString KateSchemaManager::name (uint number)
{
  if ((number>1) && (number < m_schemas.count()))
    return m_schemas[number];
  else if (number == 1)
    return "Kate Printing Schema";

  return "Kate Normal Schema";
}

//
//
//
// DIALOGS !!!
//
//

//BEGIN KateSchemaConfigColorTab
KateSchemaConfigColorTab::KateSchemaConfigColorTab( QWidget *parent, const char * )
  : QWidget (parent)
{
  QHBox *b;
  QLabel *label;

  QVBoxLayout *blay=new QVBoxLayout(this, 0, KDialog::spacingHint());

  QVGroupBox *gbTextArea = new QVGroupBox(i18n("Text Area Background"), this);

  b = new QHBox (gbTextArea);
  label = new QLabel( i18n("Normal text:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_back = new KColorButton(b);
  connect( m_back, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  b = new QHBox (gbTextArea);
  label = new QLabel( i18n("Selected text:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_selected = new KColorButton(b);
  connect( m_selected, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  b = new QHBox (gbTextArea);
  label = new QLabel( i18n("Current line:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_current = new KColorButton(b);
  connect( m_current, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  blay->addWidget(gbTextArea);

  QVGroupBox *gbBorder = new QVGroupBox(i18n("Additional Elements"), this);

  b = new QHBox (gbBorder);
  label = new QLabel( i18n("Left border background:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_iconborder = new KColorButton(b);
  connect( m_iconborder, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  b = new QHBox (gbBorder);
  label = new QLabel( i18n("Bracket highlight:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_bracket = new KColorButton(b);
  connect( m_bracket, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  b = new QHBox (gbBorder);
  label = new QLabel( i18n("Word wrap markers:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_wwmarker = new KColorButton(b);
  connect( m_wwmarker, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  b = new QHBox (gbBorder);
  label = new QLabel( i18n("Tab markers:"), b);
  label->setAlignment( AlignLeft|AlignVCenter);
  m_tmarker = new KColorButton(b);
  connect( m_tmarker, SIGNAL( changed( const QColor & ) ), parent->parentWidget(), SLOT( slotChanged() ) );

  blay->addWidget(gbBorder);

  blay->addStretch();

  // QWhatsThis help
  QWhatsThis::add(m_back, i18n("<p>Sets the background color of the editing area.</p>"));
  QWhatsThis::add(m_selected, i18n("<p>Sets the background color of the selection.</p>"
        "<p>To set the text color for selected text, use the \"<b>Configure "
        "Highlighting</b>\" dialog.</p>"));
  QWhatsThis::add(m_current, i18n("<p>Sets the background color of the currently "
        "active line, which means the line where your cursor is positioned.</p>"));
  QWhatsThis::add(m_bracket, i18n("<p>Sets the bracket matching color. This means, "
        "if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will "
        "be highlighted with this color.</p>"));
  QWhatsThis::add(m_wwmarker, i18n(
        "<p>Sets the color of Word Wrap-related markers:</p>"
        "<dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where "
        "text is going to be wrapped</dd>"
        "<dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of "
        "visually-wrapped lines</dd></dl>"));
  QWhatsThis::add(m_tmarker, i18n(
        "<p>Sets the color of the tabulator marks:</p>"));
}

KateSchemaConfigColorTab::~KateSchemaConfigColorTab()
{
}

void KateSchemaConfigColorTab::readConfig (KConfig *config)
{
  QColor tmp0 (KGlobalSettings::baseColor());
  QColor tmp1 (KGlobalSettings::highlightColor());
  QColor tmp2 (KGlobalSettings::alternateBackgroundColor());
  QColor tmp3 ( "#FFFF99" );
  QColor tmp4 (tmp2.dark());
  QColor tmp5 ( KGlobalSettings::textColor() );
  QColor tmp6 ( "#EAE9E8" );

  m_back->setColor(config->readColorEntry("Color Background", &tmp0));
  m_selected->setColor(config->readColorEntry("Color Selection", &tmp1));
  m_current->setColor(config->readColorEntry("Color Highlighted Line", &tmp2));
  m_bracket->setColor(config->readColorEntry("Color Highlighted Bracket", &tmp3));
  m_wwmarker->setColor(config->readColorEntry("Color Word Wrap Marker", &tmp4));
  m_tmarker->setColor(config->readColorEntry("Color Tab Marker", &tmp5));
  m_iconborder->setColor(config->readColorEntry("Color Icon Bar", &tmp6));
}

void KateSchemaConfigColorTab::writeConfig (KConfig *config)
{
  config->writeEntry("Color Background", m_back->color());
  config->writeEntry("Color Selection", m_selected->color());
  config->writeEntry("Color Highlighted Line", m_current->color());
  config->writeEntry("Color Highlighted Bracket", m_bracket->color());
  config->writeEntry("Color Word Wrap Marker", m_wwmarker->color());
  config->writeEntry("Color Tab Marker", m_tmarker->color());
  config->writeEntry("Color Icon Bar", m_iconborder->color());
}

//END KateSchemaConfigColorTab

//BEGIN FontConfig
KateSchemaConfigFontTab::KateSchemaConfigFontTab( QWidget *parent, const char * )
  : QWidget (parent)
{
    // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  m_fontchooser = new KFontChooser ( this, 0L, false, QStringList(), false );
  m_fontchooser->enableColumn(KFontChooser::StyleList, false);
  grid->addWidget( m_fontchooser, 0, 0);

  connect (m_fontchooser, SIGNAL (fontSelected( const QFont & )), this, SLOT (slotFontSelected( const QFont & )));
  connect (m_fontchooser, SIGNAL (fontSelected( const QFont & )), parent->parentWidget(), SLOT (slotChanged()));
}

KateSchemaConfigFontTab::~KateSchemaConfigFontTab()
{
}

void KateSchemaConfigFontTab::slotFontSelected( const QFont &font )
{
  myFont = font;
}

void KateSchemaConfigFontTab::readConfig (KConfig *config)
{
  QFont f (KGlobalSettings::fixedFont());

  m_fontchooser->setFont (config->readFontEntry("Font", &f));
}

void KateSchemaConfigFontTab::writeConfig (KConfig *config)
{
  config->writeEntry("Font", myFont);
}

//END FontConfig

//BEGIN FontColorConfig
KateSchemaConfigFontColorTab::KateSchemaConfigFontColorTab( QWidget *parent, const char * )
  : QWidget (parent)
{
  m_defaultStyleLists.setAutoDelete(true);

  // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  m_defaultStyles = new StyleListView( this, false );
  grid->addWidget( m_defaultStyles, 0, 0);
  
  connect (m_defaultStyles, SIGNAL (changed()), parent->parentWidget(), SLOT (slotChanged()));
}

KateSchemaConfigFontColorTab::~KateSchemaConfigFontColorTab()
{
}

void KateSchemaConfigFontColorTab::schemaChanged (uint schema)
{
  m_defaultStyles->clear ();

  if (!m_defaultStyleLists[schema])
  {
    KateAttributeList *list = new KateAttributeList ();
    HlManager::self()->getDefaults(schema, *list);
    
    m_defaultStyleLists.insert (schema, list);
  }

  for ( uint i = 0; i < HlManager::self()->defaultStyles(); i++ )
  {
    m_defaultStyles->insertItem( new StyleListItem( m_defaultStyles, HlManager::self()->defaultStyleName(i),
                              m_defaultStyleLists[schema]->at( i ) ) );
  }
}

void KateSchemaConfigFontColorTab::reload ()
{
  m_defaultStyles->clear ();
  m_defaultStyleLists.clear ();
}

void KateSchemaConfigFontColorTab::apply ()
{
  for ( QIntDictIterator<KateAttributeList> it( m_defaultStyleLists ); it.current(); ++it )
    HlManager::self()->setDefaults(it.currentKey(), *(it.current()));
}

//END FontColorConfig

//BEGIN FontColorConfig
KateSchemaConfigHighlightTab::KateSchemaConfigHighlightTab( QWidget *parent, const char * )
  : QWidget (parent)
{
  m_schema = 0;
  m_hl = 0;
  
  m_hlDict.setAutoDelete (true);
  
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBox *hbHl = new QHBox( this );
  layout->add (hbHl);
  
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), hbHl );
  hlCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );
           
  for( int i = 0; i < HlManager::self()->highlights(); i++) {
    if (HlManager::self()->hlSection(i).length() > 0)
      hlCombo->insertItem(HlManager::self()->hlSection(i) + QString ("/") + HlManager::self()->hlName(i));
    else
      hlCombo->insertItem(HlManager::self()->hlName(i));
  }
  hlCombo->setCurrentItem(0);

  // styles listview
  m_styles = new StyleListView( this, true );
  layout->addWidget (m_styles, 999);

  hlCombo->setCurrentItem ( 0 );
  hlChanged ( 0 );

  QWhatsThis::add( m_styles,  i18n("This list displays the contexts of the current syntax highlight mode and offers the means to edit them. The context name reflects the current style settings.<p>To edit using the keyboard, press <strong>&lt;SPACE&gt;</strong> and choose a property from the popup menu.<p>To edit the colors, click the colored squares, or select the color to edit from the popup menu.") );
  
  connect (m_styles, SIGNAL (changed()), parent->parentWidget(), SLOT (slotChanged()));
}

KateSchemaConfigHighlightTab::~KateSchemaConfigHighlightTab()
{
}

void KateSchemaConfigHighlightTab::hlChanged(int z)
{
  m_hl = z;
  
  schemaChanged (m_schema);
}

void KateSchemaConfigHighlightTab::schemaChanged (uint schema)
{
  m_schema = schema;
  
  kdDebug () << "NEW SCHEMA: " << m_schema << " NEW HL: " << m_hl << endl;

  QListViewItemIterator it ( m_styles );
  while( it.current() )
  {
    ((StyleListItem*)it.current())->updateStyle();

    ++it;
  }
  
  m_styles->clear ();

  if (!m_hlDict[m_schema])
  {
    m_hlDict.insert (schema, new QIntDict<ItemDataList>);
    m_hlDict[m_schema]->setAutoDelete (true);
  }
  
  if (!m_hlDict[m_schema]->find(m_hl))
  {
    ItemDataList *list = new ItemDataList ();
    HlManager::self()->getHl( m_hl )->getItemDataList (m_schema, *list);
    m_hlDict[m_schema]->insert (m_hl, list);
  }

  KateAttributeList list;
  HlManager::self()->getDefaults(m_schema, list);
  
  for ( ItemData *itemData = m_hlDict[m_schema]->find(m_hl)->first();
        itemData != 0L;
        itemData = m_hlDict[m_schema]->find(m_hl)->next())
  {
    m_styles->insertItem( new StyleListItem( m_styles, i18n(itemData->name.latin1()),
                          list.at(itemData->defStyleNum), itemData ) );
  }
}

void KateSchemaConfigHighlightTab::reload ()
{
  m_styles->clear ();
  m_hlDict.clear ();
  
  hlChanged (0);
}

void KateSchemaConfigHighlightTab::apply ()
{
  QListViewItemIterator it ( m_styles );
  while( it.current() )
  {
    ((StyleListItem*)it.current())->updateStyle();

    ++it;
  }

  for ( QIntDictIterator< QIntDict<ItemDataList> > it( m_hlDict ); it.current(); ++it )
    for ( QIntDictIterator< ItemDataList > it2( *it.current() ); it2.current(); ++it2 )
       HlManager::self()->getHl( it2.currentKey() )->setItemDataList (it.currentKey(), *(it2.current()));
}

//END HighlightConfig

KateSchemaConfigPage::KateSchemaConfigPage( QWidget *parent )
  : Kate::ConfigPage( parent ),
    m_lastSchema (-1)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBox *hbHl = new QHBox( this );
  layout->add (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("&Schema:"), hbHl );
  schemaCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( schemaCombo );
  connect( schemaCombo, SIGNAL(activated(int)),
           this, SLOT(schemaChanged(int)) );

  btndel = new QPushButton( i18n("&Delete"), hbHl );
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteSchema()) );

  QPushButton *btnnew = new QPushButton( i18n("&New..."), hbHl );
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newSchema()) );

  m_tabWidget = new QTabWidget ( this );
  m_tabWidget->setMargin (KDialog::marginHint());
  layout->add (m_tabWidget);

  m_colorTab = new KateSchemaConfigColorTab (m_tabWidget);
  m_tabWidget->addTab (m_colorTab, i18n("Colors"));
  
  m_fontTab = new KateSchemaConfigFontTab (m_tabWidget);
  m_tabWidget->addTab (m_fontTab, i18n("Font"));
  
  m_fontColorTab = new KateSchemaConfigFontColorTab (m_tabWidget);
  m_tabWidget->addTab (m_fontColorTab, i18n("Normal Text Styles"));
 
  m_highlightTab = new KateSchemaConfigHighlightTab (m_tabWidget);
  m_tabWidget->addTab (m_highlightTab, i18n("Highlighting Text Styles"));

  reload();
}

KateSchemaConfigPage::~KateSchemaConfigPage ()
{
  // just reload config from disc
  KateFactory::self()->schemaManager()->update ();
}

void KateSchemaConfigPage::apply()
{
  if (m_lastSchema > -1)
  {
    m_colorTab->writeConfig (KateFactory::self()->schemaManager()->schema(m_lastSchema));
    m_fontTab->writeConfig (KateFactory::self()->schemaManager()->schema(m_lastSchema));
  }

  // just sync the config
  KateFactory::self()->schemaManager()->schema (0)->sync();
  KateFactory::self()->schemaManager()->update ();

  KateRendererConfig::global()->setSchema (KateRendererConfig::global()->schema());
  
  // special for the highlighting stuff
  m_fontColorTab->apply ();
  m_highlightTab->apply ();
  
  // sync the hl config for real
  HlManager::self()->getKConfig()->sync ();
}

void KateSchemaConfigPage::reload()
{
  // just reload the config from disc
  KateFactory::self()->schemaManager()->update ();

  // special for the highlighting stuff
  m_fontColorTab->reload ();
  
  update ();
}

void KateSchemaConfigPage::reset()
{
  reload ();
}

void KateSchemaConfigPage::defaults()
{
  reload ();
}

void KateSchemaConfigPage::update ()
{
  // soft update, no load from disk
  KateFactory::self()->schemaManager()->update (false);

  schemaCombo->clear ();
  schemaCombo->insertStringList (KateFactory::self()->schemaManager()->list ());

  schemaCombo->setCurrentItem (0);
  schemaChanged (0);

  schemaCombo->setEnabled (schemaCombo->count() > 0);
}

void KateSchemaConfigPage::deleteSchema ()
{
  int t = schemaCombo->currentItem ();

  KateFactory::self()->schemaManager()->removeSchema (t);

  update ();
}

void KateSchemaConfigPage::newSchema ()
{
  QString t = KLineEditDlg::getText (i18n("Name for New Schema"), i18n ("Name:"), i18n("New Schema"), 0, this);

  KateFactory::self()->schemaManager()->addSchema (t);

  // soft update, no load from disk
  KateFactory::self()->schemaManager()->update (false);
  int i = KateFactory::self()->schemaManager()->list ().findIndex (t);

  update ();
  if (i > -1)
  {
    schemaCombo->setCurrentItem (i);
    schemaChanged (i);
  }
}

void KateSchemaConfigPage::schemaChanged (int schema)
{
  if (schema < 2)
  {
    btndel->setEnabled (false);
  }
  else
  {
    btndel->setEnabled (true);
  }

  if (m_lastSchema > -1)
  {
    m_colorTab->writeConfig (KateFactory::self()->schemaManager()->schema(m_lastSchema));
    m_fontTab->writeConfig (KateFactory::self()->schemaManager()->schema(m_lastSchema));
  }

  m_colorTab->readConfig (KateFactory::self()->schemaManager()->schema(schema));
  m_fontTab->readConfig (KateFactory::self()->schemaManager()->schema(schema));
  m_fontColorTab->schemaChanged (schema);
  m_highlightTab->schemaChanged (schema);

  m_lastSchema = schema;
}


void KateViewSchemaAction::init()
{
  m_view = 0;
  last = 0;

  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewSchemaAction::updateMenu (KateView *view)
{
  m_view = view;
}

void KateViewSchemaAction::slotAboutToShow()
{
  KateView *view=m_view;
  int count = KateFactory::self()->schemaManager()->list().count();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateFactory::self()->schemaManager()->list().operator[](z);

    if (names.contains(hlName) < 1)
    {
      names << hlName;
      popupMenu()->insertItem ( hlName, this, SLOT(setSchema(int)), 0,  z+1);
    }
  }

  if (!view) return;

  popupMenu()->setItemChecked (last, false);
  popupMenu()->setItemChecked (view->renderer()->config()->schema()+1, true);

  last = view->renderer()->config()->schema()+1;
}

void KateViewSchemaAction::setSchema (int mode)
{
  KateView *view=m_view;

  if (view)
    view->renderer()->config()->setSchema (mode-1);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
