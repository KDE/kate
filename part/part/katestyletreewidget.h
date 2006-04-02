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

#ifndef KATESTYLELISTVIEW_H
#define KATESTYLELISTVIEW_H

#include <QTreeWidget>

namespace KTextEditor {
  class Attribute;
}

class KateExtendedAttribute;

/**
 * QTreeWidget that automatically adds columns for KateStyleListItems and provides a
 * popup menu and a slot to edit a style using the keyboard.
 * Added by anders, jan 23 2002.
 */
class KateStyleTreeWidget : public QTreeWidget
{
  Q_OBJECT

  friend class KateStyleListItem;

  public:
    KateStyleTreeWidget(QWidget *parent = 0, bool showUseDefaults = false);

    void emitChanged();

    void setBgCol( const QColor &c ) { bgcol = c; }
    void setSelCol( const QColor &c ) { selcol = c; }
    void setNormalCol( const QColor &c ) { normalcol = c; }

    void addItem( QTreeWidgetItem *parent, const QString& styleName, KTextEditor::Attribute* defaultstyle, KateExtendedAttribute* data = 0L );
    void addItem( const QString& styleName, KTextEditor::Attribute* defaultstyle, KateExtendedAttribute* data = 0L );

  Q_SIGNALS:
    void changed();

  protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event);

  private Q_SLOTS:
    void changeProperty( );
    void unsetColor( );

  private:
    QColor bgcol, selcol, normalcol;
    QFont docfont;
};

#endif
