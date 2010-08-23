/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#include "katefiletreemodel.h"
#include <KIcon>
#include <QDir>
#include <QList>
#include <KMimeType>
#include <KColorScheme>

#include <ktexteditor/document.h>

#include <kate/application.h>
#include <kate/documentmanager.h>

#include "katefiletreedebug.h"

class ProxyItem {
  public:
    enum Flag { None = 0, Dir = 1, Modified = 2, ModifiedExternally = 4, DeletedExternally = 8 };
    Q_DECLARE_FLAGS(Flags, Flag)
    
    ProxyItem(QString n, ProxyItem *p = 0);
    ~ProxyItem();

    int addChild(ProxyItem *p);
    void remChild(ProxyItem *p);

    ProxyItem *parent();

    ProxyItem *child(int idx);
    int childCount();

    int row();
    
    QString display();
    QString path();
    void setPath(const QString &str);
    
    void setIcon(KIcon i);
    KIcon icon();

    QList<ProxyItem*> &children();

    void setDoc(KTextEditor::Document *doc);
    KTextEditor::Document *doc();

    void setFlags(Flags flags);
    void setFlag(Flag flag);
    void clearFlag(Flag flag);
    bool flag(Flag flag);
    
  private:
    QString m_path;
    ProxyItem *m_parent;
    QList<ProxyItem*> m_children;
    int m_row;
    Flags m_flags;
    
    QString m_display;
    KIcon m_icon;
    KTextEditor::Document *m_doc;
    void initDisplay();
};

class ProxyItemDir : public ProxyItem
{
  public:
    ProxyItemDir(QString n, ProxyItem *p = 0) : ProxyItem(n, p) { setFlag(ProxyItem::Dir); }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ProxyItem::Flags)

ProxyItem::ProxyItem(QString d, ProxyItem *p)
  : m_path(d), m_parent(p), m_row(-1), m_flags(ProxyItem::None), m_doc(0)
{
  kDebug(debugArea()) << "this=" << this << "(" << d << ")";
  initDisplay();
  
  if(p)
    p->addChild(this);

}

ProxyItem::~ProxyItem()
{
  foreach(ProxyItem *item, m_children) {
    delete item;
  }
}

void ProxyItem::initDisplay()
{
  m_display = m_path.section(QDir::separator(), -1, -1);
}

int ProxyItem::addChild(ProxyItem *item)
{
  int item_row = m_children.count();
  item->m_row = item_row;
  m_children.append(item);
  item->m_parent = this;
  kDebug(debugArea()) << "added" << item->display() << "to" << display();
  return item_row;
}

void ProxyItem::remChild(ProxyItem *item)
{
  kDebug(debugArea()) << "remove" << item->path();
  m_children.removeOne(item);
  // fix up item rows
  // could be done a little better, but this'll work.
  for(int i = 0; i < m_children.count(); i++) {
    m_children[i]->m_row = i;
  }

  item->m_parent = 0;
}

ProxyItem *ProxyItem::parent()
{
  return m_parent;
}

ProxyItem *ProxyItem::child(int idx)
{
  if(idx < 0 || idx >= m_children.count()) return 0;
  return m_children[idx];
}

int ProxyItem::childCount()
{
  return m_children.count();
}

int ProxyItem::row()
{
  return m_row;
}

KIcon ProxyItem::icon()
{
  if(m_children.count())
    return KIcon("folder-blue");

  return m_icon;
}

void ProxyItem::setIcon(KIcon i)
{
  m_icon = i;
}

QString ProxyItem::display()
{
  return m_display;
}

QString ProxyItem::path()
{
  return m_path;
}

void ProxyItem::setPath(const QString &p)
{
  m_path = p;
  initDisplay();
}

QList<ProxyItem*> &ProxyItem::children()
{
  return m_children;
}

void ProxyItem::setDoc(KTextEditor::Document *doc)
{
  m_doc = doc;
}

