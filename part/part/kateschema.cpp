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

#include <klocale.h>
#include <kdialogbase.h>
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

#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <q3ptrcollection.h>
#include <qdialog.h>
#include <q3grid.h>
#include <q3groupbox.h>
#include <qlabel.h>
#include <qtextcodec.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <q3header.h>
#include <q3listbox.h>
#include <qpainter.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <q3vbox.h>
#include <QPolygon>
//END

//BEGIN KateStyleListViewItem decl
/*
    QListViewItem subclass to display/edit a style, bold/italic is check boxes,
    normal and selected colors are boxes, which will display a color chooser when
    activated.
    The context name for the style will be drawn using the editor default font and
    the chosen colors.
    This widget id designed to handle the default as well as the individual hl style
    lists.
    This widget is designed to work with the KateStyleListView class exclusively.
    Added by anders, jan 23 2002.
*/
class KateStyleListItem : public Q3ListViewItem
{
  public:
    KateStyleListItem( Q3ListViewItem *parent=0, const QString & stylename=0,
                   class KTextEditor::Attribute* defaultstyle=0, class KateHlItemData *data=0 );
    KateStyleListItem( Q3ListView *parent, const QString & stylename=0,
                   class KTextEditor::Attribute* defaultstyle=0, class KateHlItemData *data=0 );
    ~KateStyleListItem() { if (st) delete is; };

    /* mainly for readability */
    enum Property { ContextName, Bold, Italic, TextUnderline, Strikeout, Color, SelColor, BgColor, SelBgColor, UseDefStyle };

    /* initializes the style from the default and the hldata */
    void initStyle();
    /* updates the hldata's style */
    void updateStyle();
    /* reimp */
    virtual int width ( const QFontMetrics & fm, const Q3ListView * lv, int c ) const;
    /* calls changeProperty() if it makes sense considering pos. */
    void activate( int column, const QPoint &localPos );
    /* For bool fields, toggles them, for color fields, display a color chooser */
    void changeProperty( Property p );
    /** unset a color.
     * c is 100 (BGColor) or 101 (SelectedBGColor) for now.
     */
    void unsetColor( int c );
    /* style context name */
    QString contextName() { return text(0); };
    /* only true for a hl mode item using it's default style */
    bool defStyle();
    /* true for default styles */
    bool isDefault();
    /* whichever style is active (st for hl mode styles not using
       the default style, ds otherwise) */
    class KTextEditor::Attribute* style() { return is; };

  protected:
    /* reimp */
    void paintCell(QPainter *p, const QColorGroup& cg, int col, int width, int align);

  private:
    /* private methods to change properties */
    void toggleDefStyle();
    void setColor( int );
    /* helper function to copy the default style into the KateHlItemData,
       when a property is changed and we are using default style. */

    class KTextEditor::Attribute *is, // the style currently in use
              *ds;           // default style for hl mode contexts and default styles
    class KateHlItemData *st;      // itemdata for hl mode contexts
};
//END

//BEGIN KateStyleListCaption decl
/*
    This is a simple subclass for drawing the language names in a nice treeview
    with the styles.  It is needed because we do not like to mess with the default
    palette of the containing ListView.  Only the paintCell method is overwritten
    to use our own palette (that is set on the viewport rather than on the listview
    itself).
*/
class KateStyleListCaption : public Q3ListViewItem
{
  public:
    KateStyleListCaption( Q3ListView *parent, const QString & name );
    ~KateStyleListCaption() {};

  protected:
    void paintCell(QPainter *p, const QColorGroup& cg, int col, int width, int align);
};
//END

//BEGIN KateSchemaManager
QString KateSchemaManager::normalSchema ()
{
  return KApplication::kApplication()->aboutData()->appName () + QString (" - Normal");
}

QString KateSchemaManager::printingSchema ()
{
  return KApplication::kApplication()->aboutData()->appName () + QString (" - Printing");
}

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

  m_schemas.remove (printingSchema());
  m_schemas.remove (normalSchema());
  m_schemas.prepend (printingSchema());
  m_schemas.prepend (normalSchema());
}

