#include "katecolortreewidget.h"

#include <QtGui/QStyledItemDelegate>
#include <QtGui/QPainter>

#include <klocale.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kcolordialog.h>
#include <kcolorscheme.h>
#include <kcolorutils.h>

#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

// both color roles are used in column 1
static const int KateCustomColor = Qt::UserRole;
static const int KateSystemColor = KateCustomColor + 1;

//BEGIN KateColorTreeDelegate
class KateColorTreeDelegate : public QStyledItemDelegate
{
  public:
    KateColorTreeDelegate(QTreeWidget* widget)
      : QStyledItemDelegate(widget)
      , m_tree(widget)
    {
    }

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
      // draw standard item
      QStyledItemDelegate::paint(painter, option, index);

      Q_ASSERT(index.isValid());
      Q_ASSERT(index.column() >= 0 && index.column() <= 2);

      // if top-level node, abort
      if (!index.parent().isValid()) {
        return;
      }

      // TODO: more Q_ASSERT();

      if (index.column() == 1) {

        QModelIndex useDefaultIndex = index.sibling(index.row(), 2);
        Q_ASSERT(useDefaultIndex.isValid());

        bool useDefault = (useDefaultIndex.data(Qt::CheckStateRole) == Qt::Checked);

        QVariant color;
        if (useDefault) {
          color = index.data(KateSystemColor);
        } else {
           color = index.data(KateCustomColor);
        }

        if (color.canConvert(QVariant::Color)) {

          QStyleOptionButton opt;
          opt.rect = option.rect;
          opt.palette = m_tree->palette();

          // draw color button
          m_tree->style()->drawControl(QStyle::CE_PushButton, &opt, painter, m_tree);
          QBrush brush = color.value<QColor>();
          painter->fillRect(m_tree->style()->subElementRect(QStyle::SE_PushButtonContents, &opt,m_tree), brush);
        }
      }
    }

  private:
    QTreeWidget* m_tree;
};
//END KateColorTreeDelegate


//BEGIN KateColorTreeItem
class KateColorTreeItem : public QTreeWidgetItem
{
  public:
    KateColorTreeItem(const QString& name, const QString& key, QTreeWidgetItem* parent = 0)
      : QTreeWidgetItem(parent)
      , m_key(key)
    {
      setText(0, name);
      setData(2, Qt::CheckStateRole, Qt::Checked);
    }

    QColor color() const
    {
      int role = useCustomColor() ? KateCustomColor : KateSystemColor;
      QVariant colorVariant = data(1, role);
      Q_ASSERT(colorVariant.canConvert(QVariant::Color));
      return colorVariant.value<QColor>();
    }

    bool useCustomColor() const
    {
      return data(2, Qt::CheckStateRole) == Qt::Unchecked;
    }

    void setUseCustomColor(bool useCustom)
    {
      setData(2, Qt::CheckStateRole, useCustom ? Qt::Unchecked : Qt::Checked);
    }

    void setCustomColor(const QColor& c)
    {
      setData(1, KateCustomColor, c);
    }

    void setSystemColor(const QColor& c)
    {
      setData(1, KateSystemColor, c);
    }

    void readConfig(KConfigGroup& config, const QColor& systemColor)
    {
      const QColor customColor = config.readEntry(m_key, systemColor);
      const bool useCustomColor = config.readEntry("Custom " + m_key, false);
      setCustomColor(customColor);
      setSystemColor(systemColor);
      setUseCustomColor(useCustomColor);
    }

    void writeConfig(KConfigGroup& config)
    {
      config.writeEntry(m_key, color());
      config.writeEntry("Custom " + m_key, useCustomColor());
    }

  private:
    QString m_key;
};
//END KateColorTreeItem


KateColorTreeWidget::KateColorTreeWidget(QWidget *parent)
  : QTreeWidget(parent)
{
  setItemDelegate(new KateColorTreeDelegate(this));

  QStringList headers;
  headers << i18nc("@title:column the color name", "Color Role")
          << i18nc("@title:column a color button", "Color")
          << i18nc("@title:column KDE color scheme", "Use System Default");
  setHeaderLabels(headers);
  setRootIsDecorated(false);

  init();

  expandAll();

  resizeColumnToContents(0);
  setColumnWidth(1, 100);
  resizeColumnToContents(2);

  // create dummy config to initialize all entries with default values
  KConfig cfg;
  KConfigGroup cg(&cfg, QString());
  readConfig(cg);
}

void KateColorTreeWidget::dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  Q_UNUSED(bottomRight)

  // if top-level node, abort
  if (!topLeft.parent().isValid()) {
    return;
  }

  Q_ASSERT(topLeft.row() == bottomRight.row());
  Q_ASSERT(topLeft.column() == bottomRight.column());

  QModelIndex left(topLeft);
  QModelIndex right(left);

  if (left.column() == 1) {
    // if checked, uncheck column 2: it is a custom color now
    QModelIndex useDefaultIndex = left.sibling(left.row(), 2);
    if (useDefaultIndex.data(Qt::CheckStateRole) == Qt::Checked) {
      model()->setData(useDefaultIndex, Qt::Unchecked, Qt::CheckStateRole);
    }
  }
  else if (left.column() == 2) {
    // repaint column 1: toggle of system default color or custom color
    left = topLeft.sibling(topLeft.row(), 1);
  }

  QTreeView::dataChanged(left, right);

  emit changed();
}

