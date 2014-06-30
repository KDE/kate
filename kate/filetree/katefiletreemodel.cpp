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
#include <QFileInfo>
#include <QList>
#include <KMimeType>
#include <KColorScheme>
#include <KColorUtils>
#include <klocale.h>

#include <ktexteditor/document.h>

#include <kate/application.h>
#include <kate/documentmanager.h>

#include "katefiletreedebug.h"

class ProxyItemDir;
class ProxyItem {
  friend class KateFileTreeModel;
  
  public:
    enum Flag { None = 0, Dir = 1, Modified = 2, ModifiedExternally = 4, DeletedExternally = 8, Empty = 16, ShowFullPath = 32, Host=64 };
    Q_DECLARE_FLAGS(Flags, Flag)
    
    ProxyItem(QString n, ProxyItemDir *p = 0, Flags f = ProxyItem::None);
    ~ProxyItem();

    int addChild(ProxyItem *p);
    void remChild(ProxyItem *p);

    ProxyItemDir *parent();

    ProxyItem *child(int idx);
    int childCount();

    int row();
    
    QString display();
    QString path();
    QString documentName();
    void setPath(const QString &str);
    
    void setIcon(KIcon i);
    KIcon icon();

    QList<ProxyItem*> &children();

    void setDoc(KTextEditor::Document *doc);
    KTextEditor::Document *doc();
    QList<KTextEditor::Document*> docTree() const;

    void setFlags(Flags flags);
    void setFlag(Flag flag);
    void clearFlag(Flag flag);
    bool flag(Flag flag);
    void setHost(const QString &host);
    const QString& host() const;
    
  private:
    QString m_path;
    QString m_documentName;
    ProxyItemDir *m_parent;
    QList<ProxyItem*> m_children;
    int m_row;
    Flags m_flags;
    
    QString m_display;
    KIcon m_icon;
    KTextEditor::Document *m_doc;
    QString m_host;
  protected:
    void initDisplay();
};

QDebug operator<<(QDebug dbg, ProxyItem *item)
{
  if(!item) {
    dbg.nospace() << "ProxyItem(0x0) ";
    return dbg.maybeSpace();
  }
  
  void *parent = static_cast<void *>(item->parent());
  
  dbg.nospace() << "ProxyItem(" << (void*)item << ",";
  dbg.nospace() << parent << "," << item->row() << ",";
  dbg.nospace() << item->doc() << "," << item->path() << ") ";
  return dbg.maybeSpace();
}


class ProxyItemDir : public ProxyItem
{
  public:
    ProxyItemDir(QString n, ProxyItemDir *p = 0) : ProxyItem(n, p) { setFlag(ProxyItem::Dir); initDisplay();}
};

QDebug operator<<(QDebug dbg, ProxyItemDir *item)
{
  if(!item) {
    dbg.nospace() << "ProxyItemDir(0x0) ";
    return dbg.maybeSpace();
  }
  
  void *parent = static_cast<void *>(item->parent());
  
  dbg.nospace() << "ProxyItemDir(" << (void*)item << ",";
  dbg.nospace() << parent << "," << item->row() << ",";
  dbg.nospace() << item->path() << ", children:" << item->childCount() << ") ";
  return dbg.maybeSpace();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(ProxyItem::Flags)

ProxyItem::ProxyItem(QString d, ProxyItemDir *p, ProxyItem::Flags f)
  : m_path(d), m_parent(p), m_row(-1), m_flags(f), m_doc(0)
{
  kDebug(debugArea()) << this;
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
  // triggers only if this is a top level node and the root has the show full path flag set.
  if (flag(ProxyItem::Dir) && m_parent && !m_parent->m_parent && m_parent->flag(ProxyItem::ShowFullPath)) {
    m_display = m_path;
    if(m_display.startsWith(QDir::homePath())) {
      m_display.replace(0, QDir::homePath().length(), "~");
    }
  } else {
    m_display = m_path.section(QLatin1Char('/'), -1, -1);
    if (flag(ProxyItem::Host) && (!m_parent || (m_parent && !m_parent->m_parent))) {
      QString hostPrefix="["+host()+"]";
      if (hostPrefix!=m_display)
        m_display=hostPrefix+m_display;
    }
  }
    
}

int ProxyItem::addChild(ProxyItem *item)
{
  int item_row = m_children.count();
  item->m_row = item_row;
  m_children.append(item);
  item->m_parent = static_cast<ProxyItemDir*>(this);

  item->initDisplay();

  kDebug(debugArea()) << "added" << item << "to" << item->m_parent;
  return item_row;
}

void ProxyItem::remChild(ProxyItem *item)
{
  kDebug(debugArea()) << "remove" << item << "from" << static_cast<ProxyItemDir*>(this);
  m_children.removeOne(item);
  // fix up item rows
  // could be done a little better, but this'll work.
  for(int i = 0; i < m_children.count(); i++) {
    m_children[i]->m_row = i;
  }

  item->m_parent = 0;
}

ProxyItemDir *ProxyItem::parent()
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
    return KIcon("folder");

  return m_icon;
}