//
// get the right group
// special handling of the default schemas ;)
//
KConfig *KateSchemaManager::schema (uint number)
{
  if ((number>1) && (number < (uint)m_schemas.count()))
    m_config.setGroup (m_schemas[number]);
  else if (number == 1)
    m_config.setGroup (printingSchema());
  else
    m_config.setGroup (normalSchema());

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
  if ((i = m_schemas.findIndex(name)) > -1)
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
KateSchemaConfigColorTab::KateSchemaConfigColorTab( QWidget *parent, const char * )
  : QWidget (parent)
{
  m_schema = -1;

  KHBox *b;
  QLabel *label;

  QVBoxLayout *blay=new QVBoxLayout(this, 0, KDialog::spacingHint());

  QGroupBox *gbTextArea = new Q3GroupBox(1, Qt::Horizontal, i18n("Text Area Background"), this);

  b = new KHBox (gbTextArea);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Normal text:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_back = new KColorButton(b);

  b = new KHBox (gbTextArea);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Selected text:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_selected = new KColorButton(b);

  b = new KHBox (gbTextArea);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Current line:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_current = new KColorButton(b);

  // Markers from kdelibs/interfaces/ktextinterface/markinterface.h
  b = new KHBox (gbTextArea);
  b->setSpacing(KDialog::spacingHint());
  m_combobox = new KComboBox(b);
  // add the predefined mark types as defined in markinterface.h
  m_combobox->addItem(i18n("Bookmark"));            // markType01
  m_combobox->addItem(i18n("Active Breakpoint"));   // markType02
  m_combobox->addItem(i18n("Reached Breakpoint"));  // markType03
  m_combobox->addItem(i18n("Disabled Breakpoint")); // markType04
  m_combobox->addItem(i18n("Execution"));           // markType05
  m_combobox->addItem(i18n("Warning"));             // markType06
  m_combobox->addItem(i18n("Error"));               // markType07
  m_combobox->setCurrentItem(0);
  m_markers = new KColorButton(b);
  connect( m_combobox, SIGNAL( activated( int ) ), SLOT( slotComboBoxChanged( int ) ) );

  blay->addWidget(gbTextArea);

  QGroupBox *gbBorder = new Q3GroupBox(1, Qt::Horizontal, i18n("Additional Elements"), this);

  b = new KHBox (gbBorder);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Left border background:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_iconborder = new KColorButton(b);

  b = new KHBox (gbBorder);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Line numbers:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_linenumber = new KColorButton(b);

  b = new KHBox (gbBorder);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Bracket highlight:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_bracket = new KColorButton(b);

  b = new KHBox (gbBorder);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Word wrap markers:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_wwmarker = new KColorButton(b);

  b = new KHBox (gbBorder);
  b->setSpacing(KDialog::spacingHint());
  label = new QLabel( i18n("Tab markers:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_tmarker = new KColorButton(b);

  blay->addWidget(gbBorder);

  blay->addStretch();

  // connect signal changed(); changed is emitted by a ColorButton change!
  connect( this, SIGNAL( changed() ), parent->parentWidget(), SLOT( slotChanged() ) );

  // QWhatsThis help
  m_back->setWhatsThis(i18n("<p>Sets the background color of the editing area.</p>"));
  m_selected->setWhatsThis(i18n("<p>Sets the background color of the selection.</p>"
        "<p>To set the text color for selected text, use the \"<b>Configure "
        "Highlighting</b>\" dialog.</p>"));
  m_markers->setWhatsThis(i18n("<p>Sets the background color of the selected "
        "marker type.</p><p><b>Note</b>: The marker color is displayed lightly because "
        "of transparency.</p>"));
  m_combobox->setWhatsThis(i18n("<p>Select the marker type you want to change.</p>"));
  m_current->setWhatsThis(i18n("<p>Sets the background color of the currently "
        "active line, which means the line where your cursor is positioned.</p>"));
  m_linenumber->setWhatsThis(i18n(
        "<p>This color will be used to draw the line numbers (if enabled) and the "
        "lines in the code-folding pane.</p>" ) );
  m_bracket->setWhatsThis(i18n("<p>Sets the bracket matching color. This means, "
        "if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will "
        "be highlighted with this color.</p>"));
  m_wwmarker->setWhatsThis(i18n(
        "<p>Sets the color of Word Wrap-related markers:</p>"
        "<dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where "
        "text is going to be wrapped</dd>"
        "<dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of "
        "visually-wrapped lines</dd></dl>"));
  m_tmarker->setWhatsThis(i18n(
        "<p>Sets the color of the tabulator marks:</p>"));
}

KateSchemaConfigColorTab::~KateSchemaConfigColorTab()
{
}

void KateSchemaConfigColorTab::schemaChanged ( int newSchema )
{
  // save curent schema
  if ( m_schema > -1 )
  {
    m_schemas[ m_schema ].back = m_back->color();
    m_schemas[ m_schema ].selected = m_selected->color();
    m_schemas[ m_schema ].current = m_current->color();
    m_schemas[ m_schema ].bracket = m_bracket->color();
    m_schemas[ m_schema ].wwmarker = m_wwmarker->color();
    m_schemas[ m_schema ].iconborder = m_iconborder->color();
    m_schemas[ m_schema ].tmarker = m_tmarker->color();
    m_schemas[ m_schema ].linenumber = m_linenumber->color();
  }

  if ( newSchema == m_schema ) return;

  // switch
  m_schema = newSchema;

  // first disconnect all signals otherwise setColor emits changed
  m_back      ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_selected  ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_current   ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_bracket   ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_wwmarker  ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_iconborder->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_tmarker   ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_markers   ->disconnect( SIGNAL( changed( const QColor & ) ) );
  m_linenumber->disconnect( SIGNAL( changed( const QColor & ) ) );

  // If we havent this schema, read in from config file
  if ( ! m_schemas.contains( newSchema ) )
  {
    // fallback defaults
    QColor tmp0 (KGlobalSettings::baseColor());
    QColor tmp1 (KGlobalSettings::highlightColor());
    QColor tmp2 (KGlobalSettings::alternateBackgroundColor());
    QColor tmp3 ( "#FFFF99" );
    QColor tmp4 (tmp2.dark());
    QColor tmp5 ( KGlobalSettings::textColor() );
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
    KConfig *config = KateGlobal::self()->schemaManager()->schema(newSchema);

    c.back= config->readColorEntry("Color Background", &tmp0);
    c.selected = config->readColorEntry("Color Selection", &tmp1);
    c.current = config->readColorEntry("Color Highlighted Line", &tmp2);
    c.bracket = config->readColorEntry("Color Highlighted Bracket", &tmp3);
    c.wwmarker = config->readColorEntry("Color Word Wrap Marker", &tmp4);
    c.tmarker = config->readColorEntry("Color Tab Marker", &tmp5);
    c.iconborder = config->readColorEntry("Color Icon Bar", &tmp6);
    c.linenumber = config->readColorEntry("Color Line Number", &tmp7);

    for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
      c.markerColors[i] =  config->readColorEntry( QString("Color MarkType%1").arg(i+1), &mark[i] );

     m_schemas[ newSchema ] = c;
  }

  m_back->setColor(  m_schemas[ newSchema ].back);
  m_selected->setColor(  m_schemas [ newSchema ].selected );
  m_current->setColor(  m_schemas [ newSchema ].current );
  m_bracket->setColor(  m_schemas [ newSchema ].bracket );
  m_wwmarker->setColor(  m_schemas [ newSchema ].wwmarker );
  m_tmarker->setColor(  m_schemas [ newSchema ].tmarker );
  m_iconborder->setColor(  m_schemas [ newSchema ].iconborder );
  m_linenumber->setColor(  m_schemas [ newSchema ].linenumber );

  // map from 0..reservedMarkersCount()-1 - the same index as in markInterface
  for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
  {
    QPixmap pix(16, 16);
    pix.fill( m_schemas [ newSchema ].markerColors[i]);
    m_combobox->changeItem(pix, m_combobox->text(i), i);
  }
  m_markers->setColor(  m_schemas [ newSchema ].markerColors[ m_combobox->currentItem() ] );

  connect( m_back      , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_selected  , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_current   , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_bracket   , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_wwmarker  , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_iconborder, SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_tmarker   , SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_linenumber, SIGNAL( changed( const QColor& ) ), SIGNAL( changed() ) );
  connect( m_markers   , SIGNAL( changed( const QColor& ) ), SLOT( slotMarkerColorChanged( const QColor& ) ) );
}

void KateSchemaConfigColorTab::apply ()
{
  schemaChanged( m_schema );
  QMap<int,SchemaColors>::Iterator it;
  for ( it =  m_schemas.begin(); it !=  m_schemas.end(); ++it )
  {
    kdDebug(13030)<<"APPLY scheme = "<<it.key()<<endl;
    KConfig *config = KateGlobal::self()->schemaManager()->schema( it.key() );
    kdDebug(13030)<<"Using config group "<<config->group()<<endl;
    SchemaColors c = it.data();

    config->writeEntry("Color Background", c.back);
    config->writeEntry("Color Selection", c.selected);
    config->writeEntry("Color Highlighted Line", c.current);
    config->writeEntry("Color Highlighted Bracket", c.bracket);
    config->writeEntry("Color Word Wrap Marker", c.wwmarker);
    config->writeEntry("Color Tab Marker", c.tmarker);
    config->writeEntry("Color Icon Bar", c.iconborder);
    config->writeEntry("Color Line Number", c.linenumber);

    for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
    {
      config->writeEntry(QString("Color MarkType%1").arg(i + 1), c.markerColors[i]);
    }
  }
}

void KateSchemaConfigColorTab::slotMarkerColorChanged( const QColor& color)
{
  int index = m_combobox->currentItem();
   m_schemas[ m_schema ].markerColors[ index ] = color;
  QPixmap pix(16, 16);
  pix.fill(color);
  m_combobox->changeItem(pix, m_combobox->text(index), index);

  emit changed();
}

void KateSchemaConfigColorTab::slotComboBoxChanged(int index)
{
  // temporarily disconnect the changed-signal because setColor emits changed as well
  m_markers->disconnect( SIGNAL( changed( const QColor& ) ) );
  m_markers->setColor( m_schemas[m_schema].markerColors[index] );
  connect( m_markers, SIGNAL( changed( const QColor& ) ), SLOT( slotMarkerColorChanged( const QColor& ) ) );
}

//END KateSchemaConfigColorTab

//BEGIN FontConfig -- 'Fonts' tab
KateSchemaConfigFontTab::KateSchemaConfigFontTab( QWidget *parent, const char * )
  : QWidget (parent)
{
    // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  m_fontchooser = new KFontChooser ( this, 0L, false, QStringList(), false );
  m_fontchooser->enableColumn(KFontChooser::StyleList, false);
  grid->addWidget( m_fontchooser, 0, 0);

  connect (this, SIGNAL( changed()), parent->parentWidget(), SLOT (slotChanged()));
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
    KateGlobal::self()->schemaManager()->schema( it.key() )->writeEntry( "Font", it.data() );
  }
}

void KateSchemaConfigFontTab::schemaChanged( int newSchema )
{
  if ( m_schema > -1 )
    m_fonts[ m_schema ] = m_fontchooser->font();

  m_schema = newSchema;

  QFont f (KGlobalSettings::fixedFont());

  m_fontchooser->disconnect ( this );
  m_fontchooser->setFont ( KateGlobal::self()->schemaManager()->schema( newSchema )->readFontEntry("Font", &f) );
  m_fonts[ newSchema ] = m_fontchooser->font();
  connect (m_fontchooser, SIGNAL (fontSelected( const QFont & )), this, SLOT (slotFontSelected( const QFont & )));
}
//END FontConfig

//BEGIN FontColorConfig -- 'Normal Text Styles' tab
KateSchemaConfigFontColorTab::KateSchemaConfigFontColorTab( QWidget *parent, const char * )
  : QWidget (parent)
{
  // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  m_defaultStyles = new KateStyleListView( this, false );
  grid->addWidget( m_defaultStyles, 0, 0);

  connect (m_defaultStyles, SIGNAL (changed()), parent->parentWidget(), SLOT (slotChanged()));

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
  p.setColor( QColorGroup::Base,
    KateGlobal::self()->schemaManager()->schema(schema)->
      readColorEntry( "Color Background", &_c ) );
  _c = KGlobalSettings::highlightColor();
  p.setColor( QColorGroup::Highlight,
    KateGlobal::self()->schemaManager()->schema(schema)->
      readColorEntry( "Color Selection", &_c ) );
  _c = l->at(0)->textColor(); // not quite as much of an assumption ;)
  p.setColor( QColorGroup::Text, _c );
  m_defaultStyles->viewport()->setPalette( p );

  // insert the default styles backwards to get them in the right order
  for ( int i = KateHlManager::self()->defaultStyles() - 1; i >= 0; i-- )
  {
    new KateStyleListItem( m_defaultStyles, KateHlManager::self()->defaultStyleName(i, true), l->at( i ) );
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
KateSchemaConfigHighlightTab::KateSchemaConfigHighlightTab( QWidget *parent, const char *, KateSchemaConfigFontColorTab *page, uint hl )
  : QWidget (parent)
{
  m_defaults = page;

  m_schema = 0;
  m_hl = 0;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  KHBox *hbHl = new KHBox( this );
  layout->add (hbHl);

  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), hbHl );
  hlCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );

  for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (KateHlManager::self()->hlSection(i).length() > 0)
      hlCombo->addItem(KateHlManager::self()->hlSection(i) + QString ("/") + KateHlManager::self()->hlNameTranslated(i));
    else
      hlCombo->addItem(KateHlManager::self()->hlNameTranslated(i));
  }
  hlCombo->setCurrentItem(0);

  // styles listview
  m_styles = new KateStyleListView( this, true );
  layout->addWidget (m_styles, 999);

  hlCombo->setCurrentItem ( hl );
  hlChanged ( hl );

  m_styles->setWhatsThis(i18n(
    "This list displays the contexts of the current syntax highlight mode and "
    "offers the means to edit them. The context name reflects the current "
    "style settings.<p>To edit using the keyboard, press "
    "<strong>&lt;SPACE&gt;</strong> and choose a property from the popup menu."
    "<p>To edit the colors, click the colored squares, or select the color "
    "to edit from the popup menu.<p>You can unset the Background and Selected "
    "Background colors from the context menu when appropriate.") );

  connect (m_styles, SIGNAL (changed()), parent->parentWidget(), SLOT (slotChanged()));
}

