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

#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qtextcodec.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <QPolygon>
#include <QGroupBox>
#include <QTreeWidgetItem>
#include <QItemDelegate>
//END

//BEGIN KateStyleListItem decl
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
class KateStyleListItem : public QTreeWidgetItem
{
  public:
    KateStyleListItem( QTreeWidgetItem *parent, const QString& styleName, KTextEditor::Attribute* defaultstyle, KateExtendedAttribute* data = 0L );
    KateStyleListItem( QTreeWidget *parent, const QString& styleName, KTextEditor::Attribute* defaultstyle, KateExtendedAttribute* data = 0L );
    ~KateStyleListItem() { if (actualStyle) delete currentStyle; };

    enum columns {
      Context = 0,
      Bold,
      Italic,
      Underline,
      StrikeOut,
      Foreground,
      SelectedForeground,
      Background,
      SelectedBackground,
      UseDefaultStyle,
      NumColumns
    };

    /* initializes the style from the default and the hldata */
    void initStyle();
    /* updates the hldata's style */
    void updateStyle();
    /* calls changeProperty() if it makes sense considering pos. */
    void activate( int column, const QPoint &localPos );
    /* For bool fields, toggles them, for color fields, display a color chooser */
    void changeProperty( int p );
    /** unset a color.
     * c is 100 (BGColor) or 101 (SelectedBGColor) for now.
     */
    void unsetColor( int c );
    /* style context name */
    QString contextName() const { return text(0); };
    /* only true for a hl mode item using it's default style */
    bool defStyle() const;
    /* true for default styles */
    bool isDefault() const;
    /* whichever style is active (currentStyle for hl mode styles not using
       the default style, defaultStyle otherwise) */
    KTextEditor::Attribute* style() const { return currentStyle; };

    virtual QVariant data( int column, int role ) const;
    virtual void setData( int column, int role, const QVariant& value );

  private:
    /* private methods to change properties */
    void toggleDefStyle();
    void setColor( int );
    /* helper function to copy the default style into the KateExtendedAttribute,
       when a property is changed and we are using default style. */

    KTextEditor::Attribute *currentStyle, // the style currently in use (was "is")
                           *defaultStyle; // default style for hl mode contexts and default styles (was "ds")
    KateExtendedAttribute  *actualStyle;  // itemdata for hl mode contexts (was "st")
};
//END

// BEGIN KateStyleListDelegate
class KateStyleListDelegate : public QItemDelegate
{
  public:
    KateStyleListDelegate(QWidget* widget);

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

  private:
    QWidget* m_widget;
};
// END

#if 0
class KateAttributeGroup : public KateAttributeList
{
  public:
    uint index;
};

class KateStyleListModel : public QAbstractItemModel
{
  public:
    KateStyleListModel(QObject* parent)
      : QAbstractItemModel(parent)
      , m_defaultOnly(false)
      // FIXME
      , m_schemaIndex(0)
    {
    }

    virtual ~KateStyleListModel()
    {
      //qDeleteAll(m_defaultStyleLists);
    }

    KateAttributeList* listForIndex(const QModelIndex& index) const
    {
      /*foreach (const QPair<int, KateAttributeList*>& pair, m_defaultStyleLists) {
        if (pair.first == index.row())
          return pair.second;
      }

      KateAttributeList *list = new KateAttributeList();
      KateHlManager::self()->getDefaults(index.row(), *list);

      m_defaultStyleLists.append(qMakePair(index.row(), list));

      return list;*/
    }

    void setSchema(uint index)
    {
      m_schemaIndex = index;
      reset();
    }

    enum columns {
      Context = 0,
      Bold,
      Italic,
      Underline,
      StrikeOut,
      NormalColor,
      SelectedColor,
      BackgroundColor,
      BackgroundSelectedColor,
      UseDefaultStyle,
      NumColumns
    };