void ProxyItem::setIcon(KIcon i)
{
  m_icon = i;
}

QString ProxyItem::documentName() {
  return m_documentName;
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
  if (!doc)
    m_documentName=QString();
  else {
        QString docName=doc->documentName();
        if (flag(ProxyItem::Host))
          m_documentName="["+m_host+"]"+docName;
        else
          m_documentName=docName;
  }
}

KTextEditor::Document *ProxyItem::doc()
{
  return m_doc;
}

QList<KTextEditor::Document*> ProxyItem::docTree() const
{
  QList<KTextEditor::Document*> result;
  if (m_doc) {
    result.append(m_doc);
  }
  for (QList<ProxyItem*>::const_iterator iter = m_children.constBegin(); iter != m_children.constEnd(); ++iter) {
    result.append((*iter)->docTree());
  }
  return result;
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

void ProxyItem::setHost(const QString& host) 
{
  QString docName;
  if (m_doc)
    docName=m_doc->documentName();
  if (host.isEmpty()) {
    clearFlag(Host);
    m_documentName=docName;
  } else {
    setFlag(Host);
    m_documentName="["+host+"]"+docName;
  }
  m_host=host;
  
  initDisplay();
}

const QString& ProxyItem::host() const
{
  return m_host;
}

KateFileTreeModel::KateFileTreeModel(QObject *p)
  : QAbstractItemModel(p),
    m_root(new ProxyItemDir(QString("m_root"), 0))
{

  // setup default settings
  // session init will set these all soon
  KColorScheme colors(QPalette::Active);
  QColor bg = colors.background().color();
  m_editShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::ActiveText).color(), 0.5);
  m_viewShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::VisitedText).color(), 0.5);
  m_shadingEnabled = true;
  m_listMode = false;
  
  initModel();
}

KateFileTreeModel::~KateFileTreeModel()
{

}

bool KateFileTreeModel::shadingEnabled()
{
  return m_shadingEnabled;
}

void KateFileTreeModel::setShadingEnabled(bool se)
{
  if(m_shadingEnabled != se) {
    updateBackgrounds(true);
    m_shadingEnabled = se;
  }
}

QColor KateFileTreeModel::editShade()
{
  return m_editShade;
}

void KateFileTreeModel::setEditShade(QColor es)
{
  m_editShade = es;
}

QColor KateFileTreeModel::viewShade()
{
  return m_viewShade;
}

void KateFileTreeModel::setViewShade(QColor vs)
{
  m_viewShade = vs;
}

bool KateFileTreeModel::showFullPathOnRoots(void)
{
  return m_root->flag(ProxyItem::ShowFullPath);
}

