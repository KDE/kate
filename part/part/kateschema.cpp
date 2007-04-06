/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>

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
#include "kateschema.h"
#include "kateschema.moc"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include "katerenderer.h"
#include "kateextendedattribute.h"
#include "katestyletreewidget.h"

#include "ui_schemaconfigcolortab.h"

#include <klocale.h>
#include <kdialog.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kinputdialog.h>
#include <kfontdialog.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kcolordialog.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <ktexteditor/markinterface.h>
#include <khbox.h>

#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QLabel>
#include <QtCore/QTextCodec>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtCore/QObject>
#include <QtGui/QPixmap>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QDoubleSpinBox>
#include <QtCore/QStringList>
#include <QtGui/QTabWidget>
#include <QPolygon>
#include <QGroupBox>
#include <QTreeWidgetItem>
#include <QItemDelegate>
//END


//BEGIN KateSchemaManager
QString KateSchemaManager::normalSchema ()
{
  return KGlobal::mainComponent().aboutData()->appName () + QString (" - Normal");
}

QString KateSchemaManager::printingSchema ()
{
  return KGlobal::mainComponent().aboutData()->appName () + QString (" - Printing");
}

KateSchemaManager::KateSchemaManager ()
    : m_config ("kateschemarc", KConfig::NoGlobals)
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

  m_schemas.removeAll (printingSchema());
  m_schemas.removeAll (normalSchema());
  m_schemas.prepend (printingSchema());
  m_schemas.prepend (normalSchema());
}

//
// get the right group
// special handling of the default schemas ;)
//
KConfigGroup KateSchemaManager::schema (uint number)
{
  if ((number>1) && (number < (uint)m_schemas.count()))
    return m_config.group (m_schemas[number]);
  else if (number == 1)
    return m_config.group (printingSchema());
  else
    return m_config.group (normalSchema());
}

void KateSchemaManager::addSchema (const QString &t)
{
  m_config.group(t).writeEntry("Color Background", KGlobalSettings::baseColor());

  update (false);
}

void KateSchemaManager::removeSchema (uint number)
{
  if (number >= (uint)m_schemas.count())
    return;

  if (number < 2)
    return;

  m_config.deleteGroup (name (number));

  update (false);
}

bool KateSchemaManager::validSchema (uint number)
{
  if (number < (uint)m_schemas.count())
    return true;

  return false;
}

uint KateSchemaManager::number (const QString &name)
{
  if (name == normalSchema())
    return 0;

  if (name == printingSchema())
    return 1;

  int i;
  if ((i = m_schemas.indexOf(name)) > -1)
    return i;

  return 0;
}

QString KateSchemaManager::name (uint number)
{
  if ((number>1) && (number < (uint)m_schemas.count()))
    return m_schemas[number];
  else if (number == 1)
    return printingSchema();

  return normalSchema();
}
//END

//
// DIALOGS !!!
//