    virtual int columnCount(const QModelIndex&) const
    {
      return NumColumns;
    }

    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const
    {
      if (role == Qt::DisplayRole) {
        if (section >= Bold && section <= StrikeOut)
          return QVariant();

      } else if (role == Qt::DecorationRole) {
        if (section < Bold || section > StrikeOut)
          return QVariant();

      } else {
        return QVariant();
      }

      switch (section) {
        case Context:
          return i18n("Context");
        case Bold:
          return SmallIconSet("text_bold");
        case Italic:
          return SmallIconSet("text_italic");
        case Underline:
          return SmallIconSet("text_under");
        case StrikeOut:
          return SmallIconSet("text_strike");
        case NormalColor:
          return i18n("Normal");
        case SelectedColor:
          return i18n("Selected");
        case BackgroundColor:
          return i18n("Background");
        case BackgroundSelectedColor:
          return i18n("Background Selected");
        case UseDefaultStyle:
          return i18n("Use Default Style");
      }

      return QVariant();
    }

    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
    {
      if (role != Qt::DisplayRole)
        return QVariant();

      if (!m_defaultOnly || index.parent().isValid()) {
        KTextEditor::Attribute* a = static_cast<KTextEditor::Attribute*>(index.internalPointer());
        switch (index.column()) {
          case Context:
            return KateHlManager::self()->defaultStyleName(index.row(), true);
          case Bold:
            return a->fontBold();
          case Italic:
            return a->fontItalic();
          case Underline:
            return a->fontUnderline();
          case StrikeOut:
            return a->fontStrikeOut();
          case NormalColor:
            return a->foreground();
          case SelectedColor:
            return a->selectedForeground();
          case BackgroundColor:
            return a->background();
          case BackgroundSelectedColor:
            return a->selectedBackground();
          case UseDefaultStyle:
            return "FIXME-UseDefaultStyle";
        }
      } else {

      }

      return QVariant();
    }

    /**
     * The indexes are as follows:
     *
     * Needs rethink
     *
     * For the parents
     *   id = highlight index
     * For the children
     *   id = negative highlight index
     */
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const
    {
      if (!m_defaultOnly || parent.isValid())
        return createIndex(row, column, -1);

      return createIndex(row, column, 0);
    }

    virtual QModelIndex parent ( const QModelIndex & index ) const
    {
      if (!m_defaultOnly || index.row() == 0)
        return QModelIndex();

      return createIndex(index.row(), index.column(), -1);
    }

    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const
    {
      if (!m_defaultOnly || parent.isValid())
        return listForIndex(parent)->count();

      return 1;//schemaManager()->count();
    }

    virtual QSize span ( const QModelIndex & index ) const
    {
      if (index.parent().isValid())
        return QSize(1, 1);

      return QSize(NumColumns, 1);
    }

  private:
    // Only showing the default styles
    bool m_defaultOnly;

    int m_schemaIndex;
    int m_highlightIndex;

    QList< QPair<int,KateAttributeList*> > m_defaultStyleLists;

    QHash<int, QHash<int, KateExtendedAttributeList*> > m_hlDict;
};
#endif

//BEGIN KateStyleListCaption decl
/*
    This is a simple subclass for drawing the language names in a nice treeview
    with the styles.  It is needed because we do not like to mess with the default
    palette of the containing ListView.  Only the paintCell method is overwritten
    to use our own palette (that is set on the viewport rather than on the listview
    itself).
*/
class KateStyleListCaption : public QTreeWidgetItem
{
  public:
    KateStyleListCaption( QTreeWidget *parent, const QString & name );
    ~KateStyleListCaption() {};
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

  QGroupBox *gbTextArea = new QGroupBox(this);
  gbTextArea->setTitle(i18n("Text Area Background"));
  QVBoxLayout* vbLayout = new QVBoxLayout(gbTextArea);

  b = new KHBox (gbTextArea);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Normal text:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_back = new KColorButton(b);

  b = new KHBox (gbTextArea);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Selected text:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_selected = new KColorButton(b);

  b = new KHBox (gbTextArea);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Current line:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_current = new KColorButton(b);