void KateFileTreeModel::setShowFullPathOnRoots(bool s)
{
  if(s)
    m_root->setFlag(ProxyItem::ShowFullPath);
  else
    m_root->clearFlag(ProxyItem::ShowFullPath);
  
  foreach(ProxyItem *root, m_root->children()) {
    root->initDisplay();
  }
}

// FIXME: optimize this later to insert all at once if possible
// maybe add a "bool emitSignals" to documentOpened
// and possibly use beginResetModel here? I dunno.

void KateFileTreeModel::initModel()
{
  // add already existing documents
  foreach( KTextEditor::Document* doc, Kate::application()->documentManager()->documents() )
    documentOpened( doc );
}

void KateFileTreeModel::clearModel()
{
  // remove all items
  // can safely ignore documentClosed here

  beginRemoveRows(QModelIndex(), 0, m_root->childCount()-1);

  delete m_root;
  m_root = new ProxyItemDir(QString("m_root"), 0);

  m_docmap.clear();
  m_viewHistory.clear();
  m_editHistory.clear();
  m_brushes.clear();

  endRemoveRows();
}

void KateFileTreeModel::connectDocument(const KTextEditor::Document *doc)
{
  connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged(KTextEditor::Document*)));
  connect(doc, SIGNAL(documentUrlChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged(KTextEditor::Document*)));
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(documentModifiedChanged(KTextEditor::Document*)));
  connect(doc, SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
          this,  SLOT(documentModifiedOnDisc(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)) );
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

#include "metatype_qlist_ktexteditor_document_pointer.h"

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
    case KateFileTreeModel::PathRole:
      // allow to sort with hostname + path, bug 271488
      return (item->doc() && !item->doc()->url().isEmpty()) ? item->doc()->url().pathOrUrl() : item->path();
      
    case KateFileTreeModel::DocumentRole:
      return QVariant::fromValue(item->doc());

    case KateFileTreeModel::OpeningOrderRole:
      return item->row();

    case KateFileTreeModel::DocumentTreeRole:
      return QVariant::fromValue(item->docTree());

    case Qt::DisplayRole:
      // in list mode we want to use kate's fancy names.
      if(m_listMode) {
        return item->documentName();
      } else
        return item->display();

    case Qt::DecorationRole:
      return item->icon();

    case Qt::ToolTipRole: {
      QString tooltip = item->path();
      if (item->flag(ProxyItem::DeletedExternally) || item->flag(ProxyItem::ModifiedExternally)) {
        tooltip = i18nc("%1 is the full path", "<p><b>%1</b></p><p>The document has been modified by another application.</p>", item->path());
      }

      return tooltip;
    }

    case Qt::ForegroundRole: {
      KColorScheme colors(QPalette::Active);
      if(!item->flag(ProxyItem::Dir) && (!item->doc() || item->doc()->openingError())) return colors.foreground(KColorScheme::InactiveText).color();
    } break;
      
    case Qt::BackgroundRole:
      // TODO: do that funky shading the file list does...
      if(m_shadingEnabled && m_brushes.contains(item))
        return m_brushes[item];
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
  if(!item) {
    kDebug(debugArea()) << "internal pointer is invalid";
    return 0;
  }

  return item->childCount();
}

int KateFileTreeModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED(parent);
  return 1;
}


QModelIndex KateFileTreeModel::parent( const QModelIndex &index ) const
{
  if(!index.isValid()) {
    kDebug(debugArea()) << "index is invalid";
    return QModelIndex();
  }
  
  ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
  if(!item) {
    kDebug(debugArea()) << "internal pointer is invalid";
    return QModelIndex();
  }

  if(!item->parent()) {
    kDebug(debugArea()) << "parent pointer is null";
    return QModelIndex();
  }

  if(item->parent() == m_root)
    return QModelIndex();

  return createIndex(item->parent()->row(), 0, item->parent());
}

