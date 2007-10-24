/* This file is part of the KDE project
   Copyright (C) 2006 Joseph Wenninger <jowenn@kde.org>

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

#ifndef KATEVIEWDOCUMENTPROXYMODEL_H
#define KATEVIEWDOCUMENTPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QList>
#include <QMimeData>
#include <Qt>
#include <QTimer>
#include <QColor>

class KateViewDocumentProxyModel: public QAbstractProxyModel
{
    Q_OBJECT
  public:
    KateViewDocumentProxyModel(QObject *parent);
    virtual ~KateViewDocumentProxyModel();
    virtual QVariant data ( const QModelIndex & index, int role ) const;
    virtual QModelIndex mapFromSource ( const QModelIndex & sourceIndex ) const;
    virtual QItemSelection mapSelectionFromSource ( const QItemSelection & sourceSelection ) const;
    virtual QItemSelection mapSelectionToSource ( const QItemSelection & proxySelection ) const;
    virtual QModelIndex mapToSource ( const QModelIndex & proxyIndex ) const;

    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual void setSourceModel ( QAbstractItemModel * sourceModel );

    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool dropMimeData(const QMimeData *data,
                              Qt::DropAction action, int row, int column, const QModelIndex &parent);
    virtual Qt::DropActions supportedDropActions () const;
    QItemSelectionModel *selection()
    {
      return m_selection;
    }

    bool shadingEnabled() const { return m_shadingEnabled; }
    void setShadingEnabled(bool enable) { m_shadingEnabled = enable; }
    const QColor& editShade() const { return m_editShade; }
    const QColor& viewShade() const { return m_viewShade; }
    void setEditShade( const QColor &shade ) { m_editShade = shade; }
    void setViewShade( const QColor &shade ) { m_viewShade = shade; }

    int sortRole() const { return m_sortRole; }
    void setSortRole( int role ) { m_sortRole = role; }

  private:
    QItemSelectionModel *m_selection;
    QList<QModelIndex> m_viewHistory;
    QList<QModelIndex> m_editHistory;
    QMap<QModelIndex, QBrush> m_brushes;
    QModelIndex m_current;
    QList<int> m_mapToSource;
    QList<int> m_mapFromSource;
    QTimer *m_markOpenedTimer;
    int m_rowCountOffset;
    void removeItemFromColoring(int line);
    QColor m_editShade;
    QColor m_viewShade;
    bool m_shadingEnabled;
    int m_sortRole;

  private Q_SLOTS:
    void slotColumnsAboutToBeInserted ( const QModelIndex & parent, int start, int end );
    void slotColumnsAboutToBeRemoved ( const QModelIndex & parent, int start, int end ) ;
    void slotColumnsInserted ( const QModelIndex & parent, int start, int end ) ;
    void slotColumnsRemoved ( const QModelIndex & parent, int start, int end ) ;
    void slotDataChanged ( const QModelIndex & topLeft, const QModelIndex & bottomRight );
    void slotHeaderDataChanged ( Qt::Orientation orientation, int first, int last ) ;
    void slotLayoutAboutToBeChanged () ;
    void slotLayoutChanged () ;
    void slotModelAboutToBeReset ();
    void slotModelReset () ;
    void slotRowsAboutToBeInserted ( const QModelIndex & parent, int start, int end );
    void slotRowsAboutToBeRemoved ( const QModelIndex & parent, int start, int end ) ;
    void slotRowsInserted ( const QModelIndex & parent, int start, int end ) ;
    void slotRowsRemoved ( const QModelIndex & parent, int start, int end );

    void slotMarkOpenedTimer();
    //void slotSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );

  public Q_SLOTS:
    void opened(const QModelIndex &index);
    void modified(const QModelIndex &index);

  private:
    void updateBackgrounds(bool emitSignals = true);
};

#endif // KATEVIEWDOCUMENTPROXYMODEL_H

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