  // Markers from kdelibs/interfaces/ktextinterface/markinterface.h
  b = new KHBox (gbTextArea);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
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

  QGroupBox *gbBorder = new QGroupBox(this);
  gbBorder->setTitle(i18n("Additional Elements"));
  vbLayout = new QVBoxLayout(gbBorder);

  b = new KHBox (gbBorder);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Left border background:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_iconborder = new KColorButton(b);

  b = new KHBox (gbBorder);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Line numbers:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_linenumber = new KColorButton(b);

  b = new KHBox (gbBorder);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Bracket highlight:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_bracket = new KColorButton(b);

  b = new KHBox (gbBorder);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
  label = new QLabel( i18n("Word wrap markers:"), b);
  label->setAlignment( Qt::AlignLeft|Qt::AlignVCenter);
  m_wwmarker = new KColorButton(b);

  b = new KHBox (gbBorder);
  vbLayout->addWidget(b);
  b->setSpacing(KDialog::spacingHint());
  b->setMargin(0);
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

    c.back= config->readEntry("Color Background", tmp0);
    c.selected = config->readEntry("Color Selection", tmp1);
    c.current = config->readEntry("Color Highlighted Line", tmp2);
    c.bracket = config->readEntry("Color Highlighted Bracket", tmp3);
    c.wwmarker = config->readEntry("Color Word Wrap Marker", tmp4);
    c.tmarker = config->readEntry("Color Tab Marker", tmp5);
    c.iconborder = config->readEntry("Color Icon Bar", tmp6);
    c.linenumber = config->readEntry("Color Line Number", tmp7);

    for (int i = 0; i < KTextEditor::MarkInterface::reservedMarkersCount(); i++)
      c.markerColors[i] =  config->readEntry( QString("Color MarkType%1").arg(i+1), mark[i] );

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

  m_fontchooser = new KFontChooser ( this, false, QStringList(), false );
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

  m_defaultStyles = new KateStyleListView( this );
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
      readEntry( "Color Background", _c ) );
  _c = KGlobalSettings::highlightColor();
  p.setColor( QColorGroup::Highlight,
    KateGlobal::self()->schemaManager()->schema(schema)->
      readEntry( "Color Selection", _c ) );
  _c = l->at(0)->foreground(); // not quite as much of an assumption ;)
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
  QHashIterator<int, QHash<int, KateExtendedAttributeList*> > it = m_hlDict;
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

    m_hlDict.insert (schema, QHash<int, KateExtendedAttributeList*>());
  }

  if (!m_hlDict[m_schema].contains(m_hl))
  {
    kdDebug(13030) << "NEW HL, create list" << endl;

    KateExtendedAttributeList *list = new KateExtendedAttributeList ();
    KateHlManager::self()->getHl( m_hl )->getKateExtendedAttributeListCopy (m_schema, *list);
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
      readEntry( "Color Background", _c ) );
  _c = KGlobalSettings::highlightColor();
  p.setColor( QColorGroup::Highlight,
    KateGlobal::self()->schemaManager()->schema(m_schema)->
      readEntry( "Color Selection", _c ) );
  _c = l->at(0)->foreground(); // not quite as much of an assumption ;)
  p.setColor( QColorGroup::Text, _c );
  m_styles->viewport()->setPalette( p );

  QHash<QString, KateStyleListCaption*> prefixes;
  KateExtendedAttributeList::ConstIterator it = m_hlDict[m_schema][m_hl]->end();
  while (it != m_hlDict[m_schema][m_hl]->begin())
  {
    --it;
    KateExtendedAttribute *itemData = *it;
    Q_ASSERT(itemData);

    kdDebug(13030) << "insert items " << itemData->name() << endl;

    // All stylenames have their language mode prefixed, e.g. HTML:Comment
    // split them and put them into nice substructures.
    int c = itemData->name().find(':');
    if ( c > 0 ) {
      QString prefix = itemData->name().left(c);
      QString name   = itemData->name().mid(c+1);

      KateStyleListCaption *parent = prefixes[prefix];
      if ( ! parent )
      {
        parent = new KateStyleListCaption( m_styles, prefix );
        m_styles->expandItem(parent);
        prefixes.insert( prefix, parent );
      }
      new KateStyleListItem( parent, name, l->at(itemData->defaultStyleIndex()), itemData );
    } else {
      new KateStyleListItem( m_styles, itemData->name(), l->at(itemData->defaultStyleIndex()), itemData );
    }
  }
}

