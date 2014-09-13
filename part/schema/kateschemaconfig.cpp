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
#include "katedefaultcolors.h"

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
  QGridLayout* l = new QGridLayout(this);
  setLayout(l);

  ui = new KateColorTreeWidget(this);
  QPushButton* btnUseColorScheme = new QPushButton(i18n("Use KDE Color Scheme"), this);

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

  KateDefaultColors colors;

  //
  // editor background colors
  //
  KateColorItem ci;
  ci.category = i18n("Editor Background Colors");

  ci.name = i18n("Text Area");
  ci.key = "Color Background";
  ci.whatsThis = i18n("<p>Sets the background color of the editing area.</p>");
  ci.defaultColor = colors.color(Kate::Background);
  items.append(ci);

  ci.name = i18n("Selected Text");
  ci.key = "Color Selection";
  ci.whatsThis = i18n("<p>Sets the background color of the selection.</p><p>To set the text color for selected text, use the &quot;<b>Configure Highlighting</b>&quot; dialog.</p>");
  ci.defaultColor = colors.color(Kate::SelectionBackground);
  items.append(ci);

  ci.name = i18n("Current Line");
  ci.key = "Color Highlighted Line";
  ci.whatsThis = i18n("<p>Sets the background color of the currently active line, which means the line where your cursor is positioned.</p>");
  ci.defaultColor = colors.color(Kate::HighlightedLineBackground);
  items.append(ci);

  ci.name = i18n("Search Highlight");
  ci.key = "Color Search Highlight";
  ci.whatsThis = i18n("<p>Sets the background color of search results.</p>");
  ci.defaultColor = colors.color(Kate::SearchHighlight);
  items.append(ci);

  ci.name = i18n("Replace Highlight");
  ci.key = "Color Replace Highlight";
  ci.whatsThis = i18n("<p>Sets the background color of replaced text.</p>");
  ci.defaultColor = colors.color(Kate::ReplaceHighlight);
  items.append(ci);


  //
  // icon border
  //
  ci.category = i18n("Icon Border");

  ci.name = i18n("Background Area");
  ci.key = "Color Icon Bar";
  ci.whatsThis = i18n("<p>Sets the background color of the icon border.</p>");
  ci.defaultColor = colors.color(Kate::IconBar);
  items.append(ci);

  ci.name = i18n("Line Numbers");
  ci.key = "Color Line Number";
  ci.whatsThis = i18n("<p>This color will be used to draw the line numbers (if enabled).</p>");
  ci.defaultColor = colors.color(Kate::LineNumber);
  items.append(ci);

  ci.name = i18n("Separator");
  ci.key = "Color Separator";
  ci.whatsThis = i18n("<p>This color will be used to draw the line between line numbers and the icon borders, if both are enabled.</p>");
  ci.defaultColor = colors.color(Kate::Separator);
  items.append(ci);

  ci.name = i18n("Word Wrap Marker");
  ci.key = "Color Word Wrap Marker";
  ci.whatsThis = i18n("<p>Sets the color of Word Wrap-related markers:</p><dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where text is going to be wrapped</dd><dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of visually-wrapped lines</dd></dl>");
  ci.defaultColor = colors.color(Kate::WordWrapMarker);
  items.append(ci);

  ci.name = i18n("Code Folding");
  ci.key = "Color Code Folding";
  ci.whatsThis = i18n("<p>Sets the color of the code folding bar.</p>");
  ci.defaultColor = colors.color(Kate::CodeFolding);
  items.append(ci);


  ci.name = i18n("Modified Lines");
  ci.key = "Color Modified Lines";
  ci.whatsThis = i18n("<p>Sets the color of the line modification marker for modified lines.</p>");
  ci.defaultColor = colors.color(Kate::ModifiedLine);
  items.append(ci);

  ci.name = i18n("Saved Lines");
  ci.key = "Color Saved Lines";
  ci.whatsThis = i18n("<p>Sets the color of the line modification marker for saved lines.</p>");
  ci.defaultColor = colors.color(Kate::SavedLine);
  items.append(ci);


  //
  // text decorations
  //
  ci.category = i18n("Text Decorations");

  ci.name = i18n("Spelling Mistake Line");
  ci.key = "Color Spelling Mistake Line";
  ci.whatsThis = i18n("<p>Sets the color of the line that is used to indicate spelling mistakes.</p>");
  ci.defaultColor = colors.color(Kate::SpellingMistakeLine);
  items.append(ci);

  ci.name = i18n("Tab and Space Markers");
  ci.key = "Color Tab Marker";
  ci.whatsThis = i18n("<p>Sets the color of the tabulator marks.</p>");
  ci.defaultColor = colors.color(Kate::TabMarker);
  items.append(ci);

  ci.name = i18n("Indentation Line");
  ci.key = "Color Indentation Line";
  ci.whatsThis = i18n("<p>Sets the color of the vertical indentation lines.</p>");
  ci.defaultColor = colors.color(Kate::IndentationLine);
  items.append(ci);

  ci.name = i18n("Bracket Highlight");
  ci.key = "Color Highlighted Bracket";
  ci.whatsThis = i18n("<p>Sets the bracket matching color. This means, if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will be highlighted with this color.</p>");
  ci.defaultColor = colors.color(Kate::HighlightedBracket);
  items.append(ci);


  //
  // marker colors
  //
  ci.category = i18n("Marker Colors");

  const QString markerNames[Kate::LAST_MARK + 1] = {
    i18n("Bookmark"),
    i18n("Active Breakpoint"),
    i18n("Reached Breakpoint"),
    i18n("Disabled Breakpoint"),
    i18n("Execution"),
    i18n("Warning"),
    i18n("Error")
  };

  ci.whatsThis = i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>");
  for (int i = Kate::FIRST_MARK; i <= Kate::LAST_MARK; ++i) {
    ci.defaultColor = colors.mark(i);
    ci.name = markerNames[i];
    ci.key = "Color MarkType " + QString::number(i + 1);
    items.append(ci);
  }

  //
  // text templates
  //
  ci.category = i18n("Text Templates & Snippets");

  ci.whatsThis = QString(); // TODO: add whatsThis for text templates

  ci.name = i18n("Background");
  ci.key = "Color Template Background";
  ci.defaultColor = colors.color(Kate::TemplateBackground);
  items.append(ci);

  ci.name = i18n("Editable Placeholder");
  ci.key = "Color Template Editable Placeholder";
  ci.defaultColor = colors.color(Kate::TemplateEditablePlaceholder);
  items.append(ci);

  ci.name = i18n("Focused Editable Placeholder");
  ci.key = "Color Template Focused Editable Placeholder";
  ci.defaultColor = colors.color(Kate::TemplateFocusedEditablePlaceholder);
  items.append(ci);

  ci.name = i18n("Not Editable Placeholder");
  ci.key = "Color Template Not Editable Placeholder";
  ci.defaultColor = colors.color(Kate::TemplateNotEditablePlaceholder);
  items.append(ci);


  //
  // finally, add all elements
  //
  return items;
}