KateSchemaConfigHighlightTab::~KateSchemaConfigHighlightTab()
{
  QHashIterator<int, QHash<int, KateHlItemDataList*> > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    qDeleteAll(it.value());
  }
}

void KateSchemaConfigHighlightTab::hlChanged(int z)
{
  m_hl = z;

  schemaChanged (m_schema);
}

void KateSchemaConfigHighlightTab::schemaChanged (int schema)
{
  m_schema = schema;

  kdDebug(13030) << "NEW SCHEMA: " << m_schema << " NEW HL: " << m_hl << endl;

  m_styles->clear ();

  if (!m_hlDict.contains(m_schema))
  {
    kdDebug(13030) << "NEW SCHEMA, create dict" << endl;

    m_hlDict.insert (schema, QHash<int, KateHlItemDataList*>());
  }

  if (!m_hlDict[m_schema].contains(m_hl))
  {
    kdDebug(13030) << "NEW HL, create list" << endl;

    KateHlItemDataList *list = new KateHlItemDataList ();
    KateHlManager::self()->getHl( m_hl )->getKateHlItemDataListCopy (m_schema, *list);
    m_hlDict[m_schema].insert (m_hl, list);
  }

  KateAttributeList *l = m_defaults->attributeList (schema);

  // Set listview colors
  // We do that now, because we can now get the "normal text" color.
  // TODO this reads of the KConfig object, which should be changed when
  // the color tab is fixed.
  QPalette p ( m_styles->palette() );
  QColor _c ( KGlobalSettings::baseColor() );
  p.setColor( QColorGroup::Base,
    KateGlobal::self()->schemaManager()->schema(m_schema)->
      readColorEntry( "Color Background", &_c ) );
  _c = KGlobalSettings::highlightColor();
  p.setColor( QColorGroup::Highlight,
    KateGlobal::self()->schemaManager()->schema(m_schema)->
      readColorEntry( "Color Selection", &_c ) );
  _c = l->at(0)->textColor(); // not quite as much of an assumption ;)
  p.setColor( QColorGroup::Text, _c );
  m_styles->viewport()->setPalette( p );

  Q3Dict<KateStyleListCaption> prefixes;
  for ( KateHlItemData *itemData = m_hlDict[m_schema][m_hl]->last();
        itemData != 0L;
        itemData = m_hlDict[m_schema][m_hl]->prev())
  {
    kdDebug(13030) << "insert items " << itemData->name << endl;

    // All stylenames have their language mode prefixed, e.g. HTML:Comment
    // split them and put them into nice substructures.
    int c = itemData->name.find(':');
    if ( c > 0 ) {
      QString prefix = itemData->name.left(c);
      QString name   = itemData->name.mid(c+1);

      KateStyleListCaption *parent = prefixes.find( prefix );
      if ( ! parent )
      {
        parent = new KateStyleListCaption( m_styles, prefix );
        parent->setOpen(true);
        prefixes.insert( prefix, parent );
      }
      new KateStyleListItem( parent, name, l->at(itemData->defStyleNum), itemData );
    } else {
      new KateStyleListItem( m_styles, itemData->name, l->at(itemData->defStyleNum), itemData );
    }
  }
}

