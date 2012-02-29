/* This file is part of the KDE libraries
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

//BEGIN KateColorTreeItem
class KateColorTreeItem : public QTreeWidgetItem
{
  public:
    KateColorTreeItem(const KateColorItem& colorItem, QTreeWidgetItem* parent = 0)
      : QTreeWidgetItem(parent)
      , m_colorItem(colorItem)
    {
      setText(0, m_colorItem.name);
      if (!colorItem.whatsThis.isEmpty()) {
        setData(1, Qt::WhatsThisRole, colorItem.whatsThis);
      }
    }

    QColor color() const {
      return m_colorItem.color;
    }

    void setColor(const QColor& c) {
      m_colorItem.color = c;
    }

    QColor defaultColor() const {
      return m_colorItem.defaultColor;
    }

    bool useDefaultColor() const {
      return m_colorItem.useDefault;
    }

    void setUseDefaultColor(bool useDefault) {
      m_colorItem.useDefault = useDefault;
    }

    QString key() {
      return m_colorItem.key;
    }

  private:
    KateColorItem m_colorItem;
};
//END KateColorTreeItem


//BEGIN KateColorTreeDelegate
class KateColorTreeDelegate : public QStyledItemDelegate
{
  public:
    KateColorTreeDelegate(KateColorTreeWidget* widget)
      : QStyledItemDelegate(widget)
      , m_tree(widget)
    {
    }

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
      Q_ASSERT(index.isValid());
      Q_ASSERT(index.column() >= 0 && index.column() <= 1);

      QStyledItemDelegate::paint(painter, option, index);

      // if top-level node, abort
      if (!index.parent().isValid() || index.column() == 0) {
        return;
      }

      KateColorTreeItem* item = dynamic_cast<KateColorTreeItem*>(m_tree->itemFromIndex(index));

      if (index.column() == 1) {

        QColor color = item->useDefaultColor() ? item->defaultColor() : item->color();

        QStyleOptionButton opt;
        opt.rect = option.rect;
        opt.palette = m_tree->palette();

        m_tree->style()->drawControl(QStyle::CE_PushButton, &opt, painter, m_tree);
        painter->fillRect(m_tree->style()->subElementRect(QStyle::SE_PushButtonContents, &opt, m_tree), color);
      }

      if (index.column() == 2 && !item->useDefaultColor()) {

        QPixmap p = SmallIcon("edit-undo");
        QRect rect(option.rect.left() + 10, option.rect.top() + (option.rect.height() - p.height() + 1) / 2, p.width(), p.height());

        if (option.state & QStyle::State_MouseOver || option.state & QStyle::State_HasFocus) {
          painter->drawPixmap(rect, p);
        } else {
          painter->drawPixmap(rect, SmallIcon("edit-undo", 0, KIconLoader::DisabledState));
        }
      }
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
      QSize s(QStyledItemDelegate::sizeHint(option, index));
      s.rheight() += 5;
      return s;
    }

  private:
    KateColorTreeWidget* m_tree;
};
//END KateColorTreeDelegate

KateColorTreeWidget::KateColorTreeWidget(QWidget *parent)
  : QTreeWidget(parent)
{
  setItemDelegate(new KateColorTreeDelegate(this));
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setUniformRowHeights(true);

  QStringList headers;
  headers << i18nc("@title:column the color name", "Color Role")
          << i18nc("@title:column a color button", "Color")
          << i18nc("@title:column reset color", "Reset");
  setHeaderLabels(headers);
  setRootIsDecorated(false);

  connect(this, SIGNAL(changed()), this, SLOT(testChanged()));
}

bool KateColorTreeWidget::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
  // accept edit only for color buttons in column 1 and reset in column 2
  if (!index.parent().isValid() || index.column() < 1) {
    return QTreeWidget::edit(index, trigger, event);
  }

  bool accept = false;
  if (event && event->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    accept = (ke->key() == Qt::Key_Space); // allow Space to edit
  }

  switch (trigger) {
    case QAbstractItemView::DoubleClicked:
    case QAbstractItemView::SelectedClicked:
    case QAbstractItemView::EditKeyPressed: // = F2
      accept = true;
      break;
    default: break;
  }

  if (accept) {
    KateColorTreeItem* item = dynamic_cast<KateColorTreeItem*>(itemFromIndex(index));
    QColor color = item->useDefaultColor() ? item->defaultColor() : item->color();

    // FIXME: how do we force a repaint???
    if (index.column() == 1) {
      if (KColorDialog::getColor(color, item->defaultColor(), this) == QDialog::Accepted) {
        item->setUseDefaultColor(false);
        item->setColor(color);
        viewport()->update();
        emit changed();
      }
    } else if (index.column() == 2 && !item->useDefaultColor()) {
      item->setUseDefaultColor(true);
      viewport()->update();
      emit changed();
    }

    return false;
  }
  return QTreeWidget::edit(index, trigger, event);
}

void KateColorTreeWidget::selectDefaults()
{
  bool somethingChanged = false;

  // use default colors for all selected items
  foreach (QTreeWidgetItem* item, selectedItems()) {
    if (item->parent()) {
      KateColorTreeItem* it = dynamic_cast<KateColorTreeItem*>(item);
      if (!it->useDefaultColor()) {
        it->setUseDefaultColor(true);
        somethingChanged = true;
      }
    }
  }
  if (somethingChanged) {
    viewport()->update();
    emit changed();
  }
}

void KateColorTreeWidget::addColorItem(const KateColorItem& colorItem)
{
  QTreeWidgetItem* categoryItem = 0;
  for (int i = 0; i < topLevelItemCount(); ++i) {
    if (topLevelItem(i)->text(0) == colorItem.category) {
      categoryItem = topLevelItem(i);
      break;
    }
  }

  if (!categoryItem) {
    categoryItem = new QTreeWidgetItem();
    categoryItem->setText(0, colorItem.category);
    addTopLevelItem(categoryItem);
    expandItem(categoryItem);
  }

  new KateColorTreeItem(colorItem, categoryItem);

  resizeColumnToContents(0);
}

void KateColorTreeWidget::addColorItems(const QVector<KateColorItem>& colorItems)
{
  foreach(const KateColorItem& item, colorItems)
    addColorItem(item);
}

void KateColorTreeWidget::readConfig(KConfigGroup& config)
{
  for (int a = 0; a < topLevelItemCount(); ++a) {
    QTreeWidgetItem* top = topLevelItem(a);
    for (int b = 0; b < top->childCount(); ++b) {
      KateColorTreeItem* item = dynamic_cast<KateColorTreeItem*>(top->child(b));
      item->setColor(config.readEntry(item->key(), item->defaultColor()));
      item->setUseDefaultColor(config.readEntry("Use Default " + item->key(), true));
    }
  }
  viewport()->update();
}

void KateColorTreeWidget::writeConfig(KConfigGroup& config)
{
    for (int a = 0; a < topLevelItemCount(); ++a) {
    QTreeWidgetItem* top = topLevelItem(a);
    for (int b = 0; b < top->childCount(); ++b) {
      KateColorTreeItem* item = dynamic_cast<KateColorTreeItem*>(top->child(b));
      config.writeEntry(item->key(), item->color());
      config.writeEntry("Use Default " + item->key(), item->useDefaultColor());
    }
  }
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

void KateColorTreeWidget::testChanged()
{
  qDebug() << "something changed";
}

// kate: indent-width 2; replace-tabs on;
