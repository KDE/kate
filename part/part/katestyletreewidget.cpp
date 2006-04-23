/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

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

#include "katestyletreewidget.h"

#include <QPainter>
#include <QContextMenuEvent>
#include <QAction>
#include <QItemDelegate>

#include <klocale.h>
#include <kicon.h>
#include <kglobalsettings.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kcolordialog.h>

#include "kateconfig.h"
#include "kateextendedattribute.h"

//BEGIN KateStyleTreeDelegate
class KateStyleTreeDelegate : public QItemDelegate
{
  public:
    KateStyleTreeDelegate(QWidget* widget);

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

  private:
    QWidget* m_widget;
};
//END

//BEGIN KateStyleTreeWidgetItem decl
/*
    QListViewItem subclass to display/edit a style, bold/italic is check boxes,
    normal and selected colors are boxes, which will display a color chooser when
    activated.
    The context name for the style will be drawn using the editor default font and
    the chosen colors.
    This widget id designed to handle the default as well as the individual hl style
    lists.
    This widget is designed to work with the KateStyleTreeWidget class exclusively.
    Added by anders, jan 23 2002.
*/
class KateStyleTreeWidgetItem : public QTreeWidgetItem
{
  public:
    KateStyleTreeWidgetItem( QTreeWidgetItem *parent, const QString& styleName, KTextEditor::Attribute* defaultstyle, KateExtendedAttribute* data = 0L );
    KateStyleTreeWidgetItem( QTreeWidget *parent, const QString& styleName, KTextEditor::Attribute* defaultstyle, KateExtendedAttribute* data = 0L );
    ~KateStyleTreeWidgetItem() { if (actualStyle) delete currentStyle; };

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