void KateSchemaConfigColorTab::schemaChanged ( const QString &newSchema )
{
  // save curent schema
  if ( !m_currentSchema.isEmpty() ) {
    if (m_schemas.contains(m_currentSchema)) {
      m_schemas.remove(m_currentSchema); // clear this color schema
    }

    // now add it again
    m_schemas[m_currentSchema] = ui->colorItems();
  }

  if ( newSchema == m_currentSchema ) return;

  // switch
  m_currentSchema = newSchema;

  // If we havent this schema, read in from config file
  if ( ! m_schemas.contains( newSchema ) )
  {
    KConfigGroup config = KateGlobal::self()->schemaManager()->schema(newSchema);
    QVector<KateColorItem> items = readConfig(config);

    m_schemas[ newSchema ] = items;
  }

  // first block signals otherwise setColor emits changed
  const bool blocked = blockSignals(true);

  ui->clear();
  ui->addColorItems(m_schemas[m_currentSchema]);

  blockSignals(blocked);
}

QVector<KateColorItem> KateSchemaConfigColorTab::readConfig(KConfigGroup& config)
{
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
  return items;
}

void KateSchemaConfigColorTab::importSchema(KConfigGroup& config)
{
  m_schemas[m_currentSchema] = readConfig(config);

  // first block signals otherwise setColor emits changed
  const bool blocked = blockSignals(true);

  ui->clear();
  ui->addColorItems(m_schemas[m_currentSchema]);

  blockSignals(blocked);
}