//BEGIN KateSchemaConfigColorTab -- 'Colors' tab
KateSchemaConfigColorTab::KateSchemaConfigColorTab()
  : ui(new Ui::SchemaConfigColorTab())
{
  m_schema = -1;

  ui->setupUi(this);

  // Markers from kdelibs/interfaces/ktextinterface/markinterface.h
  // add the predefined mark types as defined in markinterface.h
  ui->combobox->addItem(i18n("Bookmark"));            // markType01
  ui->combobox->addItem(i18n("Active Breakpoint"));   // markType02
  ui->combobox->addItem(i18n("Reached Breakpoint"));  // markType03
  ui->combobox->addItem(i18n("Disabled Breakpoint")); // markType04
  ui->combobox->addItem(i18n("Execution"));           // markType05
  ui->combobox->addItem(i18n("Warning"));             // markType06
  ui->combobox->addItem(i18n("Error"));               // markType07
  ui->combobox->addItem(i18n("Template Background"));
  ui->combobox->addItem(i18n("Template Editable Placeholder"));
  ui->combobox->addItem(i18n("Template Focused Editable Placeholder"));
  ui->combobox->addItem(i18n("Template Not Editable Placeholder"));
  ui->combobox->setCurrentIndex(0);

  connect( ui->combobox  , SIGNAL( activated( int ) )        , SLOT( slotComboBoxChanged( int ) ) );
  connect( ui->back      , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->selected  , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->current   , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->bracket   , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->wwmarker  , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->iconborder, SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->tmarker   , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->linenumber, SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( ui->markers   , SIGNAL( changed( const QColor& ) ), SLOT( slotMarkerColorChanged( const QColor& ) ) );
}

KateSchemaConfigColorTab::~KateSchemaConfigColorTab()
{
  delete ui;
}

void KateSchemaConfigColorTab::schemaChanged ( int newSchema )
{
  // save curent schema
  if ( m_schema > -1 )
  {
    m_schemas[ m_schema ].back = ui->back->color();
    m_schemas[ m_schema ].selected = ui->selected->color();
    m_schemas[ m_schema ].current = ui->current->color();
    m_schemas[ m_schema ].bracket = ui->bracket->color();
    m_schemas[ m_schema ].wwmarker = ui->wwmarker->color();
    m_schemas[ m_schema ].iconborder = ui->iconborder->color();
    m_schemas[ m_schema ].tmarker = ui->tmarker->color();
    m_schemas[ m_schema ].linenumber = ui->linenumber->color();
  }

  if ( newSchema == m_schema ) return;

  // switch
  m_schema = newSchema;

  // first block signals otherwise setColor emits changed
  bool blocked = blockSignals(true);

  // If we havent this schema, read in from config file
  if ( ! m_schemas.contains( newSchema ) )
  {
    // fallback defaults
    QColor tmp0 (KGlobalSettings::baseColor());
    QColor tmp1 (KGlobalSettings::highlightColor());
    QColor tmp2 (KGlobalSettings::alternateBackgroundColor());
    QColor tmp3 ( "#FFFF99" );
    QColor tmp4 (tmp2.dark());
    QColor tmp5 ( "#EAE9E8" );
    QColor tmp6 ( "#EAE9E8" );
    QColor tmp7 ( "#000000" );

    // same std colors like in KateDocument::markColor
    QVector <QColor> mark(KTextEditor::MarkInterface::reservedMarkersCount());
    Q_ASSERT(mark.size() > 6);
    mark[0] = Qt::blue;
    mark[1] = Qt::red;
    mark[2] = Qt::yellow;
    mark[3] = Qt::magenta;
    mark[4] = Qt::gray;
    mark[5] = Qt::green;
    mark[6] = Qt::red;

    SchemaColors c;
    KConfigGroup config = KateGlobal::self()->schemaManager()->schema(newSchema);

    c.back= config.readEntry("Color Background", tmp0);
    c.selected = config.readEntry("Color Selection", tmp1);
    c.current = config.readEntry("Color Highlighted Line", tmp2);
    c.bracket = config.readEntry("Color Highlighted Bracket", tmp3);
    c.wwmarker = config.readEntry("Color Word Wrap Marker", tmp4);
    c.tmarker = config.readEntry("Color Tab Marker", tmp5);
    c.iconborder = config.readEntry("Color Icon Bar", tmp6);
    c.linenumber = config.readEntry("Color Line Number", tmp7);

    for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
      c.markerColors[i] =  config.readEntry( QString("Color MarkType%1").arg(i+1), mark[i] );

    c.templateColors[0] = config.readEntry(QString("Color Template Background"),QColor(0xcc,0xcc,0xcc));
    c.templateColors[1] = config.readEntry(QString("Color Template Editable Placeholder"),QColor(0xcc,0xff,0xcc));
    c.templateColors[2] = config.readEntry(QString("Color Template Focused Editable Placeholder"),QColor(0x66,0xff,0x66));
    c.templateColors[3] = config.readEntry(QString("Color Template Not Editable Placeholder"),QColor(0xff,0xcc,0xcc));

    m_schemas[ newSchema ] = c;
  }

  ui->back->setColor(  m_schemas[ newSchema ].back);
  ui->selected->setColor(  m_schemas [ newSchema ].selected );
  ui->current->setColor(  m_schemas [ newSchema ].current );
  ui->bracket->setColor(  m_schemas [ newSchema ].bracket );
  ui->wwmarker->setColor(  m_schemas [ newSchema ].wwmarker );
  ui->tmarker->setColor(  m_schemas [ newSchema ].tmarker );
  ui->iconborder->setColor(  m_schemas [ newSchema ].iconborder );
  ui->linenumber->setColor(  m_schemas [ newSchema ].linenumber );

  // map from 0..reservedMarkersCount()-1 - the same index as in markInterface
  for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
  {
    QPixmap pix(16, 16);
    pix.fill( m_schemas [ newSchema ].markerColors[i]);
    ui->combobox->setItemIcon(i, QIcon(pix));
  }
  for (int i = 0; i < 4; i++)
  {
    QPixmap pix(16, 16);
    pix.fill( m_schemas [ newSchema ].templateColors[i]);
    ui->combobox->setItemIcon(i+KTextEditor::MarkInterface::reservedMarkersCount(), QIcon(pix));
  }

  ui->markers->setColor(  m_schemas [ newSchema ].markerColors[ ui->combobox->currentIndex() ] );

  blockSignals(blocked);
}

void KateSchemaConfigColorTab::apply ()
{
  schemaChanged( m_schema );
  QMap<int,SchemaColors>::Iterator it;
  for ( it =  m_schemas.begin(); it !=  m_schemas.end(); ++it )
  {
    kDebug(13030)<<"APPLY scheme = "<<it.key()<<endl;
    KConfigGroup config = KateGlobal::self()->schemaManager()->schema( it.key() );
    kDebug(13030)<<"Using config group "<<config.group()<<endl;
    SchemaColors c = it.value();

    config.writeEntry("Color Background", c.back);
    config.writeEntry("Color Selection", c.selected);
    config.writeEntry("Color Highlighted Line", c.current);
    config.writeEntry("Color Highlighted Bracket", c.bracket);
    config.writeEntry("Color Word Wrap Marker", c.wwmarker);
    config.writeEntry("Color Tab Marker", c.tmarker);
    config.writeEntry("Color Icon Bar", c.iconborder);
    config.writeEntry("Color Line Number", c.linenumber);

    for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
    {
      config.writeEntry(QString("Color MarkType%1").arg(i + 1), c.markerColors[i]);
    }

    config.writeEntry(QString("Color Template Background"),c.templateColors[0]);
    config.writeEntry(QString("Color Template Editable Placeholder"),c.templateColors[1]);
    config.writeEntry(QString("Color Template Focused Editable Placeholder"),c.templateColors[2]);
    config.writeEntry(QString("Color Template Not Editable Placeholder"),c.templateColors[3]);

  }
}

void KateSchemaConfigColorTab::slotMarkerColorChanged( const QColor& color)
{
  int index = ui->combobox->currentIndex();
  if (index<7)
   m_schemas[ m_schema ].markerColors[ index ] = color;
  else
   m_schemas[m_schema].templateColors[index-7] = color;
  QPixmap pix(16, 16);
  pix.fill(color);
  ui->combobox->setItemIcon(index, QIcon(pix));

  emit changed();
}

void KateSchemaConfigColorTab::slotComboBoxChanged(int index)
{
  // temporarily block signals because setColor emits changed as well
  bool blocked = ui->markers->blockSignals(true);
  if (index<7)
    ui->markers->setColor( m_schemas[m_schema].markerColors[index] );
  else
    ui->markers->setColor( m_schemas[m_schema].templateColors[index-7] );
  ui->markers->blockSignals(blocked);
}

//END KateSchemaConfigColorTab

//BEGIN FontConfig -- 'Fonts' tab
KateSchemaConfigFontTab::KateSchemaConfigFontTab()
{
    // sizemanagment
  QGridLayout *grid = new QGridLayout( this );

  m_fontchooser = new KFontChooser ( this, false, QStringList(), false );
  m_fontchooser->enableColumn(KFontChooser::StyleList, false);
  grid->addWidget( m_fontchooser, 0, 0);

  m_schema = -1;
}

KateSchemaConfigFontTab::~KateSchemaConfigFontTab()
{
}

void KateSchemaConfigFontTab::slotFontSelected( const QFont &font )
{
  if ( m_schema > -1 )
  {
    m_fonts[m_schema] = font;
    emit changed();
  }
}

void KateSchemaConfigFontTab::apply()
{
  FontMap::Iterator it;
  for ( it = m_fonts.begin(); it != m_fonts.end(); ++it )
  {
    KateGlobal::self()->schemaManager()->schema( it.key() ).writeEntry( "Font", it.value() );
  }
}

void KateSchemaConfigFontTab::schemaChanged( int newSchema )
{
  if ( m_schema > -1 )
    m_fonts[ m_schema ] = m_fontchooser->font();

  m_schema = newSchema;

  QFont f (KGlobalSettings::fixedFont());

  m_fontchooser->disconnect ( this );
  m_fontchooser->setFont ( KateGlobal::self()->schemaManager()->schema( newSchema ).readEntry("Font", f) );
  m_fonts[ newSchema ] = m_fontchooser->font();
  connect (m_fontchooser, SIGNAL (fontSelected( const QFont & )), this, SLOT (slotFontSelected( const QFont & )));
}
//END FontConfig

//BEGIN FontColorConfig -- 'Normal Text Styles' tab
KateSchemaConfigFontColorTab::KateSchemaConfigFontColorTab()
{
  // sizemanagment
  QGridLayout *grid = new QGridLayout( this );

  m_defaultStyles = new KateStyleTreeWidget( this );
  m_defaultStyles->setRootIsDecorated(false);
  connect(m_defaultStyles, SIGNAL(changed()), this, SIGNAL(changed()));
  grid->addWidget( m_defaultStyles, 0, 0);

  m_defaultStyles->setWhatsThis(i18n(
      "This list displays the default styles for the current schema and "
      "offers the means to edit them. The style name reflects the current "
      "style settings."
      "<p>To edit the colors, click the colored squares, or select the color "
      "to edit from the popup menu.<p>You can unset the Background and Selected "
      "Background colors from the popup menu when appropriate.") );
}

KateSchemaConfigFontColorTab::~KateSchemaConfigFontColorTab()
{
  qDeleteAll(m_defaultStyleLists);
}

KateAttributeList *KateSchemaConfigFontColorTab::attributeList (uint schema)
{
  if (!m_defaultStyleLists.contains(schema))
  {
    KateAttributeList *list = new KateAttributeList ();
    KateHlManager::self()->getDefaults(schema, *list);

    m_defaultStyleLists.insert (schema, list);
  }

  return m_defaultStyleLists[schema];
}

void KateSchemaConfigFontColorTab::schemaChanged (uint schema)
{
  m_defaultStyles->clear ();

  KateAttributeList *l = attributeList (schema);

  // set colors
  QPalette p ( m_defaultStyles->palette() );
  QColor _c ( KGlobalSettings::baseColor() );
  p.setColor( QPalette::Base,
    KateGlobal::self()->schemaManager()->schema(schema).
      readEntry( "Color Background", _c ) );
  _c = KGlobalSettings::highlightColor();
  p.setColor( QPalette::Highlight,
    KateGlobal::self()->schemaManager()->schema(schema).
      readEntry( "Color Selection", _c ) );
  _c = l->at(0)->foreground().color(); // not quite as much of an assumption ;)
  p.setColor( QPalette::Text, _c );
  m_defaultStyles->viewport()->setPalette( p );

  // insert the default styles backwards to get them in the right order
  for ( int i = KateHlManager::self()->defaultStyles() - 1; i >= 0; i-- )
  {
    m_defaultStyles->addItem( KateHlManager::self()->defaultStyleName(i, true), l->at( i ) );
  }
}

void KateSchemaConfigFontColorTab::reload ()
{
  m_defaultStyles->clear ();
  qDeleteAll(m_defaultStyleLists);
  m_defaultStyleLists.clear ();
}

void KateSchemaConfigFontColorTab::apply ()
{
  QHashIterator<int,KateAttributeList*> it = m_defaultStyleLists;
  while (it.hasNext()) {
    it.next();
    KateHlManager::self()->setDefaults(it.key(), *it.value());
  }
}

//END FontColorConfig

//BEGIN KateSchemaConfigHighlightTab -- 'Highlighting Text Styles' tab
KateSchemaConfigHighlightTab::KateSchemaConfigHighlightTab(KateSchemaConfigFontColorTab *page, uint hl )
{
  m_defaults = page;

  m_schema = 0;
  m_hl = 0;

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint());

  // hl chooser
  KHBox *hbHl = new KHBox( this );
  layout->addWidget (hbHl);

  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), hbHl );
  hlCombo = new QComboBox( hbHl );
  hlCombo->setEditable( false );
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );

  for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (KateHlManager::self()->hlSection(i).length() > 0)
      hlCombo->addItem(KateHlManager::self()->hlSection(i) + QString ("/") + KateHlManager::self()->hlNameTranslated(i));
    else
      hlCombo->addItem(KateHlManager::self()->hlNameTranslated(i));
  }
  hlCombo->setCurrentIndex(0);

  // styles listview
  m_styles = new KateStyleTreeWidget( this, true );
  connect(m_styles, SIGNAL(changed()), this, SIGNAL(changed()));
  layout->addWidget (m_styles, 999);

  hlCombo->setCurrentIndex ( hl );
  hlChanged ( hl );

  m_styles->setWhatsThis(i18n(
    "This list displays the contexts of the current syntax highlight mode and "
    "offers the means to edit them. The context name reflects the current "
    "style settings.<p>To edit using the keyboard, press "
    "<strong>&lt;SPACE&gt;</strong> and choose a property from the popup menu."
    "<p>To edit the colors, click the colored squares, or select the color "
    "to edit from the popup menu.<p>You can unset the Background and Selected "
    "Background colors from the context menu when appropriate.") );
}