    KateStyleTreeWidget* treeWidget() const;

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


//BEGIN KateStyleTreeWidget
KateStyleTreeWidget::KateStyleTreeWidget( QWidget *parent, bool showUseDefaults )
    : QTreeWidget( parent )
{
  setItemDelegate(new KateStyleTreeDelegate(this));

  setColumnCount( showUseDefaults ? 10 : 9 );
  QStringList headers;
  headers << i18n("Context") << QString() << QString() << QString() << QString() << i18n("Normal") << i18n("Selected") << i18n("Background") << i18n("Background Selected") << i18n("Use Default Style");
  setHeaderLabels(headers);

  headerItem()->setIcon(1, KIcon("text_bold"));
  headerItem()->setIcon(2, KIcon("text_italic"));
  headerItem()->setIcon(3, KIcon("text_under"));
  headerItem()->setIcon(4, KIcon("text_strike"));

  // grap the bg color, selected color and default font
  normalcol = KGlobalSettings::textColor();
  bgcol = KateRendererConfig::global()->backgroundColor();
  selcol = KateRendererConfig::global()->selectionColor();
  docfont = *KateRendererConfig::global()->font();

  QPalette pal = viewport()->palette();
  pal.setColor(QPalette::Background, bgcol);
  viewport()->setPalette( pal );
}

QIcon brushIcon(const QColor& color)
{
  QPixmap pm(16,16);
  QRect all(0,0,15,15);
  {
    QPainter p(&pm);
    p.fillRect(all, color);
    p.setPen(Qt::black);
    p.drawRect(all);
  }
  return QIcon(pm);
}

bool KateStyleTreeWidget::edit( const QModelIndex & index, EditTrigger trigger, QEvent * event )
{
  KateStyleTreeWidgetItem *i = dynamic_cast<KateStyleTreeWidgetItem*>(itemFromIndex(index));
  if (!i)
    return QTreeWidget::edit(index, trigger, event);

  switch (trigger) {
    case QAbstractItemView::DoubleClicked:
    case QAbstractItemView::SelectedClicked:
    case QAbstractItemView::EditKeyPressed:
      i->changeProperty(index.column());
      return false;
    default:
      return QTreeWidget::edit(index, trigger, event);
  }
}

void KateStyleTreeWidget::resizeColumns()
{
  for (int i = 0; i < columnCount(); ++i)
    resizeColumnToContents(i);
}

void KateStyleTreeWidget::showEvent( QShowEvent * event )
{
  QTreeWidget::showEvent(event);

  resizeColumns();
}

void KateStyleTreeWidget::contextMenuEvent( QContextMenuEvent * event )
{
  KateStyleTreeWidgetItem *i = dynamic_cast<KateStyleTreeWidgetItem*>(itemAt(event->pos()));
  if (!i) return;

  KMenu m( this );
  KTextEditor::Attribute *currentStyle = i->style();
  // the title is used, because the menu obscures the context name when
  // displayed on behalf of spacePressed().
  QPainter p;
  p.setPen(Qt::black);

  QIcon cl = brushIcon( i->style()->foreground().color() );
  QIcon scl = brushIcon( i->style()->selectedForeground().color() );
  QIcon bgcl = brushIcon( i->style()->hasProperty(QTextFormat::BackgroundBrush) ? i->style()->background().color() : viewport()->palette().base().color() );
  QIcon sbgcl = brushIcon( i->style()->hasProperty(KTextEditor::Attribute::SelectedBackground) ? i->style()->selectedBackground().color() : viewport()->palette().base().color() );

  m.addTitle( i->contextName() );

  QAction* a = m.addAction( i18n("&Bold"), this, SLOT(changeProperty()) );
  a->setCheckable(true);
  a->setChecked( currentStyle->fontBold() );
  a->setData(KateStyleTreeWidgetItem::Bold);

  a = m.addAction( i18n("&Italic"), this, SLOT(changeProperty()) );
  a->setCheckable(true);
  a->setChecked( currentStyle->fontItalic() );
  a->setData(KateStyleTreeWidgetItem::Italic);

  a = m.addAction( i18n("&Underline"), this, SLOT(changeProperty()) );
  a->setCheckable(true);
  a->setChecked( currentStyle->fontUnderline() );
  a->setData(KateStyleTreeWidgetItem::Underline);

  a = m.addAction( i18n("S&trikeout"), this, SLOT(changeProperty()) );
  a->setCheckable(true);
  a->setChecked( currentStyle->fontStrikeOut() );
  a->setData(KateStyleTreeWidgetItem::StrikeOut);

  m.addSeparator();

  a = m.addAction( cl, i18n("Normal &Color..."), this, SLOT(changeProperty()) );
  a->setData(KateStyleTreeWidgetItem::Foreground);

  a = m.addAction( scl, i18n("&Selected Color..."), this, SLOT(changeProperty()) );
  a->setData(KateStyleTreeWidgetItem::SelectedForeground);

  a = m.addAction( bgcl, i18n("&Background Color..."), this, SLOT(changeProperty()) );
  a->setData(KateStyleTreeWidgetItem::Background);

  a = m.addAction( sbgcl, i18n("S&elected Background Color..."), this, SLOT(changeProperty()) );
  a->setData(KateStyleTreeWidgetItem::SelectedBackground);

  // Unset [some] colors. I could show one only if that button was clicked, but that
  // would disable setting this with the keyboard (how many aren't doing just
  // that every day? ;)
  // ANY ideas for doing this in a nicer way will be warmly wellcomed.
  KTextEditor::Attribute *style = i->style();
  if ( style->hasProperty( QTextFormat::BackgroundBrush) || style->hasProperty( KTextEditor::Attribute::SelectedBackground ) )
  {
    m.addSeparator();
    if ( style->hasProperty( QTextFormat::BackgroundBrush) ) {
      a = m.addAction( i18n("Unset Background Color"), this, SLOT(unsetColor()) );
      a->setData(100);
    }
    if ( style->hasProperty( KTextEditor::Attribute::SelectedBackground ) ) {
      a = m.addAction( i18n("Unset Selected Background Color"), this, SLOT(unsetColor()) );
      a->setData(101);
    }
  }

  if ( ! i->isDefault() && ! i->defStyle() ) {
    m.addSeparator();
    a = m.addAction( i18n("Use &Default Style"), this, SLOT(changeProperty()) );
    a->setCheckable(true);
    a->setChecked( i->defStyle() );
    a->setData(KateStyleTreeWidgetItem::UseDefaultStyle);
  }
  m.exec( event->globalPos() );
}

void KateStyleTreeWidget::changeProperty()
{
  ((KateStyleTreeWidgetItem*)currentItem())->changeProperty( static_cast<QAction*>(sender())->data().toInt() );
}

void KateStyleTreeWidget::unsetColor()
{
  ((KateStyleTreeWidgetItem*)currentItem())->unsetColor( static_cast<QAction*>(sender())->data().toInt() );
}

void KateStyleTreeWidget::emitChanged( )
{
  emit changed();
}

void KateStyleTreeWidget::addItem( const QString & styleName, KTextEditor::Attribute * defaultstyle, KateExtendedAttribute * data )
{
  new KateStyleTreeWidgetItem(this, styleName, defaultstyle, data);
}

void KateStyleTreeWidget::addItem( QTreeWidgetItem * parent, const QString & styleName, KTextEditor::Attribute * defaultstyle, KateExtendedAttribute * data )
{
  new KateStyleTreeWidgetItem(parent, styleName, defaultstyle, data);
}
//END

//BEGIN KateStyleTreeWidgetItem
static const int BoxSize = 16;
static const int ColorBtnWidth = 32;

KateStyleTreeDelegate::KateStyleTreeDelegate(QWidget* widget)
  : m_widget(widget)
{
}

void KateStyleTreeDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  static QSet<int> columns;
  if (!columns.count())
    columns << KateStyleTreeWidgetItem::Foreground << KateStyleTreeWidgetItem::SelectedForeground << KateStyleTreeWidgetItem::Background << KateStyleTreeWidgetItem::SelectedBackground;