void KateSchemaConfigColorTab::exportSchema(KConfigGroup& config)
{
  QVector<KateColorItem> items = ui->colorItems();
  foreach (const KateColorItem& item, items) {
    QColor c = item.useDefault ? item.defaultColor : item.color;
    config.writeEntry(item.key, c);
  }
}

void KateSchemaConfigColorTab::apply ()
{
  schemaChanged( m_currentSchema );
  QMap<QString, QVector<KateColorItem> >::Iterator it;
  for ( it =  m_schemas.begin(); it !=  m_schemas.end(); ++it )
  {
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

    // add dummy entry to prevent the config group from being empty.
    // As if the group is empty, KateSchemaManager will not find it anymore.
    config.writeEntry("dummy", "prevent-empty-group");
  }

  // all colors are written, so throw away all cached schemas
  m_schemas.clear();
}

void KateSchemaConfigColorTab::reload()
{
  // drop all cached data
  m_schemas.clear();

  // load from config
  KConfigGroup config = KateGlobal::self()->schemaManager()->schema(m_currentSchema);
  QVector<KateColorItem> items = readConfig(config);

  // first block signals otherwise setColor emits changed
  const bool blocked = blockSignals(true);

  ui->clear();
  ui->addColorItems(items);

  blockSignals(blocked);
}

QColor KateSchemaConfigColorTab::backgroundColor() const
{
  return ui->findColor("Color Background");
}

QColor KateSchemaConfigColorTab::selectionColor() const
{
  return ui->findColor("Color Selection");
}
//END KateSchemaConfigColorTab

//BEGIN FontConfig -- 'Fonts' tab
KateSchemaConfigFontTab::KateSchemaConfigFontTab()
{
  QGridLayout *grid = new QGridLayout( this );

  m_fontchooser = new KFontChooser ( this, KFontChooser::NoDisplayFlags );
  grid->addWidget( m_fontchooser, 0, 0);
}

KateSchemaConfigFontTab::~KateSchemaConfigFontTab()
{
}

void KateSchemaConfigFontTab::slotFontSelected( const QFont &font )
{
  if ( !m_currentSchema.isEmpty() ) {
    m_fonts[m_currentSchema] = font;
    emit changed();
  }
}

void KateSchemaConfigFontTab::apply()
{
  QMap<QString, QFont>::Iterator it;
  for ( it = m_fonts.begin(); it != m_fonts.end(); ++it )
  {
    KateGlobal::self()->schemaManager()->schema( it.key() ).writeEntry( "Font", it.value() );
  }

  // all fonts are written, so throw away all cached schemas
  m_fonts.clear();
}

void KateSchemaConfigFontTab::reload()
{
  // drop all cached data
  m_fonts.clear();

  // now set current schema font in the font chooser
  schemaChanged(m_currentSchema);
}

void KateSchemaConfigFontTab::schemaChanged( const QString &newSchema )
{
  m_currentSchema = newSchema;

  // reuse font, if cached
  QFont newFont (KGlobalSettings::fixedFont());
  if (m_fonts.contains(m_currentSchema)) {
    newFont = m_fonts[m_currentSchema];
  } else {
    newFont = KateGlobal::self()->schemaManager()->schema(m_currentSchema).readEntry("Font", newFont);
  }

  m_fontchooser->disconnect ( this );
  m_fontchooser->setFont ( newFont );
  connect (m_fontchooser, SIGNAL (fontSelected(QFont)), this, SLOT (slotFontSelected(QFont)));
}

void KateSchemaConfigFontTab::importSchema(KConfigGroup& config)
{
  QFont f (KGlobalSettings::fixedFont());
  m_fontchooser->setFont(config.readEntry("Font", f));
  m_fonts[m_currentSchema] = m_fontchooser->font();
}

void KateSchemaConfigFontTab::exportSchema(KConfigGroup& config)
{
  config.writeEntry("Font", m_fontchooser->font());
}
//END FontConfig