bool KateColorTreeWidget::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
  // accept edit only for color buttons in column 1
  if (!index.parent().isValid() || index.column() != 1) {
    return QTreeWidget::edit(index, trigger, event);
  }

  Q_ASSERT(index.data(KateCustomColor).canConvert(QVariant::Color));
  Q_ASSERT(index.data(KateSystemColor).canConvert(QVariant::Color));

  bool openColorDialog = false;
  if (event && event->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    openColorDialog = (ke->key() == Qt::Key_Space); // allow Space to edit
  }

  switch (trigger) {
    case QAbstractItemView::DoubleClicked:
    case QAbstractItemView::SelectedClicked:
    case QAbstractItemView::EditKeyPressed: // = F2
      openColorDialog = true;
      break;
    default: break;
  }

  if (openColorDialog) {
    QColor customColor = index.data(KateCustomColor).value<QColor>();
    QColor systemColor = index.data(KateSystemColor).value<QColor>();

    if (index.sibling(index.row(), 2).data(Qt::CheckStateRole) == Qt::Checked) {
      customColor = systemColor;
    }

    if (KColorDialog::getColor(customColor, systemColor, this) == QDialog::Accepted) {
      model()->setData(index, customColor, KateCustomColor);
    }

    return false;
  }
  return QTreeWidget::edit(index, trigger, event);
}

void KateColorTreeWidget::selectDefaults()
{
  for (int a = 0; a < topLevelItemCount(); ++a) {
    for (int b = 0; b < topLevelItem(a)->childCount(); ++b) {
      topLevelItem(a)->child(b)->setData(2, Qt::CheckStateRole, Qt::Checked);
    }
  }
}

void KateColorTreeWidget::readConfig(KConfigGroup& config)
{
  QColor systemColor;

  //
  // editor background colors
  //
  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
  m_textArea->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Inactive, KColorScheme::Selection).background().color();
  m_selectedText->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::AlternateBackground).color();
  m_currentLine->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NeutralBackground).color();
  m_searchHighlight->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  m_replaceHighlight->readConfig(config, systemColor);

  //
  // icon border
  //
  systemColor = KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
  m_background->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground().color();
  m_lineNumbers->readConfig(config, systemColor);

  qreal bgLuma = KColorUtils::luma(KColorScheme(QPalette::Active, KColorScheme::View).background().color());
  systemColor = KColorUtils::shade( KColorScheme(QPalette::Active, KColorScheme::View).background().color(), bgLuma > 0.3 ? -0.15 : 0.03 );
  m_wordWrapMarker->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  m_savedLine->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
  m_modifiedLine->readConfig(config, systemColor);

  //
  // text decorations
  //
  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::NegativeText).color();
  m_spellingMistake->readConfig(config, systemColor);

  systemColor = KColorUtils::shade(KColorScheme(QPalette::Active, KColorScheme::View).background().color(), bgLuma > 0.7 ? -0.35 : 0.3);
  m_spaceMarker->readConfig(config, systemColor);

  systemColor = KColorUtils::tint(KColorScheme(QPalette::Active, KColorScheme::View).background().color(),
                                                                    KColorScheme(QPalette::Active, KColorScheme::View).decoration(KColorScheme::HoverColor).color());
  m_bracketHighlight->readConfig(config, systemColor);

  //
  // marker colors
  //
  systemColor = Qt::blue; // TODO: if possible, derive from system color scheme
  m_bookmark->readConfig(config, systemColor);

  systemColor = Qt::red; // TODO: if possible, derive from system color scheme
  m_activeBreakpoint->readConfig(config, systemColor);

  systemColor = Qt::yellow; // TODO: if possible, derive from system color scheme
  m_reachedBreakpoint->readConfig(config, systemColor);

  systemColor = Qt::magenta; // TODO: if possible, derive from system color scheme
  m_disabledBreakpoint->readConfig(config, systemColor);

  systemColor = Qt::gray; // TODO: if possible, derive from system color scheme
  m_execution->readConfig(config, systemColor);

  systemColor = Qt::green; // TODO: if possible, derive from system color scheme
  m_warning->readConfig(config, systemColor);

  systemColor = Qt::red; // TODO: if possible, derive from system color scheme
  m_error->readConfig(config, systemColor);

  //
  // text templates
  //
  systemColor = KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
  m_templateBackground->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  m_templateEditablePlaceholder->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::Window).background(KColorScheme::PositiveBackground).color();
  m_templateFocusedEditablePlaceholder->readConfig(config, systemColor);

  systemColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
  m_templateNotEditablePlaceholder->readConfig(config, systemColor);
}