QModelIndex KateFileTreeModel::index( int row, int column, const QModelIndex &parent ) const
{
  ProxyItem *p = 0;
  if(column != 0) {
    kDebug(debugArea()) << "column is invalid";
    return QModelIndex();
  }
  
  if(!parent.isValid())
    p = m_root;
  else
    p = static_cast<ProxyItem *>(parent.internalPointer());

  if(!p) {
    kDebug(debugArea()) << "internal pointer is invalid";
    return QModelIndex();
  }
  
  if(row < 0 || row >= p->childCount()) {
    kDebug(debugArea()) << "row is out of bounds (" << row << " < 0 || " << row << " >= " << p->childCount() << ")";
    return QModelIndex();
  }

  return createIndex(row, 0, p->child(row));
}

bool KateFileTreeModel::hasChildren( const QModelIndex & parent ) const
{
  if(!parent.isValid())
    return m_root->childCount() > 0;

  ProxyItem *item = static_cast<ProxyItem*>(parent.internalPointer());
  if(!item) {
    kDebug(debugArea()) << "internal pointer is null";
    return false;
  }
  
  return item->childCount() > 0;
}

bool KateFileTreeModel::isDir(const QModelIndex &index)
{
  if(!index.isValid())
    return true;
  
  ProxyItem *item = static_cast<ProxyItem*>(index.internalPointer());
  if(!item) {
    kDebug(debugArea()) << "internal pointer is null";
    return false;
  }
  
  return item->flag(ProxyItem::Dir);
}

bool KateFileTreeModel::listMode()
{
  return m_listMode;
}

void KateFileTreeModel::setListMode(bool lm)
{
  if(lm != m_listMode) {
    m_listMode = lm;

    clearModel();
    initModel();
  }
}

void KateFileTreeModel::documentOpened(KTextEditor::Document *doc)
{
  QString path = doc->url().path();
  bool isEmpty = false;
  QString host;
  
  if(doc->url().isEmpty()) {
    path = doc->documentName();
    isEmpty = true;
  } else {
    host=doc->url().host();
    if (!host.isEmpty())
      path="["+host+"]"+path;  
    
  }
  
  ProxyItem *item = new ProxyItem(path, 0);
  
  if(isEmpty)
    item->setFlag(ProxyItem::Empty);
  
  m_debugmap[item] = item;
  
  item->setDoc(doc);
  item->setHost(host);
  kDebug(debugArea()) << "before add:" << item;
  setupIcon(item);
  handleInsert(item);
  m_docmap[doc] = item;
  connectDocument(doc);

  kDebug(debugArea()) << "after add:" << item;
  
}

void KateFileTreeModel::documentsOpened(const QList<KTextEditor::Document*> &docs)
{
  foreach(KTextEditor::Document *doc, docs) {
    if (m_docmap.contains(doc)) {
      documentNameChanged(doc);
    } else {
      documentOpened(doc);
    }
  }
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
  if(!modified) {
    item->clearFlag(ProxyItem::ModifiedExternally);
    item->clearFlag(ProxyItem::DeletedExternally);
  } else {
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
  }
  
  setupIcon(item);
  
  QModelIndex idx = createIndex(item->row(), 0, item);
  emit dataChanged(idx, idx);
  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::documentActivated(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "BEGIN!";

  if(!m_docmap.contains(doc)) {
    kDebug(debugArea()) << "invalid doc" << doc;
    return;
  }

  ProxyItem *item = m_docmap[doc];
  kDebug(debugArea()) << "adding viewHistory" << item;
  m_viewHistory.removeAll(item);
  m_viewHistory.prepend(item);

  while (m_viewHistory.count() > 10) m_viewHistory.removeLast();

  updateBackgrounds();
  
  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::documentEdited(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "BEGIN!";

  if(!m_docmap.contains(doc)) {
    kDebug(debugArea()) << "invalid doc" << doc;
    return;
  }

  ProxyItem *item = m_docmap[doc];
  kDebug(debugArea()) << "adding editHistory" << item;
  m_editHistory.removeAll(item);
  m_editHistory.prepend(item);
  while (m_editHistory.count() > 10) m_editHistory.removeLast();

  updateBackgrounds();
  
  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::slotAboutToDeleteDocuments(const QList<KTextEditor::Document*> &docs)
{
  foreach (const KTextEditor::Document *doc, docs) {
    disconnect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged(KTextEditor::Document*)));
    disconnect(doc, SIGNAL(documentUrlChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged(KTextEditor::Document*)));
    disconnect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(documentModifiedChanged(KTextEditor::Document*)));
    disconnect(doc, SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
               this,  SLOT(documentModifiedOnDisc(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)) );
  }
}