  if (!columns.contains(index.column()))
    return QItemDelegate::paint(painter, option, index);

  QVariant displayData = index.model()->data(index);
  if (displayData.type() != QVariant::Brush)
    return QItemDelegate::paint(painter, option, index);

  QBrush brush = qVariantValue<QBrush>(displayData);

  QStyleOptionButton opt;
  opt.rect = option.rect;
  opt.palette = m_widget->palette();

  bool set = brush != QBrush();

  if (!set) {
    opt.text = i18nc("No text or background colour set", "None set");
    brush = Qt::white;
  }

  m_widget->style()->drawControl(QStyle::CE_PushButton, &opt, painter, m_widget);

  if (set)
    painter->fillRect(m_widget->style()->subElementRect(QStyle::SE_PushButtonContents, &opt,m_widget), brush);
}

KateStyleTreeWidgetItem::KateStyleTreeWidgetItem( QTreeWidgetItem *parent, const QString & stylename,
                              KTextEditor::Attribute *defaultAttribute, KateExtendedAttribute *actualAttribute )
        : QTreeWidgetItem( parent ),
          currentStyle( 0L ),
          defaultStyle( defaultAttribute ),
          actualStyle( actualAttribute )
{
  initStyle();
  setText(0, stylename);
}

KateStyleTreeWidgetItem::KateStyleTreeWidgetItem( QTreeWidget *parent, const QString & stylename,
                              KTextEditor::Attribute *defaultAttribute, KateExtendedAttribute *actualAttribute )
        : QTreeWidgetItem( parent ),
          currentStyle( 0L ),
          defaultStyle( defaultAttribute ),
          actualStyle( actualAttribute )
{
  initStyle();
  setText(0, stylename);
}

void KateStyleTreeWidgetItem::initStyle()
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

QVariant KateStyleTreeWidgetItem::data( int column, int role ) const
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

void KateStyleTreeWidgetItem::setData( int column, int role, const QVariant& value )
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

void KateStyleTreeWidgetItem::updateStyle()
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
bool KateStyleTreeWidgetItem::defStyle() const { return actualStyle && actualStyle->properties() != defaultStyle->properties(); }

/* true for default styles */
bool KateStyleTreeWidgetItem::isDefault() const { return actualStyle ? false : true; }

void KateStyleTreeWidgetItem::changeProperty( int p )
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

  treeWidget()->emitChanged();
}

void KateStyleTreeWidgetItem::toggleDefStyle()
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

void KateStyleTreeWidgetItem::setColor( int column )
{
  QColor c; // use this
  QColor d; // default color
  if ( column == Foreground)
  {
    c = currentStyle->foreground().color();
    d = defaultStyle->foreground().color();
  }
  else if ( column == SelectedForeground )
  {
    c = currentStyle->selectedForeground().color();
    d = currentStyle->selectedForeground().color();
  }
  else if ( column == Background )
  {
    c = currentStyle->background().color();
    d = defaultStyle->background().color();
  }
  else if ( column == SelectedBackground )
  {
    c = currentStyle->selectedBackground().color();
    d = defaultStyle->selectedBackground().color();
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

void KateStyleTreeWidgetItem::unsetColor( int c )
{
  if ( c == 100 && currentStyle->hasProperty(QTextFormat::BackgroundBrush) )
    currentStyle->clearProperty(QTextFormat::BackgroundBrush);
  else if ( c == 101 && currentStyle->hasProperty(KTextEditor::Attribute::SelectedBackground) )
    currentStyle->clearProperty(KTextEditor::Attribute::SelectedBackground);
}

KateStyleTreeWidget* KateStyleTreeWidgetItem::treeWidget() const
{
  return static_cast<KateStyleTreeWidget*>(QTreeWidgetItem::treeWidget());
}
//END

#include "katestyletreewidget.moc"