void KateColorTreeWidget::writeConfig(KConfigGroup& config)
{
  //
  // editor background colors
  //
  m_textArea->writeConfig(config);
  m_selectedText->writeConfig(config);
  m_currentLine->writeConfig(config);
  m_searchHighlight->writeConfig(config);
  m_replaceHighlight->writeConfig(config);

  //
  // icon border
  //
  m_background->writeConfig(config);
  m_lineNumbers->writeConfig(config);
  m_wordWrapMarker->writeConfig(config);
  m_savedLine->writeConfig(config);
  m_modifiedLine->writeConfig(config);

  //
  // text decorations
  //
  m_spellingMistake->writeConfig(config);
  m_spaceMarker->writeConfig(config);
  m_bracketHighlight->writeConfig(config);

  //
  // marker colors
  //
  m_bookmark->writeConfig(config);
  m_activeBreakpoint->writeConfig(config);
  m_reachedBreakpoint->writeConfig(config);
  m_disabledBreakpoint->writeConfig(config);
  m_execution->writeConfig(config);
  m_warning->writeConfig(config);
  m_error->writeConfig(config);

  //
  // text templates
  //
  m_templateBackground->writeConfig(config);
  m_templateEditablePlaceholder->writeConfig(config);
  m_templateFocusedEditablePlaceholder->writeConfig(config);
  m_templateNotEditablePlaceholder->writeConfig(config);
}

void KateColorTreeWidget::init()
{
  //
  // editor background colors
  //
  QTreeWidgetItem* it = new QTreeWidgetItem();
  it->setText(0, "Editor Background Colors");
  addTopLevelItem(it);

  m_textArea = new KateColorTreeItem("Text Area", "Color Background", it);
  m_selectedText = new KateColorTreeItem("Selected Text", "Color Selection", it);
  m_currentLine = new KateColorTreeItem("Current Line", "Color Highlighted Line", it);
  m_searchHighlight = new KateColorTreeItem("Search Highlight", "Color Search Highlight", it);
  m_replaceHighlight = new KateColorTreeItem("Replace Highlight", "Color Replace Highlight", it);

  //
  // icon border
  //
  it = new QTreeWidgetItem();
  it->setText(0, "Icon Border");
  addTopLevelItem(it);

  m_background = new KateColorTreeItem("Background Area", "Color Icon Bar", it);
  m_lineNumbers = new KateColorTreeItem("Line Numbers", "Color Line Number", it);
  m_wordWrapMarker = new KateColorTreeItem("Word Wrap Marker", "Color Word Wrap Marker", it);
  m_savedLine = new KateColorTreeItem("Saved Lines", "Color Saved Lines", it);
  m_modifiedLine = new KateColorTreeItem("Modified Lines", "Color Modified Lines", it);

  //
  // text decorations
  //
  it = new QTreeWidgetItem();
  it->setText(0, "Text Decorations");
  addTopLevelItem(it);

  m_spellingMistake = new KateColorTreeItem("Spelling Mistake Line", "Color Spelling Mistake Line", it);
  m_spaceMarker = new KateColorTreeItem("Tab and Space Markers", "Color Tab Marker", it);
  m_bracketHighlight = new KateColorTreeItem("Bracket Highlight", "Color Highlighted Bracket", it);

  //
  // marker colors
  //
  it = new QTreeWidgetItem();
  it->setText(0, "Marker Colors");
  addTopLevelItem(it);

  m_bookmark = new KateColorTreeItem("Bookmark", "Color MarkType 1", it);
  m_activeBreakpoint = new KateColorTreeItem("Active Breakpoint", "Color MarkType 2", it);
  m_reachedBreakpoint = new KateColorTreeItem("Reached Breakpoint", "Color MarkType 3", it);
  m_disabledBreakpoint = new KateColorTreeItem("Disabled Breakpoint", "Color MarkType 4", it);
  m_execution = new KateColorTreeItem("Execution", "Color MarkType 5", it);
  m_warning = new KateColorTreeItem("Warning", "Color MarkType 6", it);
  m_error = new KateColorTreeItem("Error", "Color MarkType 7", it);

  //
  // text templates
  //
  it = new QTreeWidgetItem();
  it->setText(0, "Text Templates & Snippets");
  addTopLevelItem(it);

  m_templateBackground = new KateColorTreeItem("Background", "Color Template Background", it);
  m_templateEditablePlaceholder = new KateColorTreeItem("Editable Placeholder", "Color Template Editable Placeholder", it);
  m_templateFocusedEditablePlaceholder = new KateColorTreeItem("Focused Editable Placeholder", "Color Template Focused Editable Placeholder", it);
  m_templateNotEditablePlaceholder = new KateColorTreeItem("Not Editable Placeholder", "Color Template Not Editable Placeholder", it);
}

void KateColorTreeWidget::testReadConfig()
{
  KConfig cfg("/tmp/test.cfg");
  KConfigGroup cg(&cfg, "Colors");
  readConfig(cg);
}

void KateColorTreeWidget::testWriteConfig()
{
  KConfig cfg("/tmp/test.cfg");
  KConfigGroup cg(&cfg, "Colors");
  writeConfig(cg);
}

// kate: indent-width 2; replace-tabs on;