void KateFileTreeModel::slotDocumentsDeleted(const QList<KTextEditor::Document*> &docs)
{
  foreach (const KTextEditor::Document *doc, docs) {
    connectDocument(doc);
  }
}

class EditViewCount
{
  public:
    EditViewCount(): edit(0), view(0)
    {}
    int edit;
    int view;
};

void KateFileTreeModel::updateBackgrounds(bool force)
{
  if (!m_shadingEnabled && !force) return;
  
  kDebug(debugArea()) << "BEGIN!";
  
  QMap <ProxyItem *, EditViewCount> helper;
  int i = 1;
  
  foreach (ProxyItem *item, m_viewHistory)
  {
    helper[item].view = i;
    if(!m_debugmap.contains(item)) {
      kDebug(debugArea()) << "m_viewHistory contains an item that doesn't exist?" << item;
    }
    i++;
  }
  
  i = 1;
  foreach (ProxyItem *item, m_editHistory)
  {
    helper[item].edit = i;
    if(!m_debugmap.contains(item)) {
      kDebug(debugArea()) << "m_editHistory contains an item that doesn't exist?" << item;
    }
    i++;
  }
  
  kDebug(debugArea()) << "m_editHistory contains " << m_editHistory.count()<<" elements";
        
  QMap<ProxyItem *, QBrush> oldBrushes = m_brushes;
  m_brushes.clear();
  
  int hc = m_viewHistory.count();
  int ec = m_editHistory.count();
  
  for (QMap<ProxyItem *, EditViewCount>::iterator it = helper.begin();it != helper.end();++it)
  {
    QColor shade( m_viewShade );
    QColor eshade( m_editShade );
    
    if (it.value().edit > 0)
    {
      int v = hc - it.value().view;
      int e = ec - it.value().edit + 1;
      
      e = e * e;

      int n = qMax(v + e, 1);
      
      shade.setRgb(
        ((shade.red()*v) + (eshade.red()*e)) / n,
        ((shade.green()*v) + (eshade.green()*e)) / n,
        ((shade.blue()*v) + (eshade.blue()*e)) / n
      );
    }

    // blend in the shade color; latest is most colored.
    double t = double(hc - it.value().view + 1) / double(hc);

    m_brushes[it.key()] = QBrush(KColorUtils::mix(QPalette().color(QPalette::Base), shade, t));
//     kdDebug()<<"m_brushes[it.key()]"<<it.key()<<m_brushes[it.key()];
  }

  foreach(ProxyItem *item, m_brushes.keys())
  {
    oldBrushes.remove(item);
    QModelIndex idx = createIndex(item->row(), 0, item);
    dataChanged(idx, idx);
  }
  
  foreach(ProxyItem *item, oldBrushes.keys())
  {
    QModelIndex idx = createIndex(item->row(), 0, item);
    dataChanged(idx, idx);
  }

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::handleEmptyParents(ProxyItemDir *item)
{
  kDebug(debugArea()) << "BEGIN!";
  Q_ASSERT(item != 0);
  
  if(!item || !item->parent()) {
    kDebug(debugArea()) << "parent" << item << "grandparent" << (item ? item->parent() : 0);
    return;
  }
  
  ProxyItemDir *parent = item->parent();
  //emit layoutAboutToBeChanged();
  
  kDebug(debugArea()) << "item" << item << "parent" << parent;
  while(parent) {
    
    kDebug(debugArea()) << "item" << item << "parent" << parent;
    if(!item->childCount()) {
      QModelIndex parent_index = parent == m_root ? QModelIndex() : createIndex(parent->row(), 0, parent);
      beginRemoveRows(parent_index, item->row(), item->row());
      parent->remChild(item);
      endRemoveRows();
      kDebug(debugArea()) << "deleted" << item;
      delete item;
    }
    else {
      // breakout early, if this node isn't empty, theres no use in checking its parents
      kDebug(debugArea()) << "END!";
      return;
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
  
  if(!m_docmap.contains(doc)) {
    kDebug(debugArea()) << "docmap doesn't contain doc" << doc;
    return;
  }
  
  kDebug(debugArea()) << path << m_docmap[doc];

  if(m_shadingEnabled) {
    ProxyItem *toRemove = m_docmap[doc];
    if(m_brushes.contains(toRemove)) {
      m_brushes.remove(toRemove);
      kDebug(debugArea()) << "removing brush" << toRemove;
    }

    if(m_viewHistory.contains(toRemove)) {
      m_viewHistory.removeAll(toRemove);
      kDebug(debugArea()) << "removing viewHistory" << toRemove;
    }

    if(m_editHistory.contains(toRemove)) {
      m_editHistory.removeAll(toRemove);
      kDebug(debugArea()) << "removing editHistory" << toRemove;
    }
  }

  ProxyItem *node = m_docmap[doc];
  ProxyItemDir *parent = node->parent();
  
  QModelIndex parent_index = parent == m_root ? QModelIndex() : createIndex(parent->row(), 0, parent);
  beginRemoveRows(parent_index, node->row(), node->row());
    node->parent()->remChild(node);
  endRemoveRows();

  m_debugmap.remove(node);
  
  delete node;
  handleEmptyParents(parent);
  
  m_docmap.remove(doc);
}

void KateFileTreeModel::documentNameChanged(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "BEGIN!";

  if(!m_docmap.contains(doc)) {
    kDebug(debugArea()) << "docmap doesn't contain doc" << doc;
    return;
  }
  
  ProxyItem *item = m_docmap[doc];
  QString path = doc->url().path();
  QString host;
  if(doc->url().isEmpty()) {
    kDebug(debugArea()) << "change to unnamed item";
    path = doc->documentName();
    item->setFlag(ProxyItem::Empty);
  }
  else {
    item->clearFlag(ProxyItem::Empty);
    host=doc->url().host();
    if (!host.isEmpty())
      path="["+host+"]"+path;
  }

  kDebug(debugArea()) << item;
  kDebug(debugArea()) << item->display() << "->" << path;

  
  if(m_shadingEnabled) {
    ProxyItem *toRemove = m_docmap[doc];
    if(m_brushes.contains(toRemove)) {
      QBrush brush=m_brushes[toRemove];
      m_brushes.remove(toRemove);
      m_brushes.insert(item,brush);
      kDebug(debugArea()) << "removing brush" << toRemove;
    }

    if(m_viewHistory.contains(toRemove)) {
      int idx=m_viewHistory.indexOf(toRemove);
      if (idx!=-1)
        m_viewHistory.replace(idx,item);
      kDebug(debugArea()) << "removing/replacing view history" << toRemove;
    }

    if(m_editHistory.contains(toRemove)) {
      int idx=m_editHistory.indexOf(toRemove);
      if (idx!=-1)
        m_editHistory.replace(idx,item);
      kDebug(debugArea()) << "removing/replacing edit history" << toRemove;
    }
  }
  
  handleNameChange(item, path, host);
  
  triggerViewChangeAfterNameChange();
  
  kDebug(debugArea()) << "END!";
}

ProxyItemDir *KateFileTreeModel::findRootNode(const QString &name, int r)
{
  QString base = name.section(QLatin1Char('/'), 0, -2);
  foreach(ProxyItem *item, m_root->children()) {
    QString path = item->path().section(QLatin1Char('/'), 0, -r);
    if (!item->flag(ProxyItem::Host) && !QFileInfo(path).isAbsolute()) {
      continue;
    }

    // make sure we're actually matching against the right dir,
    // previously the check below would match /foo/xy against /foo/x
    // and return /foo/x rather than /foo/xy
    // this seems a bit hackish, but is the simplest way to solve the
    // current issue.
    path += QLatin1Char('/');

    if(name.startsWith(path) && item->flag(ProxyItem::Dir)) {
      return static_cast<ProxyItemDir*>(item);
    }
  }

  return 0;
}

ProxyItemDir *KateFileTreeModel::findChildNode(ProxyItemDir *parent, const QString &name)
{
  Q_ASSERT(parent != 0);
  
  if(!parent || !parent->childCount()) {
    kDebug(debugArea()) << "invalid parent or no children" << parent;
    return 0;
  }

  foreach(ProxyItem *item, parent->children()) {
    if(item->display() == name) {
      if(!item->flag(ProxyItem::Dir)) {
        kDebug(debugArea()) << "found" << item << "but its not a dir?";
        return 0;
      }
      
      kDebug(debugArea()) << "found" << item;
      return static_cast<ProxyItemDir*>(item);
    }
  }

  kDebug(debugArea()) << "!found:" << name;
  return 0;
}

void KateFileTreeModel::insertItemInto(ProxyItemDir *root, ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";

  Q_ASSERT(root != 0);
  Q_ASSERT(item != 0);
  
  QString sep;
  QString tail = item->path();
  tail.remove(0, root->path().length());
  QStringList parts = tail.split(QLatin1Char('/'), QString::SkipEmptyParts);
  ProxyItemDir *ptr = root;
  QStringList current_parts;
  current_parts.append(root->path());

  // seems this can be empty, see bug 286191
  if (!parts.isEmpty())
    parts.pop_back();

  kDebug(debugArea()) << "creating tree for" << item;
  foreach(const QString &part, parts) {
    current_parts.append(part);
    ProxyItemDir *find = findChildNode(ptr, part);
    if(!find) {
      QString new_name = current_parts.join(QLatin1String("/"));
      QModelIndex parent_index = createIndex(ptr->row(), 0, ptr);
      kDebug(debugArea()) << "adding" << part << "to" << ptr;
      beginInsertRows(ptr == m_root ? QModelIndex() : parent_index, ptr->childCount(), ptr->childCount());
      ptr = new ProxyItemDir(new_name, ptr);
      endInsertRows();
    }
    else {
        ptr = find;
    }
  }

  kDebug(debugArea()) << "adding" << item << "to" << ptr;
  QModelIndex parent_index = createIndex(ptr->row(), 0, ptr);
  beginInsertRows(ptr == m_root ? QModelIndex() : parent_index, ptr->childCount(), ptr->childCount());
    ptr->addChild(item);
  endInsertRows();

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::handleInsert(ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";

  Q_ASSERT(item != 0);
  
  if(m_listMode) {
    kDebug(debugArea()) << "list mode, inserting into m_root";
    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
    m_root->addChild(item);
    endInsertRows();
    return;
  }
  
  if(item->flag(ProxyItem::Empty)) {
    kDebug(debugArea()) << "empty item";
    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
    m_root->addChild(item);
    endInsertRows();
    return;
  }
  
  ProxyItemDir *root = findRootNode(item->path());
  if(root) {
    kDebug(debugArea()) << "got a root, inserting into it";
    insertItemInto(root, item);
  } else {
    kDebug(debugArea()) << "creating a new root";

    // trim off trailing file and dir
    QString base = item->path().section(QLatin1Char('/'), 0, -2);

    // create new root
    ProxyItemDir *new_root = new ProxyItemDir(base, 0);
    new_root->setHost(item->host());
    
    // add new root to m_root
    kDebug(debugArea()) << "add" << new_root << "to m_root";
    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
      m_root->addChild(new_root);
    endInsertRows();
    
    // same fix as in findRootNode, try to match a full dir, instead of a partial path
    base += QLatin1Char('/');
    
    // try and merge existing roots with the new root node.
    kDebug(debugArea()) << "attempting to merge some existing roots";
    foreach(ProxyItem *root, m_root->children()) {
      if(root == new_root || !root->flag(ProxyItem::Dir))
        continue;
      
      if(root->path().startsWith(base)) {
        kDebug(debugArea()) << "removing" << root << "from m_root";
        beginRemoveRows(QModelIndex(), root->row(), root->row());
          m_root->remChild(root);
        endRemoveRows();

        kDebug(debugArea()) << "adding" << root << "to" << new_root;
        //beginInsertRows(new_root_index, new_root->childCount(), new_root->childCount());
          // this can't use new_root->addChild directly, or it'll potentially miss a bunch of subdirs
          insertItemInto(new_root, root);
        //endInsertRows();
      }
    }

    // add item to new root
    kDebug(debugArea()) << "adding" << item << "to" << new_root;
    // have to call begin/endInsertRows here, or the new item won't show up.
    QModelIndex new_root_index = createIndex(new_root->row(), 0, new_root);
    beginInsertRows(new_root_index, new_root->childCount(), new_root->childCount());
      new_root->addChild(item);
    endInsertRows();

  }

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::handleNameChange(ProxyItem *item, const QString &new_name, const QString& new_host)
{
  kDebug(debugArea()) << "BEGIN!";

  Q_ASSERT(item != 0);
  
  if(m_listMode) {
    item->setPath(new_name);
    item->setHost(new_host);
    QModelIndex idx = createIndex(item->row(), 0, item);
    setupIcon(item);
    emit dataChanged(idx, idx);
    kDebug(debugArea()) << "list mode, short circuit";
    return;
  }
  
  // for some reason we get useless name changes
  if(item->path() == new_name) {
    kDebug(debugArea()) << "bogus name change";
    return;
  }
  
  // in either case (new/change) we want to remove the item from its parent

  ProxyItemDir *parent = item->parent();
  if(!parent) {
    item->setPath(new_name);
    item->setHost(new_host);
    kDebug(debugArea()) << "ERROR: item" << item << "does not have a parent?";
    return;
  }

  item->setPath(new_name);
  item->setHost(new_host);

  kDebug(debugArea()) << "removing" << item << "from" << parent;
  QModelIndex parent_index = parent == m_root ? QModelIndex() : createIndex(parent->row(), 0, parent);
  beginRemoveRows(parent_index, item->row(), item->row());
    parent->remChild(item);
  endRemoveRows();
  
  // remove empty parent nodes here, recursively.
  handleEmptyParents(parent);
  
  // set new path
  //item->setPath(new_name);

  // clear all but Empty flag
  if(item->flag(ProxyItem::Empty))
    item->setFlags(ProxyItem::Empty);
  else
    item->setFlags(ProxyItem::None);
  
  setupIcon(item);
  
  // new item
  kDebug(debugArea()) << "inserting" << item;
  handleInsert(item);

  kDebug(debugArea()) << "END!";
}

void KateFileTreeModel::setupIcon(ProxyItem *item)
{
  kDebug(debugArea()) << "BEGIN!";

  Q_ASSERT(item != 0);
  
  QStringList emblems;
  QString icon_name;
  
  if(item->flag(ProxyItem::Modified)) {
    icon_name = "document-save";
  }
  else {
    KUrl url = item->path();
    icon_name = KMimeType::findByUrl(url, 0, true, true)->iconName();
  }
  
  if(item->flag(ProxyItem::ModifiedExternally) || item->flag(ProxyItem::DeletedExternally)) {
    emblems << "emblem-important";
    kDebug(debugArea()) << "modified!";
  }

  item->setIcon(KIcon(icon_name, 0, emblems));

  kDebug(debugArea()) << "END!";
}

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