//BEGIN FontColorConfig -- 'Normal Text Styles' tab
KateSchemaConfigDefaultStylesTab::KateSchemaConfigDefaultStylesTab(KateSchemaConfigColorTab* colorTab)
{
  m_colorTab = colorTab;

  // size management
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

KateSchemaConfigDefaultStylesTab::~KateSchemaConfigDefaultStylesTab()
{
  qDeleteAll(m_defaultStyleLists);
}

KateAttributeList *KateSchemaConfigDefaultStylesTab::attributeList (const QString &schema)
{
  if (!m_defaultStyleLists.contains(schema))
  {
    KateAttributeList *list = new KateAttributeList ();
    KateHlManager::self()->getDefaults(schema, *list);

    m_defaultStyleLists.insert (schema, list);
  }

  return m_defaultStyleLists[schema];
}

void KateSchemaConfigDefaultStylesTab::schemaChanged (const QString &schema)
{
  m_currentSchema = schema;

  m_defaultStyles->clear ();

  KateAttributeList *l = attributeList (schema);
  updateColorPalette(l->at(0)->foreground().color());

  for ( uint i = 0; i < KateHlManager::self()->defaultStyles(); i++ )
  {
    m_defaultStyles->addItem( KateHlManager::self()->defaultStyleName(i, true), l->at( i ) );
  }
}

void KateSchemaConfigDefaultStylesTab::updateColorPalette(const QColor& textColor)
{
  QPalette p ( m_defaultStyles->palette() );
  p.setColor( QPalette::Base, m_colorTab->backgroundColor() );
  p.setColor( QPalette::Highlight, m_colorTab->selectionColor() );
  p.setColor( QPalette::Text, textColor );
  m_defaultStyles->setPalette( p );
}

void KateSchemaConfigDefaultStylesTab::reload ()
{
  m_defaultStyles->clear ();
  qDeleteAll(m_defaultStyleLists);
  m_defaultStyleLists.clear ();

  schemaChanged(m_currentSchema);
}

void KateSchemaConfigDefaultStylesTab::apply ()
{
  QHashIterator<QString,KateAttributeList*> it = m_defaultStyleLists;
  while (it.hasNext()) {
    it.next();
    KateHlManager::self()->setDefaults(it.key(), *it.value());
  }
}

void KateSchemaConfigDefaultStylesTab::exportSchema(const QString &schema, KConfig *cfg)
{
  KateHlManager::self()->setDefaults(schema, *(m_defaultStyleLists[schema]),cfg);
}

void KateSchemaConfigDefaultStylesTab::importSchema(const QString& schemaName, const QString &schema, KConfig *cfg)
{
  KateHlManager::self()->getDefaults(schemaName, *(m_defaultStyleLists[schema]),cfg);
}

void KateSchemaConfigDefaultStylesTab::showEvent(QShowEvent* event)
{
  if (!event->spontaneous() && !m_currentSchema.isEmpty()) {
    KateAttributeList *l = attributeList (m_currentSchema);
    Q_ASSERT(l != 0);
    updateColorPalette(l->at(0)->foreground().color());
  }

  QWidget::showEvent(event);
}
//END FontColorConfig

//BEGIN KateSchemaConfigHighlightTab -- 'Highlighting Text Styles' tab
KateSchemaConfigHighlightTab::KateSchemaConfigHighlightTab(KateSchemaConfigDefaultStylesTab *page, KateSchemaConfigColorTab* colorTab)
{
  m_defaults = page;
  m_colorTab = colorTab;

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
  QPushButton *btnimport = new QPushButton( i18n("Import..."), hbHl );

  qobject_cast<QBoxLayout*>(hbHl->layout())->addStretch();

  connect( btnexport,SIGNAL(clicked()),this,SLOT(exportHl()));
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

bool KateSchemaConfigHighlightTab::loadAllHlsForSchema(const QString &schema)
{
  QProgressDialog progress(i18n("Loading all highlightings for schema"), QString(), 0, KateHlManager::self()->highlights(), this);
  progress.setWindowModality(Qt::WindowModal);
  for (int i = 0; i < KateHlManager::self()->highlights(); ++i) {
    if (!m_hlDict[schema].contains(i))
    {
      kDebug(13030) << "NEW HL, create list";

      QList<KateExtendedAttribute::Ptr> list;
      KateHlManager::self()->getHl(i)->getKateExtendedAttributeListCopy(schema, list);
      m_hlDict[schema].insert (i, list);
    }
    progress.setValue(progress.value() + 1);
  }
  progress.setValue(KateHlManager::self()->highlights());
  return true;
}

void KateSchemaConfigHighlightTab::schemaChanged (const QString &schema)
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
    KateHlManager::self()->getHl( m_hl )->getKateExtendedAttributeListCopy(m_schema, list);
    m_hlDict[m_schema].insert (m_hl, list);
  }

  KateAttributeList *l = m_defaults->attributeList (schema);

  // Set listview colors
  updateColorPalette(l->at(0)->foreground().color());

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

void KateSchemaConfigHighlightTab::updateColorPalette(const QColor& textColor)
{
  QPalette p ( m_styles->palette() );
  p.setColor( QPalette::Base, m_colorTab->backgroundColor() );
  p.setColor( QPalette::Highlight, m_colorTab->selectionColor() );
  p.setColor( QPalette::Text, textColor );
  m_styles->setPalette( p );
}

void KateSchemaConfigHighlightTab::reload ()
{
  m_styles->clear ();

  m_hlDict.clear ();

  hlChanged( hlCombo->currentIndex() );
}

void KateSchemaConfigHighlightTab::apply ()
{
  QMutableHashIterator<QString, QHash<int, QList<KateExtendedAttribute::Ptr> > > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    QMutableHashIterator<int, QList<KateExtendedAttribute::Ptr> > it2 = it.value();
    while (it2.hasNext()) {
      it2.next();
      KateHlManager::self()->getHl( it2.key() )->setKateExtendedAttributeList (it.key(), it2.value());
    }
  }
}


