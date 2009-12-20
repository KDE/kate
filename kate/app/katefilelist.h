/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001,2006 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2007 Anders Lund <anders@alweb.dk>

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

#ifndef __KATE_FILELIST_H__
#define __KATE_FILELIST_H__


#include <KSelectAction>
#include <KActionCollection>
#include <KConfigGroup>

#include <QListView>

#include "kateviewdocumentproxymodel.h"
#include "katedocmanager.h"

class KateFileList: public QListView
{
    Q_OBJECT

  friend class KateFileListConfigPage;

  public:
    KateFileList(QWidget * parent, KActionCollection *actionCollection);
    virtual ~KateFileList();
    enum SortType {
      SortOpening = KateDocManager::OpeningOrderRole,
      SortName = Qt::DisplayRole,
      SortUrl = KateDocManager::UrlRole,
      SortCustom = KateViewDocumentProxyModel::CustomOrderRole };

    int sortRole();

    void setShadingEnabled(bool enable);
    bool shadingEnabled() const;

    const QColor& editShade() const;
    void setEditShade( const QColor &shade );

    const QColor& viewShade() const;
    void setViewShade( const QColor &shade );

  private:
    QAction* m_windowNext;
    QAction* m_windowPrev;
    QAction* m_filelistCloseDocument;
    QAction* m_filelistCloseDocumentOther;
    KSelectAction* m_sortAction;
    QPersistentModelIndex m_previouslySelected;
    QPersistentModelIndex m_indexContextMenu;
    int m_sortType;

  public Q_SLOTS:
    void slotNextDocument();
    void slotPrevDocument();
    void slotDocumentClose();
    void slotDocumentCloseOther();
    void setSortRole(int role);
    void setSortRoleFromAction(QAction*);

  protected:
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent ( QMouseEvent * event );
    virtual void contextMenuEvent ( QContextMenuEvent * event );

  Q_SIGNALS:
    void closeDocument(KTextEditor::Document*);
    void closeOtherDocument(KTextEditor::Document*);
};

class KateFileListConfigPage : public QWidget {
  Q_OBJECT
  public:
    KateFileListConfigPage( QWidget* parent=0, KateFileList *fl=0 );
    ~KateFileListConfigPage() {};

    void apply();
    void reload();

  Q_SIGNALS:
    void changed();

  private slots:
    void slotMyChanged();

  private:
    class QGroupBox *gbEnableShading;
    class KColorButton *kcbViewShade, *kcbEditShade;
    class QLabel *lEditShade, *lViewShade, *lSort;
    class KComboBox *cmbSort;
    KateFileList *m_filelist;

    bool m_changed;
};

#endif //__KATE_FILELIST_H__

// kate: space-indent on; indent-width 2; replace-tabs on;