KateSchemaConfigHighlightTab::~KateSchemaConfigHighlightTab()
{
}

void KateSchemaConfigHighlightTab::hlChanged(int z)
{
  m_hl = z;

  schemaChanged (m_schema);
}

void KateSchemaConfigHighlightTab::schemaChanged (int schema)
{
  m_schema = schema;

  kDebug(13030) << "NEW SCHEMA: " << m_schema << " NEW HL: " << m_hl << endl;

  m_styles->clear ();

  if (!m_hlDict.contains(m_schema))
  {
    kDebug(13030) << "NEW SCHEMA, create dict" << endl;

    m_hlDict.insert (schema, QHash<int, QList<KateExtendedAttribute::Ptr> >());
  }

  if (!m_hlDict[m_schema].contains(m_hl))
  {
    kDebug(13030) << "NEW HL, create list" << endl;

    QList<KateExtendedAttribute::Ptr> list;
    KateHlManager::self()->getHl( m_hl )->getKateExtendedAttributeListCopy(m_schema, list);
    m_hlDict[m_schema].insert (m_hl, list);
  }

  KateAttributeList *l = m_defaults->attributeList (schema);

  // Set listview colors
  // We do that now, because we can now get the "normal text" color.
  // TODO this reads of the KConfig object, which should be changed when
  // the color tab is fixed.
  QPalette p ( m_styles->palette() );
  QColor _c ( KGlobalSettings::baseColor() );
  p.setColor( QPalette::Base,
    KateGlobal::self()->schemaManager()->schema(m_schema).
      readEntry( "Color Background", _c ) );
  _c = KGlobalSettings::highlightColor();
  p.setColor( QPalette::Highlight,
    KateGlobal::self()->schemaManager()->schema(m_schema).
      readEntry( "Color Selection", _c ) );
  _c = l->at(0)->foreground().color(); // not quite as much of an assumption ;)
  p.setColor( QPalette::Text, _c );
  m_styles->viewport()->setPalette( p );

  QHash<QString, QTreeWidgetItem*> prefixes;
  QList<KateExtendedAttribute::Ptr>::ConstIterator it = m_hlDict[m_schema][m_hl].end();
  while (it != m_hlDict[m_schema][m_hl].begin())
  {
    --it;
    KateExtendedAttribute::Ptr itemData = *it;
    Q_ASSERT(itemData);

    kDebug(13030) << "insert items " << itemData->name() << endl;

    // All stylenames have their language mode prefixed, e.g. HTML:Comment
    // split them and put them into nice substructures.
    int c = itemData->name().indexOf(':');
    if ( c > 0 ) {
      QString prefix = itemData->name().left(c);
      QString name   = itemData->name().mid(c+1);

      QTreeWidgetItem *parent = prefixes[prefix];
      if ( ! parent )
      {
        parent = new QTreeWidgetItem( m_styles, QStringList() << prefix );
        m_styles->expandItem(parent);
        prefixes.insert( prefix, parent );
      }
      m_styles->addItem( parent, name, l->at(itemData->defaultStyleIndex()), itemData );
    } else {
      m_styles->addItem( itemData->name(), l->at(itemData->defaultStyleIndex()), itemData );
    }
  }

  m_styles->resizeColumns();
}