void KateSchemaConfigHighlightTab::reload ()
{
  m_styles->clear ();

  QHashIterator<int, QHash<int, KateHlItemDataList*> > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    qDeleteAll(it.value());
  }
  m_hlDict.clear ();

  hlChanged (0);
}

void KateSchemaConfigHighlightTab::apply ()
{
  QHashIterator<int, QHash<int, KateHlItemDataList*> > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    QHashIterator<int, KateHlItemDataList*> it2 = it.value();
    while (it2.hasNext()) {
      it2.next();
      KateHlManager::self()->getHl( it2.key() )->setKateHlItemDataList (it.key(), *(it2.value()));
    }
  }
}

//END KateSchemaConfigHighlightTab

//BEGIN KateSchemaConfigPage -- Main dialog page
KateSchemaConfigPage::KateSchemaConfigPage( QWidget *parent, KateDocument *doc )
  : KateConfigPage( parent ),
    m_lastSchema (-1)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  KHBox *hbHl = new KHBox( this );
  layout->add (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("&Schema:"), hbHl );
  schemaCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( schemaCombo );
  connect( schemaCombo, SIGNAL(activated(int)),
           this, SLOT(schemaChanged(int)) );

  QPushButton *btnnew = new QPushButton( i18n("&New..."), hbHl );
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newSchema()) );

  btndel = new QPushButton( i18n("&Delete"), hbHl );
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteSchema()) );

  m_tabWidget = new QTabWidget ( this );
  m_tabWidget->setMargin (KDialog::marginHint());
  layout->add (m_tabWidget);

  connect (m_tabWidget, SIGNAL (currentChanged (QWidget *)), this, SLOT (newCurrentPage (QWidget *)));

  m_colorTab = new KateSchemaConfigColorTab (m_tabWidget);
  m_tabWidget->addTab (m_colorTab, i18n("Colors"));

  m_fontTab = new KateSchemaConfigFontTab (m_tabWidget);
  m_tabWidget->addTab (m_fontTab, i18n("Font"));

  m_fontColorTab = new KateSchemaConfigFontColorTab (m_tabWidget);
  m_tabWidget->addTab (m_fontColorTab, i18n("Normal Text Styles"));

  uint hl = doc ? doc->hlMode() : 0;
  m_highlightTab = new KateSchemaConfigHighlightTab (m_tabWidget, "", m_fontColorTab, hl );
  m_tabWidget->addTab (m_highlightTab, i18n("Highlighting Text Styles"));

  hbHl = new KHBox( this );
  layout->add (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  lHl = new QLabel( i18n("&Default schema for %1:").arg(KApplication::kApplication()->aboutData()->programName ()), hbHl );
  defaultSchemaCombo = new QComboBox( false, hbHl );
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
  KateGlobal::self()->schemaManager()->schema (0)->sync();

  KateGlobal::self()->schemaManager()->update ();

  // clear all attributes
  for (int i = 0; i < KateHlManager::self()->highlights(); ++i)
    KateHlManager::self()->getHl (i)->clearAttributeArrays ();

  // than reload the whole stuff
  KateRendererConfig::global()->setSchema (defaultSchemaCombo->currentItem());
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

  defaultSchemaCombo->setCurrentItem (KateRendererConfig::global()->schema());

  // initialize to the schema in the current document, or default schema
  schemaCombo->setCurrentItem( m_defaultSchema );
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
  schemaCombo->insertStringList (KateGlobal::self()->schemaManager()->list ());

  defaultSchemaCombo->clear ();
  defaultSchemaCombo->insertStringList (KateGlobal::self()->schemaManager()->list ());

  schemaCombo->setCurrentItem (0);
  schemaChanged (0);

  schemaCombo->setEnabled (schemaCombo->count() > 0);
}