QList<int> KateSchemaConfigHighlightTab::hlsForSchema(const QString &schema) {
  return m_hlDict[schema].keys();
}


void KateSchemaConfigHighlightTab::importHl(const QString& fromSchemaName, QString schema, int hl, KConfig *cfg) {
        QString schemaNameForLoading(fromSchemaName);
        QString hlName;
        bool doManage=(cfg==0);
        if (schema.isEmpty()) schema=m_schema;

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


void KateSchemaConfigHighlightTab::exportHl(QString schema, int hl, KConfig *cfg) {
  bool doManage=(cfg==0);
  if (schema.isEmpty()) schema=m_schema;
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
    grp.writeEntry("schema",schema);
    grp.writeEntry("full schema","false");
  }
  KateHlManager::self()->getHl(hl)->setKateExtendedAttributeList(schema,items,cfg,doManage);

  if (doManage) {
    cfg->sync();
    delete cfg;
  }

}

void KateSchemaConfigHighlightTab::showEvent(QShowEvent* event)
{
  if (!event->spontaneous()) {
    KateAttributeList *l = m_defaults->attributeList (m_schema);
    Q_ASSERT(l != 0);
    updateColorPalette(l->at(0)->foreground().color());
  }

  QWidget::showEvent(event);
}
//END KateSchemaConfigHighlightTab

//BEGIN KateSchemaConfigPage -- Main dialog page
KateSchemaConfigPage::KateSchemaConfigPage( QWidget *parent)
  : KateConfigPage( parent ),
    m_currentSchema (-1)
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
  connect( schemaCombo, SIGNAL(currentIndexChanged(int)),
           this, SLOT(comboBoxIndexChanged(int)) );

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

  m_defaultStylesTab = new KateSchemaConfigDefaultStylesTab(m_colorTab);
  m_tabWidget->addTab (m_defaultStylesTab, i18n("Default Text Styles"));
  connect(m_defaultStylesTab, SIGNAL(changed()), SLOT(slotChanged()));

  m_highlightTab = new KateSchemaConfigHighlightTab(m_defaultStylesTab, m_colorTab);
  m_tabWidget->addTab(m_highlightTab, i18n("Highlighting Text Styles"));
  connect(m_highlightTab, SIGNAL(changed()), SLOT(slotChanged()));

  hbHl = new KHBox( this );
  layout->addWidget (hbHl);
  hbHl->setSpacing( -1 );
  lHl = new QLabel( i18n("&Default schema for %1:", KGlobal::mainComponent().aboutData()->programName ()), hbHl );
  defaultSchemaCombo = new KComboBox( hbHl );
  defaultSchemaCombo->setEditable( false );
  lHl->setBuddy( defaultSchemaCombo );

  reload();

  connect( defaultSchemaCombo, SIGNAL(currentIndexChanged(int)),
           this, SLOT(slotChanged()) );
}

KateSchemaConfigPage::~KateSchemaConfigPage ()
{
}