void KateSchemaConfigHighlightTab::reload ()
{
  m_styles->clear ();

  m_hlDict.clear ();

  hlChanged (0);
}

void KateSchemaConfigHighlightTab::apply ()
{
  QMutableHashIterator<int, QHash<int, QList<KateExtendedAttribute::Ptr> > > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    QMutableHashIterator<int, QList<KateExtendedAttribute::Ptr> > it2 = it.value();
    while (it2.hasNext()) {
      it2.next();
      KateHlManager::self()->getHl( it2.key() )->setKateExtendedAttributeList (it.key(), it2.value());
    }
  }
}

//END KateSchemaConfigHighlightTab

//BEGIN KateSchemaConfigPage -- Main dialog page
KateSchemaConfigPage::KateSchemaConfigPage( QWidget *parent, KateDocument *doc )
  : KateConfigPage( parent ),
    m_lastSchema (-1)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint());

  KHBox *hbHl = new KHBox( this );
  layout->addWidget(hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("&Schema:"), hbHl );
  schemaCombo = new QComboBox( hbHl );
  schemaCombo->setEditable( false );
  lHl->setBuddy( schemaCombo );
  connect( schemaCombo, SIGNAL(activated(int)),
           this, SLOT(schemaChanged(int)) );

  QPushButton *btnnew = new QPushButton( i18n("&New..."), hbHl );
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newSchema()) );

  btndel = new QPushButton( i18n("&Delete"), hbHl );
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteSchema()) );

  m_tabWidget = new QTabWidget ( this );
  layout->addWidget (m_tabWidget);

  connect (m_tabWidget, SIGNAL (currentChanged (QWidget *)), this, SLOT (newCurrentPage (QWidget *)));

  m_colorTab = new KateSchemaConfigColorTab();
  m_tabWidget->addTab (m_colorTab, i18n("Colors"));
  connect(m_colorTab, SIGNAL(changed()), SLOT(slotChanged()));

  m_fontTab = new KateSchemaConfigFontTab();
  m_tabWidget->addTab (m_fontTab, i18n("Font"));
  connect(m_fontTab, SIGNAL(changed()), SLOT(slotChanged()));

  m_fontColorTab = new KateSchemaConfigFontColorTab();
  m_tabWidget->addTab (m_fontColorTab, i18n("Normal Text Styles"));
  connect(m_fontColorTab, SIGNAL(changed()), SLOT(slotChanged()));

  uint hl = doc ? doc->hlMode() : 0;
  m_highlightTab = new KateSchemaConfigHighlightTab(m_fontColorTab, hl );
  m_tabWidget->addTab(m_highlightTab, i18n("Highlighting Text Styles"));
  connect(m_highlightTab, SIGNAL(changed()), SLOT(slotChanged()));

  hbHl = new KHBox( this );
  layout->addWidget (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  lHl = new QLabel( i18n("&Default schema for %1:", KGlobal::mainComponent().aboutData()->programName ()), hbHl );
  defaultSchemaCombo = new QComboBox( hbHl );
  defaultSchemaCombo->setEditable( false );
  lHl->setBuddy( defaultSchemaCombo );


  m_defaultSchema = (doc && doc->activeView()) ? doc->activeKateView()->renderer()->config()->schema() : KateRendererConfig::global()->schema();

  reload();

  connect( defaultSchemaCombo, SIGNAL(activated(int)),
           this, SLOT(slotChanged()) );
}