KTextEditor::Document *ProxyItem::doc()
{
  return m_doc;
}

bool ProxyItem::flag(Flag f)
{
  return m_flags & f;
}

void ProxyItem::setFlag(Flag f)
{
  m_flags |= f;
}

void ProxyItem::setFlags(Flags f)
{
  m_flags = f;
}

void ProxyItem::clearFlag(Flag f)
{
  m_flags &= ~f;
}

KateFileTreeModel::KateFileTreeModel(QObject *p)
  : QAbstractItemModel(p),
    m_root(new ProxyItemDir(QString("m_root"), 0))
{
    // add already existing documents
  foreach( KTextEditor::Document* doc, Kate::application()->documentManager()->documents() )
    documentOpened( doc );
}

KateFileTreeModel::~KateFileTreeModel()
{

}

QModelIndex KateFileTreeModel::docIndex(KTextEditor::Document *d)
{
  kDebug(debugArea()) << "BEGIN!";
  ProxyItem *item = m_docmap[d];
  if(!item) {
    kDebug(debugArea()) << "doc" << d << "does not exist";
    return QModelIndex();
  }

  kDebug(debugArea()) << "END!";
  return createIndex(item->row(), 0, item);
}

Qt::ItemFlags KateFileTreeModel::flags( const QModelIndex &index ) const
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled;

  if(!index.isValid())
    return 0;
  
  ProxyItem *item = static_cast<ProxyItem*>(index.internalPointer());
  if(item && !item->childCount()) {
    flags |= Qt::ItemIsSelectable;
  }
  
  return flags;
}

QVariant KateFileTreeModel::data( const QModelIndex &index, int role ) const
{
  //kDebug(debugArea()) << "BEGIN!";
  if(!index.isValid()) {
    kDebug(debugArea()) << "index is invalid!";
    return QVariant();
  }

  ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
  if(!item) {
    kDebug(debugArea()) << "internal pointer is null!";
    return QVariant();
  }
  
  switch(role) {
    case KateFileTreeModel::DocumentRole:
      return QVariant::fromValue(item->doc());
      
    case Qt::DisplayRole:
      return item->display();

    case Qt::DecorationRole:
      return item->icon();

    case Qt::ToolTipRole:
      return item->path();

    case Qt::ForegroundRole: {
      KColorScheme colors(QPalette::Active);
      if(!item->flag(ProxyItem::Dir) && (!item->doc() || item->doc()->openingError())) return colors.foreground(KColorScheme::InactiveText).color();
    } break;
      
    case Qt::BackgroundRole:
      // TODO: do that funky shading the file list does...
      break;
  }

  //kDebug(debugArea()) << "END!";
  return QVariant();
}

QVariant KateFileTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  Q_UNUSED(orientation);
  Q_UNUSED(role);
  
  if(section == 0)
    return QString("a header");
  
  return QVariant();
}

int KateFileTreeModel::rowCount( const QModelIndex &parent ) const
{
  if(!parent.isValid())
    return m_root->childCount();

  ProxyItem *item = static_cast<ProxyItem *>(parent.internalPointer());
  if(!item)
    return 0;

  return item->childCount();
}

int KateFileTreeModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED(parent);
  return 1;
}


QModelIndex KateFileTreeModel::parent( const QModelIndex &index ) const
{
  if(!index.isValid())
    return QModelIndex();

  ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
  if(!item)
    return QModelIndex();

  if(!item->parent())
    return QModelIndex();

  if(item->parent() == m_root)
    return QModelIndex();

  return createIndex(item->parent()->row(), 0, item->parent());
}

QModelIndex KateFileTreeModel::index( int row, int column, const QModelIndex &parent ) const
{
  ProxyItem *p = 0;
  if(column != 0)
    return QModelIndex();
  
  if(!parent.isValid())
    p = m_root;
  else
    p = static_cast<ProxyItem *>(parent.internalPointer());

  if(!p)
    return QModelIndex();

  if(row < 0 || row >= p->childCount())
    return QModelIndex();

  return createIndex(row, 0, p->child(row));
}