void KateSchemaConfigPage::exportFullSchema()
{
  // get save destination
  const QString currentSchemaName = m_currentSchema;
  QString destName = KFileDialog::getSaveFileName(
      QString(currentSchemaName + ".kateschema"),
      QString::fromLatin1("*.kateschema|%1").arg(i18n("Kate color schema")),
      this,
      i18n("Exporting color schema: %1", currentSchemaName),
      KFileDialog::ConfirmOverwrite );

  if (destName.isEmpty()) return;

  // open config file
  KConfig cfg(destName, KConfig::SimpleConfig);

  //
  // export editor Colors (background, ...)
  //
  KConfigGroup colorConfigGroup(&cfg, "Editor Colors");
  m_colorTab->exportSchema(colorConfigGroup);

  //
  // export Default Styles
  //
  m_defaultStylesTab->exportSchema(m_currentSchema, &cfg);

  //
  // export Highlighting Text Styles
  //
  // force a load of all Highlighting Text Styles
  QStringList hlList;
  m_highlightTab->loadAllHlsForSchema(m_currentSchema);

  QList<int> hls = m_highlightTab->hlsForSchema(m_currentSchema);

  int cnt = 0;
  QProgressDialog progress(i18n("Exporting schema"), QString(), 0, hls.count(), this);
  progress.setWindowModality(Qt::WindowModal);
  foreach (int hl, hls) {
    hlList << KateHlManager::self()->getHl (hl)->name();
    m_highlightTab->exportHl(m_currentSchema, hl, &cfg);
    progress.setValue(++cnt);
    if (progress.wasCanceled()) break;
  }
  progress.setValue(hls.count());

  KConfigGroup grp(&cfg, "KateSchema");
  grp.writeEntry("full schema", "true");
  grp.writeEntry("highlightings", hlList);
  grp.writeEntry("schema", currentSchemaName);
  m_fontTab->exportSchema(grp);
  cfg.sync();
}

QString KateSchemaConfigPage::requestSchemaName(const QString& suggestedName)
{
  QString schemaName = suggestedName;

  bool reask = true;
  do {
    KDialog howToImportDialog(this);
    Ui_KateHowToImportSchema howToImport;
    howToImport.setupUi(howToImportDialog.mainWidget());

    // if schema exists, prepare option to replace
    if (KateGlobal::self()->schemaManager()->schema(schemaName).exists()) {
      howToImport.radioReplaceExisting->show();
      howToImport.radioReplaceExisting->setText(i18n("Replace existing schema %1", schemaName));
      howToImport.radioReplaceExisting->setChecked(true);
    } else {
      howToImport.radioReplaceExisting->hide();
      howToImport.newName->setText(schemaName);
    }
    
    // cancel pressed?
    if (howToImportDialog.exec() == KDialog::Cancel) {
      schemaName.clear();
      reask = false;
    }
    // check what the user wants
    else {
      // replace existing
      if (howToImport.radioReplaceExisting->isChecked()) {
        reask = false;
      }
      // replace current
      else if (howToImport.radioReplaceCurrent->isChecked()) {
        schemaName = m_currentSchema;
        reask = false;
      }
      // new one, check again, whether the schema already exists
      else if (howToImport.radioAsNew->isChecked()) {
        schemaName = howToImport.newName->text();
        if (KateGlobal::self()->schemaManager()->schema(schemaName).exists()) {
          reask = true;
        } else reask = false;
      }
      // should never happen
      else reask = true;
    }
  } while (reask);

  return schemaName;
}