void KateSchemaConfigHighlightTab::reload ()
{
  m_styles->clear ();

  QHashIterator<int, QHash<int, KateExtendedAttributeList*> > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    qDeleteAll(it.value());
  }
  m_hlDict.clear ();

  hlChanged (0);
}

void KateSchemaConfigHighlightTab::apply ()
{
  QHashIterator<int, QHash<int, KateExtendedAttributeList*> > it = m_hlDict;
  while (it.hasNext()) {
    it.next();
    QHashIterator<int, KateExtendedAttributeList*> it2 = it.value();
    while (it2.hasNext()) {
      it2.next();
      KateHlManager::self()->getHl( it2.key() )->setKateExtendedAttributeList (it.key(), *(it2.value()));
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
    KateHlManager::self()->getHl (i)->clearAttributeArrays();

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
    : QTreeWidget( parent )
{
  setItemDelegate(new KateStyleListDelegate(this));

  setColumnCount( showUseDefaults ? 10 : 9 );
  QStringList headers;
  headers << i18n("Context") << QString() << QString() << QString() << QString() << i18n("Normal") << i18n("Selected") << i18n("Background") << i18n("Background Selected") << i18n("Use Default Style");
  setHeaderLabels(headers);

  headerItem()->setIcon(1, SmallIconSet("text_bold"));
  headerItem()->setIcon(2, SmallIconSet("text_italic"));
  headerItem()->setIcon(3, SmallIconSet("text_under"));
  headerItem()->setIcon(4, SmallIconSet("text_strike"));

  connect( this, SIGNAL(mouseButtonPressed(int, QTreeWidgetItem*, const QPoint&, int)),
           this, SLOT(slotMousePressed(int, QTreeWidgetItem*, const QPoint&, int)) );
  connect( this, SIGNAL(contextMenuRequested(QTreeWidgetItem*,const QPoint&, int)),
           this, SLOT(showPopupMenu(QTreeWidgetItem*, const QPoint&)) );

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
  KTextEditor::Attribute *currentStyle = i->style();
  int id;
  // the title is used, because the menu obscures the context name when
  // displayed on behalf of spacePressed().
  QPixmap cl(16,16);
  cl.fill( i->style()->foreground() );
  QPixmap scl(16,16);
  scl.fill( i->style()->selectedForeground() );
  QPixmap bgcl(16,16);
  bgcl.fill( i->style()->hasProperty(QTextFormat::BackgroundBrush) ? i->style()->background().color() : viewport()->colorGroup().base() );
  QPixmap sbgcl(16,16);
  sbgcl.fill( i->style()->hasProperty(KTextEditor::Attribute::SelectedBackground) ? i->style()->selectedBackground().color() : viewport()->colorGroup().base() );

  if ( showtitle )
    m.addTitle( i->contextName() );
  id = m.insertItem( i18n("&Bold"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Bold );
  m.setItemChecked( id, currentStyle->fontBold() );
  id = m.insertItem( i18n("&Italic"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Italic );
  m.setItemChecked( id, currentStyle->fontItalic() );
  id = m.insertItem( i18n("&Underline"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Underline );
  m.setItemChecked( id, currentStyle->fontUnderline() );
  id = m.insertItem( i18n("S&trikeout"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::StrikeOut );
  m.setItemChecked( id, currentStyle->fontStrikeOut() );

  m.insertSeparator();

  m.insertItem( QIcon(cl), i18n("Normal &Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Foreground );
  m.insertItem( QIcon(scl), i18n("&Selected Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::SelectedForeground );
  m.insertItem( QIcon(bgcl), i18n("&Background Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::Background );
  m.insertItem( QIcon(sbgcl), i18n("S&elected Background Color..."), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::SelectedBackground );

  // Unset [some] colors. I could show one only if that button was clicked, but that
  // would disable setting this with the keyboard (how many aren't doing just
  // that every day? ;)
  // ANY ideas for doing this in a nicer way will be warmly wellcomed.
  KTextEditor::Attribute *style = i->style();
  if ( style->hasProperty( QTextFormat::BackgroundBrush) || style->hasProperty( KTextEditor::Attribute::SelectedBackground ) )
  {
    m.insertSeparator();
    if ( style->hasProperty( QTextFormat::BackgroundBrush) )
      m.insertItem( i18n("Unset Background Color"), this, SLOT(unsetColor(int)), 0, 100 );
    if ( style->hasProperty( KTextEditor::Attribute::SelectedBackground ) )
      m.insertItem( i18n("Unset Selected Background Color"), this, SLOT(unsetColor(int)), 0, 101 );
  }

  if ( ! i->isDefault() && ! i->defStyle() ) {
    m.insertSeparator();
    id = m.insertItem( i18n("Use &Default Style"), this, SLOT(mSlotPopupHandler(int)), 0, KateStyleListItem::UseDefaultStyle );
    m.setItemChecked( id, i->defStyle() );
  }
  m.exec( globalPos );
}

void KateStyleListView::showPopupMenu( QTreeWidgetItem *i, const QPoint &pos )
{
  if ( dynamic_cast<KateStyleListItem*>(i) )
    showPopupMenu( (KateStyleListItem*)i, pos, true );
}

void KateStyleListView::mSlotPopupHandler( int z )
{
  ((KateStyleListItem*)currentItem())->changeProperty( z );
}

void KateStyleListView::unsetColor( int c )
{
  ((KateStyleListItem*)currentItem())->unsetColor( c );
  emitChanged();
}
//END

//BEGIN KateStyleListItem
static const int BoxSize = 16;
static const int ColorBtnWidth = 32;

KateStyleListDelegate::KateStyleListDelegate(QWidget* widget)
  : m_widget(widget)
{
}

void KateStyleListDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  static QSet<int> columns;
  if (!columns.count())
    columns << KateStyleListItem::Foreground << KateStyleListItem::SelectedForeground << KateStyleListItem::Background << KateStyleListItem::SelectedBackground;

  if (!columns.contains(index.column()))
    return QItemDelegate::paint(painter, option, index);

  QVariant displayData = index.model()->data(index);
  Q_ASSERT(displayData.type() == QVariant::Brush);

  QBrush brush = qVariantValue<QBrush>(displayData);

  QStyleOptionButton opt;
  opt.rect = option.rect;
  opt.palette = m_widget->palette();

  if (brush == QBrush()) {
    opt.text = i18n("None set");
    brush = Qt::white;
  }

  opt.palette.setBrush(QPalette::Background, brush);

  m_widget->style()->drawControl(QStyle::CE_PushButton, &opt, painter, m_widget);
}

KateStyleListItem::KateStyleListItem( QTreeWidgetItem *parent, const QString & stylename,
                              KTextEditor::Attribute *defaultAttribute, KateExtendedAttribute *actualAttribute )
        : QTreeWidgetItem( parent ),
          currentStyle( 0L ),
          defaultStyle( defaultAttribute ),
          actualStyle( actualAttribute )
{
  initStyle();
  setText(0, stylename);
}

KateStyleListItem::KateStyleListItem( QTreeWidget *parent, const QString & stylename,
                              KTextEditor::Attribute *defaultAttribute, KateExtendedAttribute *actualAttribute )
        : QTreeWidgetItem( parent ),
          currentStyle( 0L ),
          defaultStyle( defaultAttribute ),
          actualStyle( actualAttribute )
{
  initStyle();
  setText(0, stylename);
}

void KateStyleListItem::initStyle()
{
  if (!actualStyle)
  {
    currentStyle = defaultStyle;
  }
  else
  {
    currentStyle = new KTextEditor::Attribute (*defaultStyle);

    if (actualStyle->hasAnyProperty())
      *currentStyle += *actualStyle;
  }

  setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
}

QVariant KateStyleListItem::data( int column, int role ) const
{
  if (column == Context) {
    switch (role) {
      case Qt::TextColorRole:
        if (style()->hasProperty(QTextFormat::ForegroundBrush))
          return style()->foreground().color();
        break;

      case Qt::BackgroundColorRole:
        if (style()->hasProperty(QTextFormat::BackgroundBrush))
          return style()->background().color();
        break;

      case Qt::FontRole:
        return style()->font();
        break;
    }
  }

  if (role == Qt::CheckStateRole) {
    switch (column) {
      case Bold:
        return style()->fontBold();
      case Italic:
        return style()->fontItalic();
      case Underline:
        return style()->fontUnderline();
      case StrikeOut:
        return style()->fontStrikeOut();
      case UseDefaultStyle:
        return *currentStyle == *defaultStyle;
    }
  }

  if (role == Qt::DisplayRole) {
    switch (column) {
      case Foreground:
        return style()->foreground();
      case SelectedForeground:
        return style()->selectedForeground();
      case Background:
        return style()->background();
      case SelectedBackground:
        return style()->selectedBackground();
    }
  }

  return QTreeWidgetItem::data(column, role);
}

void KateStyleListItem::setData( int column, int role, const QVariant& value )
{
  if (role == Qt::CheckStateRole) {
    switch (column) {
      case Bold:
        style()->setFontBold(value.toBool());
        break;

      case Italic:
        style()->setFontItalic(value.toBool());
        break;

      case Underline:
        style()->setFontUnderline(value.toBool());
        break;

      case StrikeOut:
        style()->setFontStrikeOut(value.toBool());
        break;
    }
  }

  QTreeWidgetItem::setData(column, role, value);
}

void KateStyleListItem::updateStyle()
{
  // nothing there, not update it, will crash
  if (!actualStyle)
    return;

  if ( currentStyle->hasProperty(QTextFormat::FontWeight) )
  {
    if ( currentStyle->fontWeight() != actualStyle->fontWeight())
      actualStyle->setFontWeight( currentStyle->fontWeight() );
  }

  if ( currentStyle->hasProperty(QTextFormat::FontItalic) )
  {
    if ( currentStyle->fontItalic() != actualStyle->fontItalic())
      actualStyle->setFontItalic( currentStyle->fontItalic() );
  }

  if ( currentStyle->hasProperty(QTextFormat::FontStrikeOut) )
  {
    if ( currentStyle->fontStrikeOut() != actualStyle->fontStrikeOut())

      actualStyle->setFontStrikeOut( currentStyle->fontStrikeOut() );
  }

  if ( currentStyle->hasProperty(QTextFormat::FontUnderline) )
  {
    if ( currentStyle->fontUnderline() != actualStyle->fontUnderline())
      actualStyle->setFontUnderline( currentStyle->fontUnderline() );
  }

  if ( currentStyle->hasProperty(KTextEditor::Attribute::Outline) )
  {
    if ( currentStyle->outline() != actualStyle->outline())
      actualStyle->setOutline( currentStyle->outline() );
  }

  if ( currentStyle->hasProperty(QTextFormat::ForegroundBrush) )
  {
    if ( currentStyle->foreground() != actualStyle->foreground())
      actualStyle->setForeground( currentStyle->foreground() );
  }

  if ( currentStyle->hasProperty(KTextEditor::Attribute::SelectedForeground) )
  {
    if ( currentStyle->selectedForeground() != actualStyle->selectedForeground())
      actualStyle->setSelectedForeground( currentStyle->selectedForeground() );
  }

  if ( currentStyle->hasProperty(QTextFormat::BackgroundBrush) )
  {
    if ( currentStyle->background() != actualStyle->background())
      actualStyle->setBackground( currentStyle->background() );
  }

  if ( currentStyle->hasProperty(KTextEditor::Attribute::SelectedBackground) )
  {
    if ( currentStyle->selectedBackground() != actualStyle->selectedBackground())
      actualStyle->setSelectedBackground( currentStyle->selectedBackground() );
  }
}

/* only true for a hl mode item using it's default style */
bool KateStyleListItem::defStyle() const { return actualStyle && actualStyle->properties() != defaultStyle->properties(); }

/* true for default styles */
bool KateStyleListItem::isDefault() const { return actualStyle ? false : true; }

void KateStyleListItem::activate( int column, const QPoint &localPos )
{
  QTreeWidget *lv = treeWidget();
  int x = 0;
  for( int c = 0; c < column-1; c++ )
    x += lv->columnWidth( c );
  int w;
  switch( column ) {
    case Bold:
    case Italic:
    case Underline:
    case StrikeOut:
    case UseDefaultStyle:
      w = BoxSize;
      break;
    case Foreground:
    case SelectedForeground:
    case Background:
    case SelectedBackground:
      w = ColorBtnWidth;
      break;
    default:
      return;
  }
  if ( !QRect( x, 0, w, BoxSize ).contains( localPos ) )
  changeProperty( column );
}

void KateStyleListItem::changeProperty( int p )
{
  if ( p == Bold )
    currentStyle->setFontBold( ! currentStyle->fontBold() );
  else if ( p == Italic )
    currentStyle->setFontItalic( ! currentStyle->fontItalic() );
  else if ( p == Underline )
    currentStyle->setFontUnderline( ! currentStyle->fontUnderline() );
  else if ( p == StrikeOut )
    currentStyle->setFontStrikeOut( ! currentStyle->fontStrikeOut() );
  else if ( p == UseDefaultStyle )
    toggleDefStyle();
  else
    setColor( p );

  updateStyle ();

  ((KateStyleListView*)treeWidget())->emitChanged();
}

void KateStyleListItem::toggleDefStyle()
{
  if ( *currentStyle == *defaultStyle ) {
    KMessageBox::information( treeWidget(),
         i18n("\"Use Default Style\" will be automatically unset when you change any style properties."),
         i18n("Kate Styles"),
         "Kate hl config use defaults" );
  }
  else {
    delete currentStyle;
    currentStyle = new KTextEditor::Attribute( *defaultStyle );
    //FIXME
    //repaint();
  }
}

void KateStyleListItem::setColor( int column )
{
  QColor c; // use this
  QColor d; // default color
  if ( column == Foreground)
  {
    c = currentStyle->foreground();
    d = defaultStyle->foreground();
  }
  else if ( column == SelectedForeground )
  {
    c = currentStyle->selectedForeground();
    d = currentStyle->selectedForeground();
  }
  else if ( column == Background )
  {
    c = currentStyle->background();
    d = defaultStyle->background();
  }
  else if ( column == SelectedBackground )
  {
    c = currentStyle->selectedBackground();
    d = defaultStyle->selectedBackground();
  }

  if ( KColorDialog::getColor( c, d, treeWidget() ) != QDialog::Accepted) return;

  bool def = ! c.isValid();

  // if set default, and the attrib is set in the default style use it
  // else if set default, unset it
  // else set the selected color
  switch (column)
  {
    case Foreground:
      if ( def )
      {
        if ( defaultStyle->hasProperty(QTextFormat::ForegroundBrush) )
          currentStyle->setForeground( defaultStyle->foreground());
        else
          currentStyle->clearProperty(QTextFormat::ForegroundBrush);
      }
      else
        currentStyle->setForeground( c );
    break;
    case SelectedForeground:
      if ( def )
      {
        if ( defaultStyle->hasProperty(KTextEditor::Attribute::SelectedForeground) )
          currentStyle->setSelectedForeground( defaultStyle->selectedForeground());
        else
          currentStyle->clearProperty(KTextEditor::Attribute::SelectedForeground);
      }
      else
        currentStyle->setSelectedForeground( c );
    break;
    case Background:
      if ( def )
      {
        if ( defaultStyle->hasProperty(QTextFormat::BackgroundBrush) )
          currentStyle->setBackground( defaultStyle->background());
        else
          currentStyle->clearProperty(QTextFormat::BackgroundBrush);
      }
      else
        currentStyle->setBackground( c );
    break;
    case SelectedBackground:
      if ( def )
      {
        if ( defaultStyle->hasProperty(KTextEditor::Attribute::SelectedBackground) )
          currentStyle->setSelectedBackground( defaultStyle->selectedBackground());
        else
          currentStyle->clearProperty(KTextEditor::Attribute::SelectedBackground);
      }
      else
        currentStyle->setSelectedBackground( c );
    break;
  }

  //FIXME
  //repaint();
}

void KateStyleListItem::unsetColor( int c )
{
  if ( c == 100 && currentStyle->hasProperty(QTextFormat::BackgroundBrush) )
    currentStyle->clearProperty(QTextFormat::BackgroundBrush);
  else if ( c == 101 && currentStyle->hasProperty(KTextEditor::Attribute::SelectedBackground) )
    currentStyle->clearProperty(KTextEditor::Attribute::SelectedBackground);
}

#if 0
void KateStyleListItem::paintCell( QPainter *p, const QColorGroup& /*cg*/, int col, int width, int align )
{

  if ( !p )
    return;

  QTreeWidget *lv = treeWidget();
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
      mcg.setColor(QColorGroup::Text, currentStyle->foreground());
      mcg.setColor(QColorGroup::HighlightedText, currentStyle->selectedForeground());
      // text background color
      c = currentStyle->background();
      if ( c.isValid() && currentStyle->hasProperty(QTextFormat::BackgroundBrush) )
        mcg.setColor( QColorGroup::Base, c );
      if ( isSelected() && currentStyle->hasProperty(KTextEditor::Attribute::SelectedBackground) )
      {
        c = currentStyle->selectedBackground();
        if ( c.isValid() )
          mcg.setColor( QColorGroup::Highlight, c );
      }
      QFont f ( ((KateStyleListView*)lv)->docfont );
      p->setFont( currentStyle->font() );
      // FIXME FIXME port to new Attribute
      // FIXME - repainting when text is cropped, and the column is enlarged is buggy.
      // Maybe I need painting the string myself :(
      // (wilbert) it depends on the font used
      QTreeWidgetItem::paintCell( p, mcg, col, width, align );
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
      if ( (col == Bold && currentStyle->fontBold()) ||
          (col == Italic && currentStyle->fontItalic()) ||
          (col == TextUnderline && currentStyle->fontUnderline()) ||
          (col == Strikeout && currentStyle->fontStrikeOut()) ||
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
        c = currentStyle->foreground();
        set = currentStyle->hasProperty(QTextFormat::ForegroundBrush);
      }
      else if ( col == SelColor )
      {
        c = currentStyle->selectedForeground();
        set = currentStyle->hasProperty( KTextEditor::Attribute::SelectedForeground);
      }
      else if ( col == BgColor )
      {
        set = currentStyle->hasProperty(QTextFormat::BackgroundBrush);
        c = set ? currentStyle->background().color() : mcg.base();
      }
      else if ( col == SelBgColor )
      {
        set = currentStyle->hasProperty(KTextEditor::Attribute::SelectedBackground);
        c = set ? currentStyle->selectedBackground().color(): mcg.base();
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
#endif
//END

//BEGIN KateStyleListCaption
KateStyleListCaption::KateStyleListCaption( QTreeWidget *parent, const QString & name )
      :  QTreeWidgetItem( parent )
{
  setText(0, name);
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on;