bool KateFileTreeModel::hasChildren( const QModelIndex & parent ) const
{
  if(!parent.isValid())
    return m_root->childCount() > 0;

  ProxyItem *item = static_cast<ProxyItem*>(parent.internalPointer());
  if(!item)
    return false;

  return item->childCount() > 0;
}

bool KateFileTreeModel::isDir(const QModelIndex &index)
{
  if(!index.isValid())
    return true;
  
  ProxyItem *item = static_cast<ProxyItem*>(index.internalPointer());
  if(!item)
    return false;
  
  return item->flag(ProxyItem::Dir);
}

void KateFileTreeModel::documentOpened(KTextEditor::Document *doc)
{
  QString path = doc->url().path();
  if(!path.length())
    path = doc->documentName();

  ProxyItem *item = new ProxyItem(path, 0);
  item->setDoc(doc);
  kDebug(debugArea()) << path << "(" << item << ")";
  handleInsert(item);
  setupIcon(item);
  m_docmap[doc] = item;
  connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged(KTextEditor::Document*)));
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(documentModifiedChanged(KTextEditor::Document*)));
  connect(doc, SIGNAL(modifiedOnDisk( KTextEditor::Document*, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason ) ),
          this,  SLOT(documentModifiedOnDisc( KTextEditor::Document*, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason ) ) );

}