void KateSchemaConfigPage::deleteSchema ()
{
  int t = schemaCombo->currentItem ();

  KateGlobal::self()->schemaManager()->removeSchema (t);

  update ();
}

void KateSchemaConfigPage::newSchema ()
{
  QString t = KInputDialog::getText (i18n("Name for New Schema"), i18n ("Name:"), i18n("New Schema"), 0, this);

  KateGlobal::self()->schemaManager()->addSchema (t);

  // soft update, no load from disk
  KateGlobal::self()->schemaManager()->update (false);
  int i = KateGlobal::self()->schemaManager()->list ().findIndex (t);

  update ();
  if (i > -1)
  {
    schemaCombo->setCurrentItem (i);
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
  int count = KateGlobal::self()->schemaManager()->list().count();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateGlobal::self()->schemaManager()->list().operator[](z);

    if (!names.contains(hlName))
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
//END SCHEMA ACTION

//BEGIN KateStyleListView
KateStyleListView::KateStyleListView( QWidget *parent, bool showUseDefaults )
    : Q3ListView( parent )
{
  setSorting( -1 ); // disable sorting, let the styles appear in their defined order
  addColumn( i18n("Context") );
  addColumn( SmallIconSet("text_bold"), QString::null );
  addColumn( SmallIconSet("text_italic"), QString::null );
  addColumn( SmallIconSet("text_under"), QString::null );
  addColumn( SmallIconSet("text_strike"), QString::null );
  addColumn( i18n("Normal") );
  addColumn( i18n("Selected") );
  addColumn( i18n("Background") );
  addColumn( i18n("Background Selected") );
  if ( showUseDefaults )
    addColumn( i18n("Use Default Style") );
  connect( this, SIGNAL(mouseButtonPressed(int, Q3ListViewItem*, const QPoint&, int)),
           this, SLOT(slotMousePressed(int, Q3ListViewItem*, const QPoint&, int)) );
  connect( this, SIGNAL(contextMenuRequested(Q3ListViewItem*,const QPoint&, int)),
           this, SLOT(showPopupMenu(Q3ListViewItem*, const QPoint&)) );
  // grap the bg color, selected color and default font
  normalcol = KGlobalSettings::textColor();
  bgcol = KateRendererConfig::global()->backgroundColor();
  selcol = KateRendererConfig::global()->selectionColor();
  docfont = *KateRendererConfig::global()->font();

  viewport()->setPaletteBackgroundColor( bgcol );
}

void KateStyleListView::showPopupMenu( KateStyleListItem *i, const QPoint &globalPos, bool showtitle )
{
  if ( !dynamic_cast<KateStyleListItem*>(i) ) return;

  KMenu m( this );
  KTextEditor::Attribute *is = i->style();
  int id;
  // the title is used, because the menu obscures the context name when
  // displayed on behalf of spacePressed().
  QPixmap cl(16,16);
  cl.fill( i->style()->textColor() );
  QPixmap scl(16,16);
  scl.fill( i->style()->selectedTextColor() );
  QPixmap bgcl(16,16);
  bgcl.fill( i->style()->itemSet(KTextEditor::Attribute::BGColor) ? i->style()->bgColor() : viewport()->colorGroup().base() );
  QPixmap sbgcl(16,16);
  sbgcl.fill( i->style()->itemSet(KTextEditor::Attribute::SelectedBGColor) ? i->style()->selectedBGColor() : viewport()->colorGroup().base() );

  if ( showtitle )
    m.addTitle( i->contextName() );
  id = m.insertItem( i18n("&Bold"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Bold );
  m.setItemChecked( id, is->bold() );
  id = m.insertItem( i18n("&Italic"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Italic );
  m.setItemChecked( id, is->italic() );
  id = m.insertItem( i18n("&Underline"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::TextUnderline );
  m.setItemChecked( id, is->underline() );
  id = m.insertItem( i18n("S&trikeout"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Strikeout );
  m.setItemChecked( id, is->strikeOut() );

  m.insertSeparator();

  m.insertItem( QIcon(cl), i18n("Normal &Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Color );
  m.insertItem( QIcon(scl), i18n("&Selected Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::SelColor );
  m.insertItem( QIcon(bgcl), i18n("&Background Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::BgColor );
  m.insertItem( QIcon(sbgcl), i18n("S&elected Background Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::SelBgColor );

  // Unset [some] colors. I could show one only if that button was clicked, but that
  // would disable setting this with the keyboard (how many aren't doing just
  // that every day? ;)
  // ANY ideas for doing this in a nicer way will be warmly wellcomed.
  KTextEditor::Attribute *style = i->style();
  if ( style->itemSet( KTextEditor::Attribute::BGColor) || style->itemSet( KTextEditor::Attribute::SelectedBGColor ) )
  {
    m.insertSeparator();
    if ( style->itemSet( KTextEditor::Attribute::BGColor) )
      m.insertItem( i18n("Unset Background Color"), this, SLOT(unsetColor(int)), 0, 100 );
    if ( style->itemSet( KTextEditor::Attribute::SelectedBGColor ) )
      m.insertItem( i18n("Unset Selected Background Color"), this, SLOT(unsetColor(int)), 0, 101 );
  }

  if ( ! i->isDefault() && ! i->defStyle() ) {
    m.insertSeparator();
    id = m.insertItem( i18n("Use &Default Style"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::UseDefStyle );
    m.setItemChecked( id, i->defStyle() );
  }
  m.exec( globalPos );
}

void KateStyleListView::showPopupMenu( Q3ListViewItem *i, const QPoint &pos )
{
  if ( dynamic_cast<KateStyleListItem*>(i) )
    showPopupMenu( (KateStyleListItem*)i, pos, true );
}

void KateStyleListView::mSlotPopupHandler( int z )
{
  ((KateStyleListItem*)currentItem())->changeProperty( (KateStyleListItem::Property)z );
}

void KateStyleListView::unsetColor( int c )
{
  ((KateStyleListItem*)currentItem())->unsetColor( c );
  emitChanged();
}

// Because QListViewItem::activatePos() is going to become deprecated,
// and also because this attempt offers more control, I connect mousePressed to this.
void KateStyleListView::slotMousePressed(int btn, Q3ListViewItem* i, const QPoint& pos, int c)
{
  if ( dynamic_cast<KateStyleListItem*>(i) ) {
     if ( btn == Qt::LeftButton && c > 0 ) {
      // map pos to item/column and call KateStyleListItem::activate(col, pos)
      ((KateStyleListItem*)i)->activate( c, viewport()->mapFromGlobal( pos ) - QPoint( 0, itemRect(i).top() ) );
    }
  }
}

//END

//BEGIN KateStyleListItem
static const int BoxSize = 16;
static const int ColorBtnWidth = 32;

KateStyleListItem::KateStyleListItem( Q3ListViewItem *parent, const QString & stylename,
                              KTextEditor::Attribute *style, KateHlItemData *data )
        : Q3ListViewItem( parent, stylename ),
          ds( style ),
          st( data )
{
  initStyle();
}

KateStyleListItem::KateStyleListItem( Q3ListView *parent, const QString & stylename,
                              KTextEditor::Attribute *style, KateHlItemData *data )
        : Q3ListViewItem( parent, stylename ),
          ds( style ),
          st( data )
{
  initStyle();
}

void KateStyleListItem::initStyle()
{
  if (!st)
    is = ds;
  else
  {
    is = new KTextEditor::Attribute (*ds);

    if (st->isSomethingSet())
      *is += *st;
  }
}

void KateStyleListItem::updateStyle()
{
  // nothing there, not update it, will crash
  if (!st)
    return;

  if ( is->itemSet(KTextEditor::Attribute::Weight) )
  {
    if ( is->weight() != st->weight())
      st->setWeight( is->weight() );
  }

  if ( is->itemSet(KTextEditor::Attribute::Italic) )
  {
    if ( is->italic() != st->italic())
      st->setItalic( is->italic() );
  }

  if ( is->itemSet(KTextEditor::Attribute::StrikeOut) )
  {
    if ( is->strikeOut() != st->strikeOut())

      st->setStrikeOut( is->strikeOut() );
  }

  if ( is->itemSet(KTextEditor::Attribute::Underline) )
  {
    if ( is->underline() != st->underline())
      st->setUnderline( is->underline() );
  }

  if ( is->itemSet(KTextEditor::Attribute::Outline) )
  {
    if ( is->outline() != st->outline())
      st->setOutline( is->outline() );
  }

  if ( is->itemSet(KTextEditor::Attribute::TextColor) )
  {
    if ( is->textColor() != st->textColor())
      st->setTextColor( is->textColor() );
  }

  if ( is->itemSet(KTextEditor::Attribute::SelectedTextColor) )
  {
    if ( is->selectedTextColor() != st->selectedTextColor())
      st->setSelectedTextColor( is->selectedTextColor() );
  }

  if ( is->itemSet(KTextEditor::Attribute::BGColor) )
  {
    if ( is->bgColor() != st->bgColor())
      st->setBGColor( is->bgColor() );
  }

  if ( is->itemSet(KTextEditor::Attribute::SelectedBGColor) )
  {
    if ( is->selectedBGColor() != st->selectedBGColor())
      st->setSelectedBGColor( is->selectedBGColor() );
  }
}

/* only true for a hl mode item using it's default style */
bool KateStyleListItem::defStyle() { return st && st->itemsSet() != ds->itemsSet(); }

/* true for default styles */
bool KateStyleListItem::isDefault() { return st ? false : true; }

int KateStyleListItem::width( const QFontMetrics & /*fm*/, const Q3ListView * lv, int col ) const
{
  int m = lv->itemMargin() * 2;
  switch ( col ) {
    case ContextName:
      // FIXME: width for name column should reflect bold/italic
      // (relevant for non-fixed fonts only - nessecary?)
      return Q3ListViewItem::width( QFontMetrics( ((KateStyleListView*)lv)->docfont), lv, col);
    case Bold:
    case Italic:
    case UseDefStyle:
      return BoxSize + m;
    case Color:
    case SelColor:
    case BgColor:
    case SelBgColor:
      return ColorBtnWidth +m;
    default:
      return 0;
  }
}

void KateStyleListItem::activate( int column, const QPoint &localPos )
{
  Q3ListView *lv = listView();
  int x = 0;
  for( int c = 0; c < column-1; c++ )
    x += lv->columnWidth( c );
  int w;
  switch( column ) {
    case Bold:
    case Italic:
    case TextUnderline:
    case Strikeout:
    case UseDefStyle:
      w = BoxSize;
      break;
    case Color:
    case SelColor:
    case BgColor:
    case SelBgColor:
      w = ColorBtnWidth;
      break;
    default:
      return;
  }
  if ( !QRect( x, 0, w, BoxSize ).contains( localPos ) )
  changeProperty( (Property)column );
}

void KateStyleListItem::changeProperty( Property p )
{
  if ( p == Bold )
    is->setBold( ! is->bold() );
  else if ( p == Italic )
    is->setItalic( ! is->italic() );
  else if ( p == TextUnderline )
    is->setUnderline( ! is->underline() );
  else if ( p == Strikeout )
    is->setStrikeOut( ! is->strikeOut() );
  else if ( p == UseDefStyle )
    toggleDefStyle();
  else
    setColor( p );

  updateStyle ();

  ((KateStyleListView*)listView())->emitChanged();
}

void KateStyleListItem::toggleDefStyle()
{
  if ( *is == *ds ) {
    KMessageBox::information( listView(),
         i18n("\"Use Default Style\" will be automatically unset when you change any style properties."),
         i18n("Kate Styles"),
         "Kate hl config use defaults" );
  }
  else {
    delete is;
    is = new KTextEditor::Attribute( *ds );
    repaint();
  }
}

void KateStyleListItem::setColor( int column )
{
  QColor c; // use this
  QColor d; // default color
  if ( column == Color)
  {
    c = is->textColor();
    d = ds->textColor();
  }
  else if ( column == SelColor )
  {
    c = is->selectedTextColor();
    d = is->selectedTextColor();
  }
  else if ( column == BgColor )
  {
    c = is->bgColor();
    d = ds->bgColor();
  }
  else if ( column == SelBgColor )
  {
    c = is->selectedBGColor();
    d = ds->selectedBGColor();
  }

  if ( KColorDialog::getColor( c, d, listView() ) != QDialog::Accepted) return;

  bool def = ! c.isValid();

  // if set default, and the attrib is set in the default style use it
  // else if set default, unset it
  // else set the selected color
  switch (column)
  {
    case Color:
      if ( def )
      {
        if ( ds->itemSet(KTextEditor::Attribute::TextColor) )
          is->setTextColor( ds->textColor());
        else
          is->clearAttribute(KTextEditor::Attribute::TextColor);
      }
      else
        is->setTextColor( c );
    break;
    case SelColor:
      if ( def )
      {
        if ( ds->itemSet(KTextEditor::Attribute::SelectedTextColor) )
          is->setSelectedTextColor( ds->selectedTextColor());
        else
          is->clearAttribute(KTextEditor::Attribute::SelectedTextColor);
      }
      else
        is->setSelectedTextColor( c );
    break;
    case BgColor:
      if ( def )
      {
        if ( ds->itemSet(KTextEditor::Attribute::BGColor) )
          is->setBGColor( ds->bgColor());
        else
          is->clearAttribute(KTextEditor::Attribute::BGColor);
      }
      else
        is->setBGColor( c );
    break;
    case SelBgColor:
      if ( def )
      {
        if ( ds->itemSet(KTextEditor::Attribute::SelectedBGColor) )
          is->setSelectedBGColor( ds->selectedBGColor());
        else
          is->clearAttribute(KTextEditor::Attribute::SelectedBGColor);
      }
      else
        is->setSelectedBGColor( c );
    break;
  }

  repaint();
}

void KateStyleListItem::unsetColor( int c )
{
  if ( c == 100 && is->itemSet(KTextEditor::Attribute::BGColor) )
    is->clearAttribute(KTextEditor::Attribute::BGColor);
  else if ( c == 101 && is->itemSet(KTextEditor::Attribute::SelectedBGColor) )
    is->clearAttribute(KTextEditor::Attribute::SelectedBGColor);
}

void KateStyleListItem::paintCell( QPainter *p, const QColorGroup& /*cg*/, int col, int width, int align )
{

  if ( !p )
    return;

  Q3ListView *lv = listView();
  if ( !lv )
    return;
  Q_ASSERT( lv ); //###

  // use a private color group and set the text/highlighted text colors
  QColorGroup mcg = lv->viewport()->colorGroup();

  if ( col ) // col 0 is drawn by the superclass method
    p->fillRect( 0, 0, width, height(), QBrush( mcg.base() ) );

  int marg = lv->itemMargin();

  QColor c;

  switch ( col )
  {
    case ContextName:
    {
      mcg.setColor(QColorGroup::Text, is->textColor());
      mcg.setColor(QColorGroup::HighlightedText, is->selectedTextColor());
      // text background color
      c = is->bgColor();
      if ( c.isValid() && is->itemSet(KTextEditor::Attribute::BGColor) )
        mcg.setColor( QColorGroup::Base, c );
      if ( isSelected() && is->itemSet(KTextEditor::Attribute::SelectedBGColor) )
      {
        c = is->selectedBGColor();
        if ( c.isValid() )
          mcg.setColor( QColorGroup::Highlight, c );
      }
      QFont f ( ((KateStyleListView*)lv)->docfont );
      p->setFont( is->font(f) );
      // FIXME - repainting when text is cropped, and the column is enlarged is buggy.
      // Maybe I need painting the string myself :(
      // (wilbert) it depends on the font used
      Q3ListViewItem::paintCell( p, mcg, col, width, align );
    }
    break;
    case Bold:
    case Italic:
    case TextUnderline:
    case Strikeout:
    case UseDefStyle:
    {
      // Bold/Italic/use default checkboxes
      // code allmost identical to QCheckListItem
      int x = 0;
      int y = (height() - BoxSize) / 2;

      if ( isEnabled() )
        p->setPen( QPen( mcg.text(), 2 ) );
      else
        p->setPen( QPen( lv->palette().color( QPalette::Disabled, QColorGroup::Text ), 2 ) );

      p->drawRect( x+marg, y+2, BoxSize-4, BoxSize-4 );
      x++;
      y++;
      if ( (col == Bold && is->bold()) ||
          (col == Italic && is->italic()) ||
          (col == TextUnderline && is->underline()) ||
          (col == Strikeout && is->strikeOut()) ||
          (col == UseDefStyle && *is == *ds ) )
      {
        QPolygon a( 7*2 );
        int i, xx, yy;
        xx = x+1+marg;
        yy = y+5;
        for ( i=0; i<3; i++ ) {
          a.setPoint( 2*i,   xx, yy );
          a.setPoint( 2*i+1, xx, yy+2 );
          xx++; yy++;
        }
        yy -= 2;
        for ( i=3; i<7; i++ ) {
          a.setPoint( 2*i,   xx, yy );
          a.setPoint( 2*i+1, xx, yy+2 );
          xx++; yy--;
        }
        p->drawLineSegments( a );
      }
    }
    break;
    case Color:
    case SelColor:
    case BgColor:
    case SelBgColor:
    {
      bool set( false );
      if ( col == Color)
      {
        c = is->textColor();
        set = is->itemSet(KTextEditor::Attribute::TextColor);
      }
      else if ( col == SelColor )
      {
        c = is->selectedTextColor();
        set = is->itemSet( KTextEditor::Attribute::SelectedTextColor);
      }
      else if ( col == BgColor )
      {
        set = is->itemSet(KTextEditor::Attribute::BGColor);
        c = set ? is->bgColor() : mcg.base();
      }
      else if ( col == SelBgColor )
      {
        set = is->itemSet(KTextEditor::Attribute::SelectedBGColor);
        c = set ? is->selectedBGColor(): mcg.base();
      }

      // color "buttons"
      int x = 0;
      int y = (height() - BoxSize) / 2;
      if ( isEnabled() )
        p->setPen( QPen( mcg.text(), 2 ) );
      else
        p->setPen( QPen( lv->palette().color( QPalette::Disabled, QColorGroup::Text ), 2 ) );

      p->drawRect( x+marg, y+2, ColorBtnWidth-4, BoxSize-4 );
      p->fillRect( x+marg+1,y+3,ColorBtnWidth-7,BoxSize-7,QBrush( c ) );
      // if this item is unset, draw a diagonal line over the button
      if ( ! set )
        p->drawLine( x+marg-1, BoxSize-3, ColorBtnWidth-4, y+1 );
    }
    //case default: // no warning...
  }
}
//END

//BEGIN KateStyleListCaption
KateStyleListCaption::KateStyleListCaption( Q3ListView *parent, const QString & name )
      :  Q3ListViewItem( parent, name )
{
}

void KateStyleListCaption::paintCell( QPainter *p, const QColorGroup& /*cg*/, int col, int width, int align )
{
  Q3ListView *lv = listView();
  if ( !lv )
    return;
  Q_ASSERT( lv ); //###

  // use the same colorgroup as the other items in the viewport
  QColorGroup mcg = lv->viewport()->colorGroup();

  Q3ListViewItem::paintCell( p, mcg, col, width, align );
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on;