KateSchemaConfigPage::~KateSchemaConfigPage ()
{
  // just reload config from disc
  KateGlobal::self()->schemaManager()->update ();
}

void KateSchemaConfigPage::apply()
{
  m_colorTab->apply();
  m_fontTab->apply();
  m_fontColorTab->apply ();
  m_highlightTab->apply ();

  // just sync the config
  KateGlobal::self()->schemaManager()->schema (0).sync();

  KateGlobal::self()->schemaManager()->update ();

  // clear all attributes
  for (int i = 0; i < KateHlManager::self()->highlights(); ++i)
    KateHlManager::self()->getHl (i)->clearAttributeArrays();

  // than reload the whole stuff
  KateRendererConfig::global()->setSchema (defaultSchemaCombo->currentIndex());
  KateRendererConfig::global()->reloadSchema();

  // sync the hl config for real
  KateHlManager::self()->getKConfig()->sync ();
}

void KateSchemaConfigPage::reload()
{
  // just reload the config from disc
  KateGlobal::self()->schemaManager()->update ();

  // special for the highlighting stuff
  m_fontColorTab->reload ();

  update ();

  defaultSchemaCombo->setCurrentIndex (KateRendererConfig::global()->schema());

  // initialize to the schema in the current document, or default schema
  schemaCombo->setCurrentIndex( m_defaultSchema );
  schemaChanged( m_defaultSchema );
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
  KateGlobal::self()->schemaManager()->update (false);

  schemaCombo->clear ();
  schemaCombo->addItems (KateGlobal::self()->schemaManager()->list ());

  defaultSchemaCombo->clear ();
  defaultSchemaCombo->addItems (KateGlobal::self()->schemaManager()->list ());

  schemaCombo->setCurrentIndex (0);
  schemaChanged (0);

  schemaCombo->setEnabled (schemaCombo->count() > 0);
}