void KateFileTreeModel::documentModifiedChanged(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "BEGIN!";
  
  ProxyItem *item = m_docmap[doc];
  if(!item)
    return;

  if(doc->isModified()) {
    item->setFlag(ProxyItem::Modified);
    kDebug(debugArea()) << "modified!";
  }
  else {
    item->clearFlag(ProxyItem::Modified);
    item->clearFlag(ProxyItem::ModifiedExternally);
    item->clearFlag(ProxyItem::DeletedExternally);
    kDebug(debugArea()) << "saved!";
  }

  setupIcon(item);
  
  QModelIndex idx = createIndex(item->row(), 0, item);
  emit dataChanged(idx, idx);

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::documentModifiedOnDisc(KTextEditor::Document *doc, bool modified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason )
{
  Q_UNUSED(modified);
  kDebug(debugArea()) << "BEGIN!";
  ProxyItem *item = m_docmap[doc];
  if(!item)
    return;

  // This didn't do what I thought it did, on an ignore
  // we'd get !modified causing the warning icons to disappear
  //if(!modified) {
  //  item->clearFlag(ProxyItem::ModifiedExternally);
  //  item->clearFlag(ProxyItem::DeletedExternally);
  //}
  
  if(reason == KTextEditor::ModificationInterface::OnDiskDeleted) {
    item->setFlag(ProxyItem::DeletedExternally);
    kDebug(debugArea()) << "deleted!";
  }
  else if(reason == KTextEditor::ModificationInterface::OnDiskModified) {
    item->setFlag(ProxyItem::ModifiedExternally);
    kDebug(debugArea()) << "modified!";
  }
  else if(reason == KTextEditor::ModificationInterface::OnDiskCreated) {
    kDebug(debugArea()) << "created!";
    // with out this, on "reload" we don't get the icons removed :(
    item->clearFlag(ProxyItem::ModifiedExternally);
    item->clearFlag(ProxyItem::DeletedExternally);
  }

  setupIcon(item);
  
  QModelIndex idx = createIndex(item->row(), 0, item);
  emit dataChanged(idx, idx);
  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::handleEmptyParents(ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";
  if(!item || !item->parent()) {
    kDebug(debugArea()) << "parent" << item << "parent" << (item ? item->parent() : 0);
    return;
  }
  
  ProxyItem *parent = item->parent();
  //emit layoutAboutToBeChanged();
  
  kDebug(debugArea()) << "parent" << item->path() << "parent" << parent;
  while(parent) {
    kDebug(debugArea()) << "parent" << parent->display() << "children:" << item->childCount();
    if(!item->childCount()) {
      QModelIndex parent_index = parent == m_root ? QModelIndex() : createIndex(parent->row(), 0, parent);
      beginRemoveRows(parent_index, item->row(), item->row());
      parent->remChild(item);
      endRemoveRows();
      kDebug(debugArea()) << "deleted parent!";
      delete item;
    }
    
    item = parent;
    parent = item->parent();
  }
  
  //emit layoutChanged();
  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::documentClosed(KTextEditor::Document *doc)
{
  QString path = doc->url().path();
  if(!path.length()) path = doc->documentName();
  kDebug(debugArea()) << path << m_docmap[doc];
  if(!m_docmap.contains(doc)) {
    kDebug(debugArea()) << "docmap doesn't contain doc!";
    return;
  }
  
  ProxyItem *node = m_docmap[doc];
  ProxyItem *parent = node->parent();
  
  QModelIndex parent_index = parent == m_root ? QModelIndex() : createIndex(parent->row(), 0, parent);
  beginRemoveRows(parent_index, node->row(), node->row());
    node->parent()->remChild(node);
  endRemoveRows();
  
  delete node;
  handleEmptyParents(parent);
  
  m_docmap.remove(doc);
}

void KateFileTreeModel::documentNameChanged(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "BEGIN!";
  ProxyItem *item = m_docmap[doc];
  QString path = doc->url().path();
  if(!path.length())
    path = doc->documentName();
  
  kDebug(debugArea()) << item->display() << "->" << path << "(" << item << ")";
  
  handleNameChange(item, path);
  
  kDebug(debugArea()) << "END!";
}

ProxyItem *KateFileTreeModel::findRootNode(const QString &name, int r)
{
  QString base = name.section(QDir::separator(), 0, -2);
  foreach(ProxyItem *item, m_root->children()) {
    QString path = item->path().section(QDir::separator(), 0, -r);
    if(path.startsWith("Untitled")) {
      continue;
    }

    if(name.startsWith(path))
      return item;
  }

  return 0;
}

ProxyItem *KateFileTreeModel::findChildNode(ProxyItem *parent, const QString &name)
{
  if(!parent || !parent->childCount())
    return 0;

  foreach(ProxyItem *item, parent->children()) {
    if(item->display() == name) {
      kDebug(debugArea()) << "found:" << name;
      return item;
    }
  }

  kDebug(debugArea()) << "!found:" << name;
  return 0;
}

void KateFileTreeModel::insertItemInto(ProxyItem *root, ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";
  
  QString tail = item->path();
  tail.remove(0, root->path().length());
  QStringList parts = tail.split(QDir::separator(), QString::SkipEmptyParts);
  ProxyItem *ptr = root;
  QStringList current_parts;
  current_parts.append(root->path());

  parts.pop_back();

  kDebug(debugArea()) << "creating tree for" << item->path();
  foreach(QString part, parts) {
    current_parts.append(part);
    ProxyItem *find = findChildNode(ptr, part);
    if(!find) {
      QString new_name = current_parts.join(QDir::separator());
      QModelIndex parent_index = createIndex(ptr->row(), 0, ptr);
      kDebug(debugArea()) << "adding" << part << "to" << ptr->display();
      beginInsertRows(ptr == m_root ? QModelIndex() : parent_index, ptr->childCount(), ptr->childCount());
      ptr = new ProxyItemDir(new_name, ptr);
      endInsertRows();
    }
    else {
      ptr = find;
    }
  }

  kDebug(debugArea()) << "adding" << item->display() << "to" << ptr->display();
  QModelIndex parent_index = createIndex(ptr->row(), 0, ptr);
  beginInsertRows(ptr == m_root ? QModelIndex() : parent_index, ptr->childCount(), ptr->childCount());
    ptr->addChild(item);
  endInsertRows();
  
  kDebug(debugArea()) << "ptr:" << ptr->path();

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::handleInsert(ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";

  if(item->path().startsWith("Untitled")) {
    kDebug(debugArea()) << "empty item";
    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
    m_root->addChild(item);
    endInsertRows();
    return;
  }
  
  ProxyItem *root = findRootNode(item->path());
  if(root) {
    kDebug(debugArea()) << "got a root, inserting into it";
    insertItemInto(root, item);
  } else {
    kDebug(debugArea()) << "creating a new root";

    // trim off trailing file and dir
    QString base = item->path().section(QDir::separator(), 0, -2);

    // create new root
    ProxyItem *new_root = new ProxyItemDir(base, 0);
    
    // add new root to m_root
    kDebug(debugArea()) << "add" << new_root->display() << "to m_root";
    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
      m_root->addChild(new_root);
    endInsertRows();
    
    QModelIndex new_root_index = createIndex(new_root->row(), 0, new_root);
    
    // try and merge existing roots with the new root node.
    kDebug(debugArea()) << "attempting to merge some existing roots";
    foreach(ProxyItem *item, m_root->children()) {
      if(item == new_root)
        continue;
      
      if(item->path().startsWith(base)) {
        kDebug(debugArea()) << "removing" << item->display() << "from m_root";
        beginRemoveRows(QModelIndex(), item->row(), item->row());
          m_root->remChild(item);
        endRemoveRows();

        kDebug(debugArea()) << "adding" << item->display() << "to" << new_root->display();
        beginInsertRows(new_root_index, new_root->childCount(), new_root->childCount());
          new_root->addChild(item);
        endInsertRows();
      }
    }

    // add item to new root
    kDebug(debugArea()) << "adding to new root";
    beginInsertRows(new_root_index, new_root->childCount(), new_root->childCount());
      new_root->addChild(item);
    endInsertRows();

  }

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::handleNameChange(ProxyItem *item, const QString &new_name)
{
  kDebug(debugArea()) << "BEGIN!";
  
  // for some reason we get useless name changes
  if(item->path() == new_name) {
      kDebug(debugArea()) << "bogus name change";
    return;
  }
  
  // in either case (new/change) we want to remove the item from its parent

  ProxyItem *parent = item->parent();
  if(!parent) {
    item->setPath(new_name);
    kDebug(debugArea()) << "item" << item->path() << "does not have a parent?";
    return;
  }

  item->setPath(new_name);

  kDebug(debugArea()) << "removing" << item->display() << "from" << parent->display();
  QModelIndex parent_index = parent == m_root ? QModelIndex() : createIndex(parent->row(), 0, parent);
  beginRemoveRows(parent_index, item->row(), item->row());
    parent->remChild(item);
  endRemoveRows();
  
  // remove empty parent nodes here, recursively.
  handleEmptyParents(parent);
  
  // set new path
  //item->setPath(new_name);

  item->setFlags(ProxyItem::None);
  setupIcon(item);
  
  // new item
  kDebug(debugArea()) << "inserting" << item->display();
  handleInsert(item);

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::setupIcon(ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";
  
  QStringList emblems;
  QString icon_name;
  
  if(item->flag(ProxyItem::Modified)) {
    icon_name = "document-save";
  }
  else {
    KUrl url = item->path();
    icon_name = KMimeType::findByUrl(url, 0, false, true)->iconName();
  }
  
  if(item->flag(ProxyItem::ModifiedExternally) || item->flag(ProxyItem::DeletedExternally)) {
    emblems << "emblem-important";
    kDebug(debugArea()) << "modified!";
  }

  item->setIcon(KIcon(icon_name, 0, emblems));

  kDebug(debugArea()) << "END!";
}

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
