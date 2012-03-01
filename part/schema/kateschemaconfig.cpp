/* This file is part of the KDE libraries
   Copyright (C) 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2012 Dominik Haumann <dhaumann kde org>

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
#include "kateschemaconfig.h"
#include "kateschemaconfig.moc"

#include "kateschema.h"
#include "kateconfig.h"
#include "kateglobal.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katestyletreewidget.h"
#include "katecolortreewidget.h"

#include "ui_howtoimportschema.h"

#include <kcolorscheme.h>
#include <kcolorutils.h>
#include <kinputdialog.h>
#include <kfontchooser.h>
#include <kmessagebox.h>
#include <khbox.h>
#include <ktabwidget.h>

#include <QtGui/QPushButton>
#include <QtGui/QProgressDialog>
//END


//BEGIN KateSchemaConfigColorTab -- 'Colors' tab
KateSchemaConfigColorTab::KateSchemaConfigColorTab()
{
  m_currentSchema = -1;

  QGridLayout* l = new QGridLayout(this);
  setLayout(l);

  ui = new KateColorTreeWidget(this);
  QPushButton* btnUseColorScheme = new QPushButton("Use KDE Color Scheme", this);

  l->addWidget(ui, 0, 0, 1, 2);
  l->addWidget(btnUseColorScheme, 1, 1);

  l->setColumnStretch(0, 1);
  l->setColumnStretch(1, 0);

  connect(btnUseColorScheme, SIGNAL(clicked()), ui, SLOT(selectDefaults()));
  connect(ui, SIGNAL(changed()), SIGNAL(changed()));
}

KateSchemaConfigColorTab::~KateSchemaConfigColorTab()
{
}

QVector<KateColorItem> KateSchemaConfigColorTab::colorItemList() const
{
  QVector<KateColorItem> items;

  KColorScheme schemeWindow = KColorScheme(QPalette::Active, KColorScheme::Window);
  KColorScheme schemeView = KColorScheme(QPalette::Active, KColorScheme::View);

  //
  // editor background colors
  //
  KateColorItem ci;
  ci.category = "Editor Background Colors";

  ci.name = i18n("Text Area");
  ci.key = "Color Background";
  ci.whatsThis = i18n("<p>Sets the background color of the editing area.</p>");
  ci.defaultColor = schemeView.background().color();
  items.append(ci);

  ci.name = i18n("Selected Text");
  ci.key = "Color Selection";
  ci.whatsThis = i18n("<p>Sets the background color of the selection.</p><p>To set the text color for selected text, use the &quot;<b>Configure Highlighting</b>&quot; dialog.</p>");
  ci.defaultColor = KColorScheme(QPalette::Inactive, KColorScheme::Selection).background().color();
  items.append(ci);

  ci.name = i18n("Current Line");
  ci.key = "Color Highlighted Line";
  ci.whatsThis = i18n("<p>Sets the background color of the currently active line, which means the line where your cursor is positioned.</p>");
  ci.defaultColor = schemeView.background(KColorScheme::AlternateBackground).color();
  items.append(ci);

  ci.name = i18n("Search Highlight");
  ci.key = "Color Search Highlight";
  ci.whatsThis = i18n("<p>Sets the background color of search results.</p>");
  ci.defaultColor = schemeView.background(KColorScheme::NeutralBackground).color();
  items.append(ci);

  ci.name = i18n("Replace Highlight");
  ci.key = "Color Replace Highlight";
  ci.whatsThis = i18n("<p>Sets the background color of replaced text.</p>");
  ci.defaultColor = schemeView.background(KColorScheme::PositiveBackground).color();
  items.append(ci);


  //
  // icon border
  //
  ci.category = i18n("Icon Border");

  ci.name = i18n("Background Area");
  ci.key = "Color Icon Bar";
  ci.whatsThis = i18n("<p>Sets the background color of the icon border.</p>");
  ci.defaultColor = schemeWindow.background().color();
  items.append(ci);

  ci.name = i18n("Line Numbers");
  ci.key = "Color Line Number";
  ci.whatsThis = i18n("<p>This color will be used to draw the line numbers (if enabled) and the lines in the code-folding pane.</p>");
  ci.defaultColor = schemeWindow.foreground().color();
  items.append(ci);

  ci.name = i18n("Word Wrap Marker");
  ci.key = "Color Word Wrap Marker";
  ci.whatsThis = i18n("<p>Sets the color of Word Wrap-related markers:</p><dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where text is going to be wrapped</dd><dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of visually-wrapped lines</dd></dl>");
  qreal bgLuma = KColorUtils::luma(schemeView.background().color());
  ci.defaultColor = KColorUtils::shade( schemeView.background().color(), bgLuma > 0.3 ? -0.15 : 0.03 );
  items.append(ci);

  ci.name = i18n("Modified Lines");
  ci.key = "Color Modified Lines";
  ci.whatsThis = i18n("<p>Sets the color of the line modification marker for modified lines.</p>");
  ci.defaultColor = schemeView.background(KColorScheme::NegativeBackground).color();
  items.append(ci);

  ci.name = i18n("Saved Lines");
  ci.key = "Color Saved Lines";
  ci.whatsThis = i18n("<p>Sets the color of the line modification marker for saved lines.</p>");
  ci.defaultColor = schemeView.background(KColorScheme::PositiveBackground).color();
  items.append(ci);


  //
  // text decorations
  //
  ci.category = i18n("Text Decorations");

  ci.name = i18n("Spelling Mistake Line");
  ci.key = "Color Spelling Mistake Line";
  ci.whatsThis = i18n("<p>Sets the color of the line that is used to indicate spelling mistakes.</p>");
  ci.defaultColor = schemeView.foreground(KColorScheme::NegativeText).color();
  items.append(ci);

  ci.name = i18n("Tab and Space Markers");
  ci.key = "Color Tab Marker";
  ci.whatsThis = i18n("<p>Sets the color of the tabulator marks.</p>");
  ci.defaultColor = KColorUtils::shade(schemeView.background().color(), bgLuma > 0.7 ? -0.35 : 0.3);
  items.append(ci);

  ci.name = i18n("Bracket Highlight");
  ci.key = "Color Highlighted Bracket";
  ci.whatsThis = i18n("<p>Sets the bracket matching color. This means, if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will be highlighted with this color.</p>");
  ci.defaultColor = KColorUtils::tint(schemeView.background().color(),
                                      schemeView.decoration(KColorScheme::HoverColor).color());
  items.append(ci);


  //
  // marker colors
  //
  ci.category = i18n("Marker Colors");

  ci.name = i18n("Bookmark");
  ci.key = "Color MarkType 1";
  ci.whatsThis = i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>");
  ci.defaultColor = Qt::blue; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Active Breakpoint");
  ci.key = "Color MarkType 2";
  ci.defaultColor = Qt::red; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Reached Breakpoint");
  ci.key = "Color MarkType 3";
  ci.defaultColor = Qt::yellow; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Disabled Breakpoint");
  ci.key = "Color MarkType 4";
  ci.defaultColor = Qt::magenta; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Execution");
  ci.key = "Color MarkType 5";
  ci.defaultColor = Qt::gray; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Warning");
  ci.key = "Color MarkType 6";
  ci.defaultColor = Qt::green; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Error");
  ci.key = "Color MarkType 7";
  ci.defaultColor = Qt::red; // TODO: if possible, derive from system color scheme
  items.append(ci);


  //
  // text templates
  //
  ci.category = i18n("Text Templates & Snippets");

  ci.whatsThis = QString(); // TODO: add whatsThis for text templates

  ci.name = i18n("Background");
  ci.key = "Color Template Background";
  ci.defaultColor = schemeWindow.background().color();
  items.append(ci);

  ci.name = i18n("Editable Placeholder");
  ci.key = "Color Template Editable Placeholder";
  ci.defaultColor = schemeView.background(KColorScheme::PositiveBackground).color();
  items.append(ci);

  ci.name = i18n("Focused Editable Placeholder");
  ci.key = "Color Template Focused Editable Placeholder";
  ci.defaultColor = schemeWindow.background(KColorScheme::PositiveBackground).color();
  items.append(ci);

  ci.name = i18n("Not Editable Placeholder");
  ci.key = "Color Template Not Editable Placeholder";
  ci.defaultColor = schemeView.background(KColorScheme::NegativeBackground).color();
  items.append(ci);


  //
  // finally, add all elements
  //
  return items;
}

void KateSchemaConfigColorTab::schemaChanged ( int newSchema )
{
  // save curent schema
  if ( m_currentSchema > -1 ) {
    if (m_schemas.contains(m_currentSchema)) {
      m_schemas.remove(m_currentSchema); // clear this color schema
    }

    // now add it again
    m_schemas[m_currentSchema] = ui->colorItems();
  }

  if ( newSchema == m_currentSchema ) return;

  // switch
  m_currentSchema = newSchema;

  // first block signals otherwise setColor emits changed
  bool blocked = blockSignals(true);

  // If we havent this schema, read in from config file
  if ( ! m_schemas.contains( newSchema ) )
  {
    KConfigGroup config = KateGlobal::self()->schemaManager()->schema(newSchema);

    // read from config
    QVector<KateColorItem> items = colorItemList();
    for (int i = 0; i < items.count(); ++i ) {
      KateColorItem& item(items[i]);
      item.useDefault = !config.hasKey(item.key);
      if (item.useDefault) {
        item.color = item.defaultColor;
      } else {
        item.color = config.readEntry(item.key, item.defaultColor);
        if (!item.color.isValid()) {
          config.deleteEntry(item.key);
          item.useDefault = true;
          item.color = item.defaultColor;
        }
      }
    }

    m_schemas[ newSchema ] = items;
  }

  QVector<KateColorItem> items;
  ui->clear();
  ui->addColorItems(m_schemas[m_currentSchema]);

  blockSignals(blocked);
}

void KateSchemaConfigColorTab::apply ()
{
  schemaChanged( m_currentSchema );
  QMap<int, QVector<KateColorItem> >::Iterator it;
  for ( it =  m_schemas.begin(); it !=  m_schemas.end(); ++it )
  {
    // TODO: if a schema was deleted (Delete button), this code writes to the
    //       config anyway. This might be a bug.

    KConfigGroup config = KateGlobal::self()->schemaManager()->schema( it.key() );
    kDebug(13030) << "writing 'Color' tab: scheme =" << it.key()
                  << "and config group =" << config.name();

    foreach (const KateColorItem& item, m_schemas[it.key()]) {
      if (item.useDefault) {
        config.deleteEntry(item.key);
      } else {
        config.writeEntry(item.key, item.color);
      }
    }
  }
}
//END KateSchemaConfigColorTab

//BEGIN FontConfig -- 'Fonts' tab
KateSchemaConfigFontTab::KateSchemaConfigFontTab()
{
    // sizemanagment
  QGridLayout *grid = new QGridLayout( this );

  m_fontchooser = new KFontChooser ( this, KFontChooser::NoDisplayFlags );
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
  connect (m_fontchooser, SIGNAL (fontSelected(QFont)), this, SLOT (slotFontSelected(QFont)));
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
      "<p>This list displays the default styles for the current schema and "
      "offers the means to edit them. The style name reflects the current "
      "style settings.</p>"
      "<p>To edit the colors, click the colored squares, or select the color "
      "to edit from the popup menu.</p><p>You can unset the Background and Selected "
      "Background colors from the popup menu when appropriate.</p>") );
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
    KateHlManager::self()->getDefaults(KateGlobal::self()->schemaManager()->name (schema), *list);

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
  KColorScheme s ( QPalette::Active, KColorScheme::View );
  QColor _c ( s.background().color() );
  p.setColor( QPalette::Base,
    KateGlobal::self()->schemaManager()->schema(schema).
      readEntry( "Color Background", _c ) );
  _c = KColorScheme(QPalette::Active, KColorScheme::Selection).background().color();
  p.setColor( QPalette::Highlight,
    KateGlobal::self()->schemaManager()->schema(schema).
      readEntry( "Color Selection", _c ) );
  _c = l->at(0)->foreground().color(); // not quite as much of an assumption ;)
  p.setColor( QPalette::Text, _c );
  m_defaultStyles->viewport()->setPalette( p );

  for ( uint i = 0; i < KateHlManager::self()->defaultStyles(); i++ )
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
    KateHlManager::self()->setDefaults(KateGlobal::self()->schemaManager()->name (it.key()), *it.value());
  }
}

void KateSchemaConfigFontColorTab::exportDefaults(int schema, KConfig *cfg) {
  KateHlManager::self()->setDefaults(KateGlobal::self()->schemaManager()->name (schema), *(m_defaultStyleLists[schema]),cfg);
}

void KateSchemaConfigFontColorTab::importDefaults(const QString& schemaName, int schema, KConfig *cfg) {
  KateHlManager::self()->getDefaults(schemaName, *(m_defaultStyleLists[schema]),cfg);
}

//END FontColorConfig

//BEGIN KateSchemaConfigHighlightTab -- 'Highlighting Text Styles' tab
KateSchemaConfigHighlightTab::KateSchemaConfigHighlightTab(KateSchemaConfigFontColorTab *page)
{
  m_defaults = page;

  m_schema = 0;
  m_hl = 0;

  QVBoxLayout *layout = new QVBoxLayout(this);

  // hl chooser
  KHBox *hbHl = new KHBox( this );
  layout->addWidget (hbHl);

  hbHl->setSpacing( -1 );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), hbHl );
  hlCombo = new KComboBox( hbHl );
  hlCombo->setEditable( false );
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );

  QPushButton *btnexport = new QPushButton( i18n("Export..."), hbHl );
  connect( btnexport,SIGNAL(clicked()),this,SLOT(exportHl()));

  QPushButton *btnimport = new QPushButton( i18n("Import..."), hbHl );
  connect( btnimport,SIGNAL(clicked()),this,SLOT(importHl()));

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

  // get current highlighting from the host application
  int hl = 0;
  KTextEditor::MdiContainer *iface = qobject_cast<KTextEditor::MdiContainer*>(KateGlobal::self()->container());
  if (iface) {
    KateView *kv = qobject_cast<KateView*>(iface->activeView());
    if (kv) {
      const QString hlName = kv->doc()->highlight()->name();
      hl = KateHlManager::self()->nameFind(hlName);
      Q_ASSERT(hl >= 0);
    }
  }
  hlCombo->setCurrentIndex ( hl );
  hlChanged ( hl );

  m_styles->setWhatsThis(i18n(
    "<p>This list displays the contexts of the current syntax highlight mode and "
    "offers the means to edit them. The context name reflects the current "
    "style settings.</p><p>To edit using the keyboard, press "
    "<strong>&lt;SPACE&gt;</strong> and choose a property from the popup menu.</p>"
    "<p>To edit the colors, click the colored squares, or select the color "
    "to edit from the popup menu.</p><p>You can unset the Background and Selected "
    "Background colors from the context menu when appropriate.</p>") );
}

KateSchemaConfigHighlightTab::~KateSchemaConfigHighlightTab()
{
}

void KateSchemaConfigHighlightTab::hlChanged(int z)
{
  m_hl = z;

  schemaChanged (m_schema);
}

bool KateSchemaConfigHighlightTab::loadAllHlsForSchema(int schema) {
  QProgressDialog progress(i18n("Loading all highlightings for schema"),i18n("Cancel"),0,KateHlManager::self()->highlights(),this);
  progress.setWindowModality(Qt::WindowModal);
  for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (!m_hlDict[schema].contains(i))
    {
      kDebug(13030) << "NEW HL, create list";

      QList<KateExtendedAttribute::Ptr> list;
      KateHlManager::self()->getHl( i )->getKateExtendedAttributeListCopy(KateGlobal::self()->schemaManager()->name (schema), list);
      m_hlDict[schema].insert (i, list);
    }
    progress.setValue(progress.value()+1);
    if (progress.wasCanceled()) {
      progress.setValue(KateHlManager::self()->highlights());
      return false;
    }
  }
  progress.setValue(KateHlManager::self()->highlights());
  return true;
}

void KateSchemaConfigHighlightTab::schemaChanged (int schema)
{
  m_schema = schema;

  kDebug(13030) << "NEW SCHEMA: " << m_schema << " NEW HL: " << m_hl;

  m_styles->clear ();

  if (!m_hlDict.contains(m_schema))
  {
    kDebug(13030) << "NEW SCHEMA, create dict";

    m_hlDict.insert (schema, QHash<int, QList<KateExtendedAttribute::Ptr> >());
  }

  if (!m_hlDict[m_schema].contains(m_hl))
  {
    kDebug(13030) << "NEW HL, create list";

    QList<KateExtendedAttribute::Ptr> list;
    KateHlManager::self()->getHl( m_hl )->getKateExtendedAttributeListCopy(KateGlobal::self()->schemaManager()->name (m_schema), list);
    m_hlDict[m_schema].insert (m_hl, list);
  }

  KateAttributeList *l = m_defaults->attributeList (schema);

  // Set listview colors
  // We do that now, because we can now get the "normal text" color.
  // TODO this reads of the KConfig object, which should be changed when
  // the color tab is fixed.
  QPalette p ( m_styles->palette() );
  KColorScheme s ( QPalette::Active, KColorScheme::View );
  QColor _c ( s.background().color() );
  p.setColor( QPalette::Base,
    KateGlobal::self()->schemaManager()->schema(m_schema).
      readEntry( "Color Background", _c ) );
  _c = KColorScheme(QPalette::Active, KColorScheme::Selection).background().color();
  p.setColor( QPalette::Highlight,
    KateGlobal::self()->schemaManager()->schema(m_schema).
      readEntry( "Color Selection", _c ) );
  _c = l->at(0)->foreground().color(); // not quite as much of an assumption ;)
  p.setColor( QPalette::Text, _c );
  m_styles->viewport()->setPalette( p );

  QHash<QString, QTreeWidgetItem*> prefixes;
  QList<KateExtendedAttribute::Ptr>::ConstIterator it = m_hlDict[m_schema][m_hl].constBegin();
  while (it != m_hlDict[m_schema][m_hl].constEnd())
  {
    const KateExtendedAttribute::Ptr itemData = *it;
    Q_ASSERT(itemData);

    kDebug(13030) << "insert items " << itemData->name();

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
    ++it;
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


QList<int> KateSchemaConfigHighlightTab::hlsForSchema(int schema) {
  return m_hlDict[schema].keys();
}


void KateSchemaConfigHighlightTab::importHl(const QString& fromSchemaName, int schema, int hl, KConfig *cfg) {
        QString schemaNameForLoading(fromSchemaName);
        QString hlName;
        bool doManage=(cfg==0);
        if (schema==-1) schema=m_schema;

        if (doManage) {
          QString srcName=KFileDialog::getOpenFileName( QString(KateHlManager::self()->getHl(hl)->name()+QString(".katehlcolor")),
                                  QString::fromLatin1("*.katehlcolor|%1").arg(i18n("Kate color schema")),
                                  this,
                                  i18n("Importing colors for single highlighting"));
          kDebug(13030)<<"hl file to open "<<srcName;
          if (srcName.isEmpty()) return;
          cfg=new KConfig(srcName,KConfig::SimpleConfig);
          KConfigGroup grp(cfg,"KateHLColors");
          hlName=grp.readEntry("highlight",QString());
          schemaNameForLoading=grp.readEntry("schema",QString());
          if ( (grp.readEntry("full schema","true").toUpper()!="FALSE") || hlName.isEmpty() || schemaNameForLoading.isEmpty()) {
            //ERROR - file format
            KMessageBox::information(
                    this,
                    i18n("File is not a single highlighting color file"),
                    i18n("Fileformat error"));
            hl=-1;
            schemaNameForLoading=QString();
          } else {
            hl = KateHlManager::self()->nameFind(hlName);
            kDebug(13030)<<hlName<<"--->"<<hl;
            if (hl==-1) {
              //hl not found
              KMessageBox::information(
                      this,
                      i18n("The selected file contains colors for a non existing highlighting:%1",hlName),
                      i18n("Import failure"));
              hl=-1;
              schemaNameForLoading=QString();
            }
          }
        }

        if ( (hl!=-1) && (!schemaNameForLoading.isEmpty())) {

          QList<KateExtendedAttribute::Ptr> list;
          KateHlManager::self()->getHl( hl )->getKateExtendedAttributeListCopy(schemaNameForLoading, list, cfg);
          KateHlManager::self()->getHl( hl )->setKateExtendedAttributeList(schema, list);
          m_hlDict[schema].insert (hl, list);
        }

        if (cfg && doManage) {
          apply();
          delete cfg;
          cfg=0;
          if ( (hl!=-1) && (!schemaNameForLoading.isEmpty())) {
            hlChanged(m_hl);
            KMessageBox::information(
                      this,
                      i18n("Colors have been imported for highlighting: %1",hlName),
                      i18n("Import has finished"));
          }
        }


}


void KateSchemaConfigHighlightTab::exportHl(int schema, int hl, KConfig *cfg) {
  bool doManage=(cfg==0);
  if (schema==-1) schema=m_schema;
  if (hl==-1) hl=m_hl;

  QList<KateExtendedAttribute::Ptr> items=m_hlDict[schema][hl];
  if (doManage)  {
    QString destName=KFileDialog::getSaveFileName( QString(KateHlManager::self()->getHl(hl)->name()+".katehlcolor"),
                                    QString::fromLatin1("*.katehlcolor|%1").arg(i18n("Kate color schema")),
                                    this,
                                    i18n("Exporting colors for single highlighting: %1", KateHlManager::self()->getHl(hl)->name()),
                                    KFileDialog::ConfirmOverwrite );

    if (destName.isEmpty()) return;

    cfg=new KConfig(destName,KConfig::SimpleConfig);
    KConfigGroup grp(cfg,"KateHLColors");
    grp.writeEntry("highlight",KateHlManager::self()->getHl(hl)->name());
    grp.writeEntry("schema",KateGlobal::self()->schemaManager()->name(schema));
    grp.writeEntry("full schema","false");
  }
  KateHlManager::self()->getHl(hl)->setKateExtendedAttributeList(schema,items,cfg,doManage);

  if (doManage) {
    cfg->sync();
    delete cfg;
  }

}

//END KateSchemaConfigHighlightTab

//BEGIN KateSchemaConfigPage -- Main dialog page
KateSchemaConfigPage::KateSchemaConfigPage( QWidget *parent)
  : KateConfigPage( parent ),
    m_lastSchema (-1)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);

  KHBox *hbHl = new KHBox( this );
  layout->addWidget(hbHl);
  hbHl->setSpacing( -1 );
  QLabel *lHl = new QLabel( i18n("&Schema:"), hbHl );
  schemaCombo = new KComboBox( hbHl );
  schemaCombo->setEditable( false );
  lHl->setBuddy( schemaCombo );
  connect( schemaCombo, SIGNAL(activated(int)),
           this, SLOT(schemaChanged(int)) );

  QPushButton *btnnew = new QPushButton( i18n("&New..."), hbHl );
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newSchema()) );

  btndel = new QPushButton( i18n("&Delete"), hbHl );
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteSchema()) );

  QPushButton *btnexport = new QPushButton( i18n("Export..."), hbHl );
  connect(btnexport,SIGNAL(clicked()),this,SLOT(exportFullSchema()));
  QPushButton *btnimport = new QPushButton( i18n("Import..."), hbHl );
  connect(btnimport,SIGNAL(clicked()),this,SLOT(importFullSchema()));

  qobject_cast<QBoxLayout *>(hbHl->layout())->addStretch();

  m_tabWidget = new KTabWidget ( this );
  layout->addWidget (m_tabWidget);

  m_colorTab = new KateSchemaConfigColorTab();
  m_tabWidget->addTab (m_colorTab, i18n("Colors"));
  connect(m_colorTab, SIGNAL(changed()), SLOT(slotChanged()));

  m_fontTab = new KateSchemaConfigFontTab();
  m_tabWidget->addTab (m_fontTab, i18n("Font"));
  connect(m_fontTab, SIGNAL(changed()), SLOT(slotChanged()));

  m_fontColorTab = new KateSchemaConfigFontColorTab();
  m_tabWidget->addTab (m_fontColorTab, i18n("Default Text Styles"));
  connect(m_fontColorTab, SIGNAL(changed()), SLOT(slotChanged()));

  m_highlightTab = new KateSchemaConfigHighlightTab(m_fontColorTab);
  m_tabWidget->addTab(m_highlightTab, i18n("Highlighting Text Styles"));
  connect(m_highlightTab, SIGNAL(changed()), SLOT(slotChanged()));

  connect (m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT (newCurrentPage(int)));

  hbHl = new KHBox( this );
  layout->addWidget (hbHl);
  hbHl->setSpacing( -1 );
  lHl = new QLabel( i18n("&Default schema for %1:", KGlobal::mainComponent().aboutData()->programName ()), hbHl );
  defaultSchemaCombo = new KComboBox( hbHl );
  defaultSchemaCombo->setEditable( false );
  lHl->setBuddy( defaultSchemaCombo );

  m_defaultSchema = KateGlobal::self()->schemaManager()->number (KateRendererConfig::global()->schema());

  reload();

  connect( defaultSchemaCombo, SIGNAL(activated(int)),
           this, SLOT(slotChanged()) );
}

void KateSchemaConfigPage::exportFullSchema() {
  QString destName=KFileDialog::getSaveFileName( QString(KateGlobal::self()->schemaManager()->name(m_lastSchema)+".kateschema"),
                                    QString::fromLatin1("*.kateschema|%1").arg(i18n("Kate color schema")),
                                    this,
                                    i18n("Exporting color schema:%1", KateGlobal::self()->schemaManager()->name(m_lastSchema)),
                                    KFileDialog::ConfirmOverwrite );

  if (destName.isEmpty()) return;
  if (!m_highlightTab->loadAllHlsForSchema(m_lastSchema)) {
    //ABORT - MESSAGE
    return;
  }
  QStringList hlList;
  QList<int> hls=m_highlightTab->hlsForSchema(m_lastSchema);
  KConfig cfg(destName,KConfig::SimpleConfig);
  m_fontColorTab->exportDefaults(m_lastSchema,&cfg);
  int cnt=0;
  QProgressDialog progress(i18n("Exporting schema"),i18n("Stop"),0,hls.count(),this);
  progress.setWindowModality(Qt::WindowModal);
  foreach(int hl,hls) {
    hlList<<KateHlManager::self()->getHl (hl)->name();
    m_highlightTab->exportHl(m_lastSchema,hl,&cfg);
    cnt++;
    progress.setValue(cnt);
    if (progress.wasCanceled()) break;
  }
  progress.setValue(hls.count());
  KConfigGroup grp(&cfg,"KateSchema");
  grp.writeEntry("full schema","true");
  grp.writeEntry("highlightings",hlList);
  grp.writeEntry("schema",KateGlobal::self()->schemaManager()->name(m_lastSchema));
  cfg.sync();

}

void KateSchemaConfigPage::importFullSchema() {
  QString srcName=KFileDialog::getOpenFileName( KUrl(),
                                    "*.kateschema|Kate color schema",
                                    this,
                                    i18n("Importing color schema")
                                    );

  if (srcName.isEmpty())  return;
  KConfig cfg(srcName,KConfig::SimpleConfig);
  KConfigGroup grp(&cfg,"KateSchema");
  if (grp.readEntry("full schema","false").toUpper()!="TRUE") {
    KMessageBox::information(
            this,
            i18n("File is not a full schema file"),
            i18n("Fileformat error"));
    return;
  }
  QStringList highlightings=grp.readEntry("highlightings",QStringList());
  QString schemaName=grp.readEntry("schema",i18n("Name unspecified"));
  QString fromSchemaName=schemaName;
  bool reask=true;
  do {
    KDialog howToImportDialog(this);
    Ui_KateHowToImportSchema howToImport;
    howToImport.setupUi(howToImportDialog.mainWidget());
    if (KateGlobal::self()->schemaManager()->list().contains(schemaName)) {
      howToImport.radioReplaceExisting->show();
      howToImport.radioReplaceExisting->setText(i18n("Replace existing schema %1",schemaName));
      howToImport.radioReplaceExisting->setChecked(true);
    } else {
      howToImport.radioReplaceExisting->hide();
      howToImport.newName->setText(schemaName);
    }
    if (howToImportDialog.exec()==KDialog::Cancel) {
      schemaName=QString();
      reask=false;
    } else {
      if (howToImport.radioReplaceExisting->isChecked()) {
        reask=false;
      } else if (howToImport.radioReplaceCurrent->isChecked()) {
        schemaName=KateGlobal::self()->schemaManager()->list()[m_lastSchema];
        reask=false;
      } else if (howToImport.radioAsNew->isChecked())  {
        schemaName=howToImport.newName->text();
        if (KateGlobal::self()->schemaManager()->list().contains(schemaName)) {
          reask=true;
        } else reask=false;
      } else reask=true;
    }
  } while (reask);
  if (!schemaName.isEmpty()) {
    kDebug(13030)<<"Importing schema:"<<schemaName;
    bool asNew=!KateGlobal::self()->schemaManager()->list().contains(schemaName);
    kDebug(13030)<<"schema exists?"<<(!asNew);
    int schemaId;
    if (asNew) {
      newSchema(schemaName);
      schemaId=m_lastSchema;
    } else {
      schemaId=KateGlobal::self()->schemaManager()->list().indexOf(schemaName);
    }
    schemaChanged(schemaId);
    m_fontColorTab->importDefaults(fromSchemaName,schemaId,&cfg);
    m_fontColorTab->apply();
    const int hlCount=KateHlManager::self()->highlights();
    QHash<QString,int> nameToId;
    for(int i=0;i<hlCount;i++) {
      nameToId.insert(KateHlManager::self()->hlName(i),i);
    }

    int cnt=0;
    QProgressDialog progress(i18n("Importing schema"),i18n("Stop"),0,highlightings.count(),this);
    progress.setWindowModality(Qt::WindowModal);
    foreach(const QString& hl,highlightings) {
      if (nameToId.contains(hl)) {
        int i=nameToId[hl];
        m_highlightTab->importHl(fromSchemaName,schemaId,i,&cfg);
        kDebug(13030)<<"hl imported:"<<hl;
      } else {
        kDebug(13030)<<"could not import hl, hl unknown:"<<hl;
      }
      cnt++;
      progress.setValue(cnt);
      if (progress.wasCanceled()) break;
    }
    progress.setValue(highlightings.count());
    m_highlightTab->apply();
    schemaChanged(schemaId);
  }
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
  KateRendererConfig::global()->setSchema (KateGlobal::self()->schemaManager()->name (defaultSchemaCombo->currentIndex()));
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

  defaultSchemaCombo->setCurrentIndex (KateGlobal::self()->schemaManager()->number (KateRendererConfig::global()->schema()));

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

void KateSchemaConfigPage::newSchema (const QString& newName)
{
  QString t;
  if (newName.isEmpty())
    t = KInputDialog::getText (i18n("Name for New Schema"), i18n ("Name:"), i18n("New Schema"), 0, this);
  else
    t=newName;
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

void KateSchemaConfigPage::newCurrentPage (int index)
{
   QWidget *w = m_tabWidget->widget(index);
   if (w == m_highlightTab)
    m_highlightTab->schemaChanged (m_lastSchema);
}
//END KateSchemaConfigPage

// kate: space-indent on; indent-width 2; replace-tabs on;