void KateSchemaConfigPage::deleteSchema ()
{
  int t = schemaCombo->currentIndex ();

  KateGlobal::self()->schemaManager()->removeSchema (t);

  update ();
}

void KateSchemaConfigPage::newSchema ()
{
  QString t = KInputDialog::getText (i18n("Name for New Schema"), i18n ("Name:"), i18n("New Schema"), 0, this);

  KateGlobal::self()->schemaManager()->addSchema (t);

  // soft update, no load from disk
  KateGlobal::self()->schemaManager()->update (false);
  int i = KateGlobal::self()->schemaManager()->list ().indexOf (t);

  update ();
  if (i > -1)
  {
    schemaCombo->setCurrentIndex (i);
    schemaChanged (i);
  }
}

void KateSchemaConfigPage::schemaChanged (int schema)
{
  btndel->setEnabled( schema > 1 );

  m_colorTab->schemaChanged( schema );
  m_fontTab->schemaChanged( schema );
  m_fontColorTab->schemaChanged (schema);
  m_highlightTab->schemaChanged (schema);

  m_lastSchema = schema;
}

void KateSchemaConfigPage::newCurrentPage (QWidget *w)
{
  if (w == m_highlightTab)
    m_highlightTab->schemaChanged (m_lastSchema);
}
//END KateSchemaConfigPage

