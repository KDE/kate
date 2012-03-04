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

#ifndef KATE_COLOR_TREE_WIDGET_H
#define KATE_COLOR_TREE_WIDGET_H

#include <QtGui/QTreeWidget>

class KConfigGroup;
class KateColorTreeItem;

class KateColorItem
{
  public:
    KateColorItem()
      : useDefault(true)
    {
    }

    QString name; // translated name
    QString category; // translated category for tree view hierarchy
    QString whatsThis; // what's this info
    QString key;  // untranslated id, used as key to save/load from KConfig
    QColor color; // user visible color
    QColor defaultColor; // used when "Default" is clicked
    bool useDefault; // flag whether to use the default color
};

class KateColorTreeWidget : public QTreeWidget
{
  Q_OBJECT
  friend class KateColorTreeItem;
  friend class KateColorTreeDelegate;

  public:
    explicit KateColorTreeWidget(QWidget *parent = 0);

  public:
    void addColorItem(const KateColorItem& colorItem);
    void addColorItems(const QVector<KateColorItem>& colorItems);

    QVector<KateColorItem> colorItems() const;

    QColor findColor(const QString& key) const;

  public Q_SLOTS:
    void selectDefaults();

  Q_SIGNALS:
    void changed();

  protected:
    virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event);
    void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const;
};

#endif

// kate: indent-width 2; replace-tabs on;