void KateSchemaConfigPage::importFullSchema()
{
  const QString srcName = KFileDialog::getOpenFileName(KUrl(),
                                    QString::fromLatin1("*.kateschema|%1").arg(i18n("Kate color schema")),
                                    this, i18n("Importing Color Schema"));

  if (srcName.isEmpty()) return;

  // carete config + sanity check for full color schema
  KConfig cfg(srcName, KConfig::SimpleConfig);
  KConfigGroup schemaGroup(&cfg, "KateSchema");
  if (schemaGroup.readEntry("full schema", "false").toUpper() != "TRUE") {
    KMessageBox::sorry(this, i18n("The file does not contain a full color schema."),
                       i18n("Fileformat error"));
    return;
  }

  // read color schema name
  const QStringList highlightings = schemaGroup.readEntry("highlightings",QStringList());
  const QString fromSchemaName = schemaGroup.readEntry("schema", i18n("Name unspecified"));

  // request valid schema name
  const QString schemaName = requestSchemaName(fromSchemaName);
  if (schemaName.isEmpty()) {
    return;
  }

  // if the schema already exists, select it in the combo box
  if (schemaCombo->findData(schemaName) != -1) {
    schemaCombo->setCurrentIndex(schemaCombo->findData(schemaName));
  }
  else { // it is really a new schema, easy meat :-)
    newSchema(schemaName);
  }

  // make sure the correct schema is activated
  schemaChanged(schemaName);


  // Finally, the correct schema is activated.
  // Next,  start importing.
  kDebug(13030) << "Importing schema: " << schemaName;

  //
  // import editor Colors (background, ...)
  //
  KConfigGroup colorConfigGroup(&cfg, "Editor Colors");
  m_colorTab->importSchema(colorConfigGroup);

  //
  // import font
  //
  m_fontTab->importSchema(schemaGroup);

  //
  // import Default Styles
  //
  m_defaultStylesTab->importSchema(fromSchemaName, schemaName, &cfg);

  //
  // import all Highlighting Text Styles
  //
  // create mapping from highlighting name to internal id
  const int hlCount = KateHlManager::self()->highlights();
  QHash<QString, int> nameToId;
  for(int i = 0; i < hlCount; ++i) {
    nameToId.insert(KateHlManager::self()->hlName(i),i);
  }

  // may take some time, as we have > 200 highlightings
  int cnt = 0;
  QProgressDialog progress(i18n("Importing schema"), QString(), 0, highlightings.count(), this);
  progress.setWindowModality(Qt::WindowModal);
  foreach(const QString& hl,highlightings) {
    if (nameToId.contains(hl)) {
      const int i = nameToId[hl];
      m_highlightTab->importHl(fromSchemaName, schemaName, i, &cfg);
      kDebug(13030) << "hl imported:" << hl;
    } else {
      kDebug(13030) << "could not import hl, hl unknown:" << hl;
    }
    progress.setValue(++cnt);
  }
  progress.setValue(highlightings.count());
}

void KateSchemaConfigPage::apply()
{
  // remember name + index
  const QString schemaName = schemaCombo->itemData (schemaCombo->currentIndex()).toString();

  // first apply all tabs
  m_colorTab->apply();
  m_fontTab->apply();
  m_defaultStylesTab->apply ();
  m_highlightTab->apply ();

  // just sync the config and reload
  KateGlobal::self()->schemaManager()->config ().sync();
  KateGlobal::self()->schemaManager()->config ().reparseConfiguration();

  // clear all attributes
  for (int i = 0; i < KateHlManager::self()->highlights(); ++i)
    KateHlManager::self()->getHl (i)->clearAttributeArrays();

  // than reload the whole stuff
  KateRendererConfig::global()->setSchema (defaultSchemaCombo->itemData (defaultSchemaCombo->currentIndex()).toString());
  KateRendererConfig::global()->reloadSchema();

  // sync the hl config for real
  KateHlManager::self()->getKConfig()->sync ();

  // KateSchemaManager::update() sorts the schema alphabetically, hence the
  // schema indexes change. Thus, repopulate the schema list...
  refillCombos(schemaCombo->itemData (schemaCombo->currentIndex()).toString(), defaultSchemaCombo->itemData (defaultSchemaCombo->currentIndex()).toString());
  schemaChanged(schemaName);
}

void KateSchemaConfigPage::reload()
{
  // now reload the config from disc
  KateGlobal::self()->schemaManager()->config ().reparseConfiguration();

  // reinitialize combo boxes
  refillCombos(KateRendererConfig::global()->schema(), KateRendererConfig::global()->schema());

  // finally, activate the current schema again
  schemaChanged(schemaCombo->itemData(schemaCombo->currentIndex()).toString());

  // all tabs need to reload to discard all the cached data, as the index
  // mapping may have changed
  m_colorTab->reload ();
  m_fontTab->reload ();
  m_defaultStylesTab->reload ();
  m_highlightTab->reload ();
}