//BEGIN SCHEMA ACTION -- the 'View->Schema' menu action
void KateViewSchemaAction::init()
{
  m_group=0;
  m_view = 0;
  last = 0;

  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewSchemaAction::updateMenu (KateView *view)
{
  m_view = view;
}

void KateViewSchemaAction::slotAboutToShow()
{
  KateView *view=m_view;
  int count = KateGlobal::self()->schemaManager()->list().count();

  if (!m_group) {
   m_group=new QActionGroup(menu());
   m_group->setExclusive(true);

  }

  for (int z=0; z<count; z++)
  {
    QString hlName = KateGlobal::self()->schemaManager()->list().operator[](z);

    if (!names.contains(hlName))
    {
      names << hlName;
      QAction *a=menu()->addAction ( hlName, this, SLOT(setSchema()));
      a->setData(z+1);
      a->setCheckable(true);
      a->setActionGroup(m_group);
	//FIXME EXCLUSIVE
    }
  }

  if (!view) return;

  int id=view->renderer()->config()->schema()+1;
  if (menu()->actions().at(id-1)->data().toInt()==id) {
  	menu()->actions().at(id-1)->setChecked(true);
  } else {
	foreach(QAction *a,menu()->actions()) {
		if (a->data().toInt()==id) {
			a->setChecked(true);
			break;
		}
	}
  }
//FIXME
#if 0
  popupMenu()->setItemChecked (last, false);
  popupMenu()->setItemChecked (view->renderer()->config()->schema()+1, true);

  last = view->renderer()->config()->schema()+1;
#endif
}

void KateViewSchemaAction::setSchema () {
  QAction *action = qobject_cast<QAction*>(sender());

  if (!action) return;
  int mode=action->data().toInt();

  KateView *view=m_view;

  if (view)
    view->renderer()->config()->setSchema (mode-1);
}
//END SCHEMA ACTION

// kate: space-indent on; indent-width 2; replace-tabs on;