void KateSchemaConfigPage::refillCombos(const QString& schemaName, const QString& defaultSchemaName)
{
  schemaCombo->blockSignals(true);
  defaultSchemaCombo->blockSignals(true);

  // reinitialize combo boxes
  schemaCombo->clear();
  defaultSchemaCombo->clear();
  QList<KateSchema> schemaList = KateGlobal::self()->schemaManager()->list();
  foreach (const KateSchema& s, schemaList) {
    schemaCombo->addItem(s.translatedName(), s.rawName);
    defaultSchemaCombo->addItem(s.translatedName(), s.rawName);
  }

  // set the correct indexes again, fallback to always existing "Normal"
  int schemaIndex = schemaCombo->findData (schemaName);
  if (schemaIndex == -1)
    schemaIndex = schemaCombo->findData ("Normal");
  
  int defaultSchemaIndex = defaultSchemaCombo->findData (defaultSchemaName);
  if (defaultSchemaIndex == -1)
    defaultSchemaIndex = defaultSchemaCombo->findData ("Normal");

  Q_ASSERT(schemaIndex != -1);
  Q_ASSERT(defaultSchemaIndex != -1);

  defaultSchemaCombo->setCurrentIndex (defaultSchemaIndex);
  schemaCombo->setCurrentIndex (schemaIndex);

  schemaCombo->blockSignals(false);
  defaultSchemaCombo->blockSignals(false);
}

void KateSchemaConfigPage::reset()
{
  reload ();
}

void KateSchemaConfigPage::defaults()
{
  reload ();
}

void KateSchemaConfigPage::deleteSchema ()
{
  const int comboIndex = schemaCombo->currentIndex ();
  const QString schemaNameToDelete = schemaCombo->itemData (comboIndex).toString();
  
  if (KateGlobal::self()->schemaManager()->schemaData(schemaNameToDelete).shippedDefaultSchema) {
    // Default and Printing schema cannot be deleted.
    kDebug(13030) << "default and printing schema cannot be deleted";
    return;
  }
  
  // kill group
  KateGlobal::self()->schemaManager()->config().deleteGroup (schemaNameToDelete);

  // fallback to Default schema
  schemaCombo->setCurrentIndex(schemaCombo->findData (QVariant ("Normal")));
  if (defaultSchemaCombo->currentIndex() == defaultSchemaCombo->findData (schemaNameToDelete))
    defaultSchemaCombo->setCurrentIndex(defaultSchemaCombo->findData (QVariant ("Normal")));

  // remove schema from combo box
  schemaCombo->removeItem(comboIndex);
  defaultSchemaCombo->removeItem(comboIndex);

  // Reload the color tab, since it uses cached schemas
  m_colorTab->reload();
}

bool KateSchemaConfigPage::newSchema (const QString& newName)
{
  // get sane name
  QString schemaName(newName);
  if (newName.isEmpty()) {
    bool ok = false;
    schemaName = KInputDialog::getText (i18n("Name for New Schema"), i18n ("Name:"), i18n("New Schema"), &ok, this);
    if (!ok) return false;
  }

  // try if schema already around
  if (KateGlobal::self()->schemaManager()->schema (schemaName).exists()) {
    KMessageBox::information(this, i18n("<p>The schema %1 already exists.</p><p>Please choose a different schema name.</p>", schemaName), i18n("New Schema"));
    return false;
  }

  // append items to combo boxes
  schemaCombo->addItem(schemaName, QVariant (schemaName));
  defaultSchemaCombo->addItem(schemaName, QVariant (schemaName));

  // finally, activate new schema (last item in the list)
  schemaCombo->setCurrentIndex(schemaCombo->count() - 1);

  return true;
}

void KateSchemaConfigPage::schemaChanged (const QString &schema)
{
  btndel->setEnabled( !KateGlobal::self()->schemaManager()->schemaData(schema).shippedDefaultSchema );

  // propagate changed schema to all tabs
  m_colorTab->schemaChanged(schema);
  m_fontTab->schemaChanged(schema);
  m_defaultStylesTab->schemaChanged(schema);
  m_highlightTab->schemaChanged(schema);

  // save current schema index
  m_currentSchema = schema;
}

void KateSchemaConfigPage::comboBoxIndexChanged (int currentIndex)
{
  schemaChanged (schemaCombo->itemData (currentIndex).toString());
}
//END KateSchemaConfigPage

// kate: space-indent on; indent-width 2; replace-tabs on;
