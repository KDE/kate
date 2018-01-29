/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QMimeData>
#include <QMimeDatabase>
#include <QIcon>
#include <QStack>

#include <KColorScheme>
#include <KColorUtils>
#include <KLocalizedString>
#include <KIconUtils>

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/application.h>

#include "katefiletreedebug.h"

class ProxyItemDir;
class ProxyItem
{
    friend class KateFileTreeModel;

public:
    enum Flag { None = 0, Dir = 1, Modified = 2, ModifiedExternally = 4, DeletedExternally = 8, Empty = 16, ShowFullPath = 32, Host = 64 };
    Q_DECLARE_FLAGS(Flags, Flag)

    ProxyItem(const QString &n, ProxyItemDir *p = nullptr, Flags f = ProxyItem::None);
    ~ProxyItem();

    int addChild(ProxyItem *p);
    void remChild(ProxyItem *p);

    ProxyItemDir *parent() const;

    ProxyItem *child(int idx) const;
    int childCount() const;

    int row() const;

    const QString &display() const;
    const QString &documentName() const;

    const QString &path() const;
    void setPath(const QString &str);

    void setHost(const QString &host);
    const QString &host() const;

    void setIcon(const QIcon &i);
    const QIcon &icon() const;

    const QList<ProxyItem *> &children() const;
    QList<ProxyItem *> &children();

    void setDoc(KTextEditor::Document *doc);
    KTextEditor::Document *doc() const;

    /**
     * the view usess this to close all the documents under the folder
     * @returns list of all the (nested) documents under this node
     */
    QList<KTextEditor::Document *> docTree() const;

    void setFlags(Flags flags);
    void setFlag(Flag flag);
    void clearFlag(Flag flag);
    bool flag(Flag flag) const;

private:
    QString m_path;
    QString m_documentName;
    ProxyItemDir *m_parent;
    QList<ProxyItem *> m_children;
    int m_row;
    Flags m_flags;

    QString m_display;
    QIcon m_icon;
    KTextEditor::Document *m_doc;
    QString m_host;

protected:
    void updateDisplay();
    void updateDocumentName();
};

QDebug operator<<(QDebug dbg, ProxyItem *item)
{
    if (!item) {
        dbg.nospace() << "ProxyItem(0x0) ";
        return dbg.maybeSpace();
    }

    const void *parent = static_cast<void *>(item->parent());

    dbg.nospace() << "ProxyItem(" << (void *)item << ",";
    dbg.nospace() << parent << "," << item->row() << ",";
    dbg.nospace() << item->doc() << "," << item->path() << ") ";
    return dbg.maybeSpace();
}

class ProxyItemDir : public ProxyItem
{
public:
    ProxyItemDir(QString n, ProxyItemDir *p = nullptr) : ProxyItem(n, p) {
        setFlag(ProxyItem::Dir);
        updateDisplay();

        setIcon(QIcon::fromTheme(QLatin1String("folder")));
    }
};

QDebug operator<<(QDebug dbg, ProxyItemDir *item)
{
    if (!item) {
        dbg.nospace() << "ProxyItemDir(0x0) ";
        return dbg.maybeSpace();
    }

    const void *parent = static_cast<void *>(item->parent());

    dbg.nospace() << "ProxyItemDir(" << (void *)item << ",";
    dbg.nospace() << parent << "," << item->row() << ",";
    dbg.nospace() << item->path() << ", children:" << item->childCount() << ") ";
    return dbg.maybeSpace();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(ProxyItem::Flags)

//BEGIN ProxyItem
ProxyItem::ProxyItem(const QString &d, ProxyItemDir *p, ProxyItem::Flags f)
    : m_path(d), m_parent(Q_NULLPTR), m_row(-1), m_flags(f), m_doc(nullptr)
{
    updateDisplay();

    /**
     * add to parent, if parent passed
     * we assigned above nullptr to parent to not trigger
     * remove from old parent here!
     */
    if (p) {
        p->addChild(this);
    }
}

ProxyItem::~ProxyItem()
{
    qDeleteAll(m_children);
}

void ProxyItem::updateDisplay()
{
    // triggers only if this is a top level node and the root has the show full path flag set.
    if (flag(ProxyItem::Dir) && m_parent && !m_parent->m_parent && m_parent->flag(ProxyItem::ShowFullPath)) {
        m_display = m_path;
        if (m_display.startsWith(QDir::homePath())) {
            m_display.replace(0, QDir::homePath().length(), QLatin1String("~"));
        }
    } else {
        m_display = m_path.section(QLatin1Char('/'), -1, -1);
        if (flag(ProxyItem::Host) && (!m_parent || (m_parent && !m_parent->m_parent))) {
            const QString hostPrefix = QString::fromLatin1("[%1]").arg(host());
            if (hostPrefix != m_display) {
                m_display = hostPrefix + m_display;
            }
        }
    }
}

int ProxyItem::addChild(ProxyItem *item)
{
    // remove from old parent, is any
    if (item->m_parent) {
        item->m_parent->remChild(item);
    }

    const int item_row = m_children.count();
    item->m_row = item_row;
    m_children.append(item);
    item->m_parent = static_cast<ProxyItemDir *>(this);

    item->updateDisplay();

    return item_row;
}

void ProxyItem::remChild(ProxyItem *item)
{
    const int idx = m_children.indexOf(item);
    Q_ASSERT(idx != -1);

    m_children.removeAt(idx);

    for (int i = idx; i < m_children.count(); i++) {
        m_children[i]->m_row = i;
    }

    item->m_parent = nullptr;
}

ProxyItemDir *ProxyItem::parent() const
{
    return m_parent;
}

ProxyItem *ProxyItem::child(int idx) const
{
    return (idx < 0 || idx >= m_children.count()) ? 0 : m_children[idx];
}

int ProxyItem::childCount() const
{
    return m_children.count();
}

int ProxyItem::row() const
{
    return m_row;
}

const QIcon &ProxyItem::icon() const
{
    return m_icon;
}

void ProxyItem::setIcon(const QIcon &i)
{
    m_icon = i;
}

const QString &ProxyItem::documentName() const
{
    return m_documentName;
}

const QString &ProxyItem::display() const
{
    return m_display;
}

const QString &ProxyItem::path() const
{
    return m_path;
}

void ProxyItem::setPath(const QString &p)
{
    m_path = p;
    updateDisplay();
}

const QList<ProxyItem *> &ProxyItem::children() const
{
    return m_children;
}

QList<ProxyItem *> &ProxyItem::children()
{
    return m_children;
}

void ProxyItem::setDoc(KTextEditor::Document *doc)
{
    Q_ASSERT(doc);
    m_doc = doc;
    updateDocumentName();
}

KTextEditor::Document *ProxyItem::doc() const
{
    return m_doc;
}

QList<KTextEditor::Document *> ProxyItem::docTree() const
{
    QList<KTextEditor::Document *> result;

    if (m_doc) {
        result.append(m_doc);
        return result;
    }

    foreach(const ProxyItem * item, m_children) {
        result.append(item->docTree());
    }

    return result;
}

bool ProxyItem::flag(Flag f) const
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

void ProxyItem::setHost(const QString &host)
{
    m_host = host;

    if (host.isEmpty()) {
        clearFlag(Host);
    } else {
        setFlag(Host);
    }

    updateDocumentName();
    updateDisplay();
}

const QString &ProxyItem::host() const
{
    return m_host;
}

void ProxyItem::updateDocumentName()
{
    const QString docName = m_doc ? m_doc->documentName() : QString();

    if (flag(ProxyItem::Host)) {
        m_documentName = QString::fromLatin1("[%1]%2").arg(m_host).arg(docName);
    } else {
        m_documentName = docName;
    }
}

//END ProxyItem

KateFileTreeModel::KateFileTreeModel(QObject *p)
    : QAbstractItemModel(p)
    , m_root(new ProxyItemDir(QLatin1String("m_root"), nullptr))
{
    // setup default settings
    // session init will set these all soon
    const KColorScheme colors(QPalette::Active);
    const QColor bg = colors.background().color();
    m_editShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::ActiveText).color(), 0.5);
    m_viewShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::VisitedText).color(), 0.5);
    m_shadingEnabled = true;
    m_listMode = false;

    initModel();
}

KateFileTreeModel::~KateFileTreeModel()
{
    delete m_root;
}

bool KateFileTreeModel::shadingEnabled() const
{
    return m_shadingEnabled;
}

void KateFileTreeModel::setShadingEnabled(bool se)
{
    if (m_shadingEnabled != se) {
        updateBackgrounds(true);
        m_shadingEnabled = se;
    }
}

const QColor &KateFileTreeModel::editShade() const
{
    return m_editShade;
}

void KateFileTreeModel::setEditShade(const QColor &es)
{
    m_editShade = es;
}

const QColor &KateFileTreeModel::viewShade() const
{
    return m_viewShade;
}

void KateFileTreeModel::setViewShade(const QColor &vs)
{
    m_viewShade = vs;
}

bool KateFileTreeModel::showFullPathOnRoots(void) const
{
    return m_root->flag(ProxyItem::ShowFullPath);
}

void KateFileTreeModel::setShowFullPathOnRoots(bool s)
{
    if (s) {
        m_root->setFlag(ProxyItem::ShowFullPath);
    } else {
        m_root->clearFlag(ProxyItem::ShowFullPath);
    }

    foreach(ProxyItem * root, m_root->children()) {
        root->updateDisplay();
    }
}

void KateFileTreeModel::initModel()
{
    // add already existing documents
    foreach(KTextEditor::Document * doc, KTextEditor::Editor::instance()->application()->documents()) {
        documentOpened(doc);
    }
}

void KateFileTreeModel::clearModel()
{
    // remove all items
    // can safely ignore documentClosed here

    beginRemoveRows(QModelIndex(), 0, qMax(m_root->childCount() - 1, 0));

    delete m_root;
    m_root = new ProxyItemDir(QLatin1String("m_root"), nullptr);

    m_docmap.clear();
    m_viewHistory.clear();
    m_editHistory.clear();
    m_brushes.clear();

    endRemoveRows();
}

void KateFileTreeModel::connectDocument(const KTextEditor::Document *doc)
{
    connect(doc, &KTextEditor::Document::documentNameChanged, this, &KateFileTreeModel::documentNameChanged);
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, &KateFileTreeModel::documentNameChanged);
    connect(doc, &KTextEditor::Document::modifiedChanged, this, &KateFileTreeModel::documentModifiedChanged);
    connect(doc, SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
            this,  SLOT(documentModifiedOnDisc(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));
}

QModelIndex KateFileTreeModel::docIndex(const KTextEditor::Document *doc) const
{
    if (!m_docmap.contains(doc)) {
        return QModelIndex();
    }

    ProxyItem *item = m_docmap[doc];

    return createIndex(item->row(), 0, item);
}

Qt::ItemFlags KateFileTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;

    if (!index.isValid()) {
        return nullptr;
    }

    const ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
    if (item) {
        if (!item->childCount()) {
            flags |= Qt::ItemIsSelectable;
        }

        if (item->doc() && item->doc()->url().isValid()) {
            flags |= Qt::ItemIsDragEnabled;
        }
    }

    return flags;
}

Q_DECLARE_METATYPE(QList<KTextEditor::Document *>)

QVariant KateFileTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
    if (!item) {
        return QVariant();
    }

    switch (role) {
    case KateFileTreeModel::PathRole:
        // allow to sort with hostname + path, bug 271488
        return (item->doc() && !item->doc()->url().isEmpty()) ? item->doc()->url().toString() : item->path();

    case KateFileTreeModel::DocumentRole:
        return QVariant::fromValue(item->doc());

    case KateFileTreeModel::OpeningOrderRole:
        return item->row();

    case KateFileTreeModel::DocumentTreeRole:
        return QVariant::fromValue(item->docTree());

    case Qt::DisplayRole:
        // in list mode we want to use kate's fancy names.
        if (m_listMode) {
            return item->documentName();
        } else {
            return item->display();
        }

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
        const KColorScheme colors(QPalette::Active);
        if (!item->flag(ProxyItem::Dir) && (!item->doc() || item->doc()->openingError())) {
            return colors.foreground(KColorScheme::InactiveText).color();
        }
    }
    break;

    case Qt::BackgroundRole:
        // TODO: do that funky shading the file list does...
        if (m_shadingEnabled && m_brushes.contains(item)) {
            return m_brushes[item];
        }
        break;
    }

    return QVariant();
}

QMimeData *KateFileTreeModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;

    for (const auto &index : indexes) {
        ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
        if (!item || !item->doc() || !item->doc()->url().isValid()) {
            continue;
        }

        urls.append(item->doc()->url());
    }

    if (urls.isEmpty()) {
        return nullptr;
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    return mimeData;
}

QVariant KateFileTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    Q_UNUSED(role);

    if (section == 0) {
        return QLatin1String("name");
    }

    return QVariant();
}

int KateFileTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_root->childCount();
    }

    const ProxyItem *item = static_cast<ProxyItem *>(parent.internalPointer());
    if (!item) {
        return 0;
    }

    return item->childCount();
}

int KateFileTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex KateFileTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    const ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
    if (!item) {
        return QModelIndex();
    }

    if (!item->parent()) {
        return QModelIndex();
    }

    if (item->parent() == m_root) {
        return QModelIndex();
    }

    return createIndex(item->parent()->row(), 0, item->parent());
}

QModelIndex KateFileTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    const ProxyItem *p = nullptr;
    if (column != 0) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        p = m_root;
    } else {
        p = static_cast<ProxyItem *>(parent.internalPointer());
    }

    if (!p) {
        return QModelIndex();
    }

    if (row < 0 || row >= p->childCount()) {
        return QModelIndex();
    }

    return createIndex(row, 0, p->child(row));
}

bool KateFileTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_root->childCount() > 0;
    }

    const ProxyItem *item = static_cast<ProxyItem *>(parent.internalPointer());
    if (!item) {
        return false;
    }

    return item->childCount() > 0;
}

bool KateFileTreeModel::isDir(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return true;
    }

    const ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
    if (!item) {
        return false;
    }

    return item->flag(ProxyItem::Dir);
}

bool KateFileTreeModel::listMode() const
{
    return m_listMode;
}

void KateFileTreeModel::setListMode(bool lm)
{
    if (lm != m_listMode) {
        m_listMode = lm;

        clearModel();
        initModel();
    }
}

void KateFileTreeModel::documentOpened(KTextEditor::Document *doc)
{
    ProxyItem *item = new ProxyItem(QString());
    item->setDoc(doc);

    updateItemPathAndHost(item);
    setupIcon(item);
    handleInsert(item);
    m_docmap[doc] = item;
    connectDocument(doc);
}

void KateFileTreeModel::documentsOpened(const QList<KTextEditor::Document *> &docs)
{
    foreach(KTextEditor::Document * doc, docs) {
        if (m_docmap.contains(doc)) {
            documentNameChanged(doc);
        } else {
            documentOpened(doc);
        }
    }
}

void KateFileTreeModel::documentModifiedChanged(KTextEditor::Document *doc)
{
    if (!m_docmap.contains(doc)) {
        return;
    }

    ProxyItem *item = m_docmap[doc];

    if (doc->isModified()) {
        item->setFlag(ProxyItem::Modified);
    } else {
        item->clearFlag(ProxyItem::Modified);
        item->clearFlag(ProxyItem::ModifiedExternally);
        item->clearFlag(ProxyItem::DeletedExternally);
    }

    setupIcon(item);

    const QModelIndex idx = createIndex(item->row(), 0, item);
    emit dataChanged(idx, idx);
}

void KateFileTreeModel::documentModifiedOnDisc(KTextEditor::Document *doc, bool modified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
    Q_UNUSED(modified);
    if (!m_docmap.contains(doc)) {
        return;
    }

    ProxyItem *item = m_docmap[doc];

    // This didn't do what I thought it did, on an ignore
    // we'd get !modified causing the warning icons to disappear
    if (!modified) {
        item->clearFlag(ProxyItem::ModifiedExternally);
        item->clearFlag(ProxyItem::DeletedExternally);
    } else {
        if (reason == KTextEditor::ModificationInterface::OnDiskDeleted) {
            item->setFlag(ProxyItem::DeletedExternally);
        } else if (reason == KTextEditor::ModificationInterface::OnDiskModified) {
            item->setFlag(ProxyItem::ModifiedExternally);
        } else if (reason == KTextEditor::ModificationInterface::OnDiskCreated) {
            // with out this, on "reload" we don't get the icons removed :(
            item->clearFlag(ProxyItem::ModifiedExternally);
            item->clearFlag(ProxyItem::DeletedExternally);
        }
    }

    setupIcon(item);

    const QModelIndex idx = createIndex(item->row(), 0, item);
    emit dataChanged(idx, idx);
}

void KateFileTreeModel::documentActivated(const KTextEditor::Document *doc)
{
    if (!m_docmap.contains(doc)) {
        return;
    }

    ProxyItem *item = m_docmap[doc];
    m_viewHistory.removeAll(item);
    m_viewHistory.prepend(item);

    while (m_viewHistory.count() > 10) {
        m_viewHistory.removeLast();
    }

    updateBackgrounds();
}

void KateFileTreeModel::documentEdited(const KTextEditor::Document *doc)
{
    if (!m_docmap.contains(doc)) {
        return;
    }

    ProxyItem *item = m_docmap[doc];
    m_editHistory.removeAll(item);
    m_editHistory.prepend(item);
    while (m_editHistory.count() > 10) {
        m_editHistory.removeLast();
    }

    updateBackgrounds();
}

void KateFileTreeModel::slotAboutToDeleteDocuments(const QList<KTextEditor::Document *> &docs)
{
    foreach(const KTextEditor::Document * doc, docs) {
        disconnect(doc, &KTextEditor::Document::documentNameChanged, this, &KateFileTreeModel::documentNameChanged);
        disconnect(doc, &KTextEditor::Document::documentUrlChanged, this, &KateFileTreeModel::documentNameChanged);
        disconnect(doc, &KTextEditor::Document::modifiedChanged, this, &KateFileTreeModel::documentModifiedChanged);
        disconnect(doc, SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
                   this,  SLOT(documentModifiedOnDisc(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));
    }
}

void KateFileTreeModel::slotDocumentsDeleted(const QList<KTextEditor::Document *> &docs)
{
    foreach(const KTextEditor::Document * doc, docs) {
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
    if (!m_shadingEnabled && !force) {
        return;
    }

    QMap <ProxyItem *, EditViewCount> helper;
    int i = 1;

    foreach(ProxyItem * item, m_viewHistory) {
        helper[item].view = i;
        i++;
    }

    i = 1;
    foreach(ProxyItem * item, m_editHistory) {
        helper[item].edit = i;
        i++;
    }

    QMap<ProxyItem *, QBrush> oldBrushes = m_brushes;
    m_brushes.clear();

    const int hc = m_viewHistory.count();
    const int ec = m_editHistory.count();

    for (QMap<ProxyItem *, EditViewCount>::iterator it = helper.begin(); it != helper.end(); ++it) {
        QColor shade(m_viewShade);
        QColor eshade(m_editShade);

        if (it.value().edit > 0) {
            int v = hc - it.value().view;
            int e = ec - it.value().edit + 1;

            e = e * e;

            const int n = qMax(v + e, 1);

            shade.setRgb(
                ((shade.red() * v) + (eshade.red() * e)) / n,
                ((shade.green() * v) + (eshade.green() * e)) / n,
                ((shade.blue() * v) + (eshade.blue() * e)) / n
            );
        }

        // blend in the shade color; latest is most colored.
        const double t = double(hc - it.value().view + 1) / double(hc);

        m_brushes[it.key()] = QBrush(KColorUtils::mix(QPalette().color(QPalette::Base), shade, t));
    }

    for (auto it = m_brushes.constBegin(), end = m_brushes.constEnd(); it != end; ++it) {
        ProxyItem *item = it.key();
        oldBrushes.remove(item);
        const QModelIndex idx = createIndex(item->row(), 0, item);
        dataChanged(idx, idx);
    }

    for (auto it = oldBrushes.constBegin(), end = oldBrushes.constEnd(); it != end; ++it) {
        ProxyItem *item = it.key();
        const QModelIndex idx = createIndex(item->row(), 0, item);
        dataChanged(idx, idx);
    }
}

void KateFileTreeModel::handleEmptyParents(ProxyItemDir *item)
{
    Q_ASSERT(item != nullptr);

    if (!item->parent()) {
        return;
    }

    ProxyItemDir *parent = item->parent();

    while (parent) {
        if (!item->childCount()) {
            const QModelIndex parent_index = (parent == m_root) ? QModelIndex() : createIndex(parent->row(), 0, parent);
            beginRemoveRows(parent_index, item->row(), item->row());
            parent->remChild(item);
            endRemoveRows();
            delete item;
        } else {
            // breakout early, if this node isn't empty, theres no use in checking its parents
            return;
        }

        item = parent;
        parent = item->parent();
    }
}

void KateFileTreeModel::documentClosed(KTextEditor::Document *doc)
{
    if (!m_docmap.contains(doc)) {
        return;
    }

    if (m_shadingEnabled) {
        ProxyItem *toRemove = m_docmap[doc];
        if (m_brushes.contains(toRemove)) {
            m_brushes.remove(toRemove);
        }

        if (m_viewHistory.contains(toRemove)) {
            m_viewHistory.removeAll(toRemove);
        }

        if (m_editHistory.contains(toRemove)) {
            m_editHistory.removeAll(toRemove);
        }
    }

    ProxyItem *node = m_docmap[doc];
    ProxyItemDir *parent = node->parent();

    const QModelIndex parent_index = (parent == m_root) ? QModelIndex() : createIndex(parent->row(), 0, parent);
    beginRemoveRows(parent_index, node->row(), node->row());
    node->parent()->remChild(node);
    endRemoveRows();

    delete node;
    handleEmptyParents(parent);

    m_docmap.remove(doc);
}

void KateFileTreeModel::documentNameChanged(KTextEditor::Document *doc)
{
    if (!m_docmap.contains(doc)) {
        return;
    }

    ProxyItem *item = m_docmap[doc];

    if (m_shadingEnabled) {
        ProxyItem *toRemove = m_docmap[doc];
        if (m_brushes.contains(toRemove)) {
            QBrush brush = m_brushes[toRemove];
            m_brushes.remove(toRemove);
            m_brushes.insert(item, brush);
        }

        if (m_viewHistory.contains(toRemove)) {
            const int idx = m_viewHistory.indexOf(toRemove);
            if (idx != -1) {
                m_viewHistory.replace(idx, item);
            }
        }

        if (m_editHistory.contains(toRemove)) {
            const int idx = m_editHistory.indexOf(toRemove);
            if (idx != -1) {
                m_editHistory.replace(idx, item);
            }
        }
    }

    handleNameChange(item);
    emit triggerViewChangeAfterNameChange(); // FIXME: heh, non-standard signal?
}

ProxyItemDir *KateFileTreeModel::findRootNode(const QString &name, const int r) const
{
    foreach(ProxyItem * item, m_root->children()) {
        if (!item->flag(ProxyItem::Dir)) {
            continue;
        }

        // make sure we're actually matching against the right dir,
        // previously the check below would match /foo/xy against /foo/x
        // and return /foo/x rather than /foo/xy
        // this seems a bit hackish, but is the simplest way to solve the
        // current issue.
        QString path = item->path().section(QLatin1Char('/'), 0, -r) + QLatin1Char('/');

        if (name.startsWith(path)) {
            return static_cast<ProxyItemDir *>(item);
        }
    }

    return nullptr;
}

ProxyItemDir *KateFileTreeModel::findChildNode(const ProxyItemDir *parent, const QString &name) const
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(!name.isEmpty());

    if (!parent->childCount()) {
        return nullptr;
    }

    foreach(ProxyItem * item, parent->children()) {
        if (!item->flag(ProxyItem::Dir)) {
            continue;
        }

        if (item->display() == name) {
            return static_cast<ProxyItemDir *>(item);
        }
    }

    return nullptr;
}

void KateFileTreeModel::insertItemInto(ProxyItemDir *root, ProxyItem *item)
{
    Q_ASSERT(root != nullptr);
    Q_ASSERT(item != nullptr);

    QString tail = item->path();
    tail.remove(0, root->path().length());
    QStringList parts = tail.split(QLatin1Char('/'), QString::SkipEmptyParts);
    ProxyItemDir *ptr = root;
    QStringList current_parts;
    current_parts.append(root->path());

    // seems this can be empty, see bug 286191
    if (!parts.isEmpty()) {
        parts.pop_back();
    }

    foreach(const QString & part, parts) {
        current_parts.append(part);
        ProxyItemDir *find = findChildNode(ptr, part);
        if (!find) {
            const QString new_name = current_parts.join(QLatin1String("/"));
            const QModelIndex parent_index = (ptr == m_root) ? QModelIndex() : createIndex(ptr->row(), 0, ptr);
            beginInsertRows(parent_index, ptr->childCount(), ptr->childCount());
            ptr = new ProxyItemDir(new_name, ptr);
            endInsertRows();
        } else {
            ptr = find;
        }
    }

    const QModelIndex parent_index = (ptr == m_root) ? QModelIndex() : createIndex(ptr->row(), 0, ptr);
    beginInsertRows(parent_index, ptr->childCount(), ptr->childCount());
    ptr->addChild(item);
    endInsertRows();
}

void KateFileTreeModel::handleInsert(ProxyItem *item)
{
    Q_ASSERT(item != nullptr);

    if (m_listMode || item->flag(ProxyItem::Empty)) {
        beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
        m_root->addChild(item);
        endInsertRows();
        return;
    }

    // case (item.path > root.path)
    ProxyItemDir *root = findRootNode(item->path());
    if (root) {
        insertItemInto(root, item);
        return;
    }

    // trim off trailing file and dir
    QString base = item->path().section(QLatin1Char('/'), 0, -2);

    // create new root
    ProxyItemDir *new_root = new ProxyItemDir(base);
    new_root->setHost(item->host());

    // add new root to m_root
    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
    m_root->addChild(new_root);
    endInsertRows();

    // same fix as in findRootNode, try to match a full dir, instead of a partial path
    base += QLatin1Char('/');

    // try and merge existing roots with the new root node (new_root.path < root.path)
    foreach(ProxyItem * root, m_root->children()) {
        if (root == new_root || !root->flag(ProxyItem::Dir)) {
            continue;
        }

        if (root->path().startsWith(base)) {
            beginRemoveRows(QModelIndex(), root->row(), root->row());
            m_root->remChild(root);
            endRemoveRows();

            //beginInsertRows(new_root_index, new_root->childCount(), new_root->childCount());
            // this can't use new_root->addChild directly, or it'll potentially miss a bunch of subdirs
            insertItemInto(new_root, root);
            //endInsertRows();
        }
    }

    // add item to new root
    // have to call begin/endInsertRows here, or the new item won't show up.
    const QModelIndex new_root_index = createIndex(new_root->row(), 0, new_root);
    beginInsertRows(new_root_index, new_root->childCount(), new_root->childCount());
    new_root->addChild(item);
    endInsertRows();

    handleDuplicitRootDisplay(new_root);
}

void KateFileTreeModel::handleDuplicitRootDisplay(ProxyItemDir *init)
{
    QStack<ProxyItemDir *> rootsToCheck;
    rootsToCheck.push(init);

    // make sure the roots don't match (recursively)
    while (!rootsToCheck.isEmpty()) {
        ProxyItemDir *check_root = rootsToCheck.pop();

        if (check_root->parent() != m_root) {
            continue;
        }

        foreach(ProxyItem * root, m_root->children()) {
            if (root == check_root || !root->flag(ProxyItem::Dir)) {
                continue;
            }

            if (check_root->display() == root->display()) {
                bool changed = false;
                bool check_root_removed = false;

                const QString rdir = root->path().section(QLatin1Char('/'), 0, -2);
                if (!rdir.isEmpty()) {
                    beginRemoveRows(QModelIndex(), root->row(), root->row());
                    m_root->remChild(root);
                    endRemoveRows();

                    ProxyItemDir *irdir = new ProxyItemDir(rdir);
                    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
                    m_root->addChild(irdir);
                    endInsertRows();

                    insertItemInto(irdir, root);

                    foreach(ProxyItem * node, m_root->children()) {
                        if (node == irdir || !root->flag(ProxyItem::Dir)) {
                            continue;
                        }

                        const QString xy = rdir + QLatin1Char('/');
                        if (node->path().startsWith(xy)) {
                            beginRemoveRows(QModelIndex(), node->row(), node->row());
                            // check_root_removed must be sticky
                            check_root_removed = check_root_removed || (node == check_root);
                            m_root->remChild(node);
                            endRemoveRows();
                            insertItemInto(irdir, node);
                        }
                    }

                    rootsToCheck.push(irdir);
                    changed = true;
                }

                if (!check_root_removed) {
                    const QString nrdir = check_root->path().section(QLatin1Char('/'), 0, -2);
                    if (!nrdir.isEmpty()) {
                        beginRemoveRows(QModelIndex(), check_root->row(), check_root->row());
                        m_root->remChild(check_root);
                        endRemoveRows();

                        ProxyItemDir *irdir = new ProxyItemDir(nrdir);
                        beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
                        m_root->addChild(irdir);
                        endInsertRows();

                        insertItemInto(irdir, check_root);

                        rootsToCheck.push(irdir);
                        changed = true;
                    }
                }

                if (changed) {
                    break; // restart
                }
            }
        } // foreach root

    }

}

void KateFileTreeModel::handleNameChange(ProxyItem *item)
{
    Q_ASSERT(item != nullptr);
    Q_ASSERT(item->parent());

    updateItemPathAndHost(item);

    if (m_listMode) {
        const QModelIndex idx = createIndex(item->row(), 0, item);
        setupIcon(item);
        emit dataChanged(idx, idx);
        return;
    }

    // in either case (new/change) we want to remove the item from its parent
    ProxyItemDir *parent = item->parent();

    const QModelIndex parent_index = (parent == m_root) ? QModelIndex() : createIndex(parent->row(), 0, parent);
    beginRemoveRows(parent_index, item->row(), item->row());
    parent->remChild(item);
    endRemoveRows();

    handleEmptyParents(parent);

    // clear all but Empty flag
    if (item->flag(ProxyItem::Empty)) {
        item->setFlags(ProxyItem::Empty);
    } else {
        item->setFlags(ProxyItem::None);
    }

    setupIcon(item);
    handleInsert(item);
}

void KateFileTreeModel::updateItemPathAndHost(ProxyItem *item) const
{
    const KTextEditor::Document *doc = item->doc();
    Q_ASSERT(doc); // this method should not be called at directory items

    QString path = doc->url().path();
    QString host;
    if (doc->url().isEmpty()) {
        path = doc->documentName();
        item->setFlag(ProxyItem::Empty);
    } else {
        item->clearFlag(ProxyItem::Empty);
        host = doc->url().host();
        if (!host.isEmpty()) {
            path = QString::fromLatin1("[%1]%2").arg(host).arg(path);
        }
    }

    // for some reason we get useless name changes [should be fixed in 5.0]
    if (item->path() == path) {
        return;
    }

    item->setPath(path);
    item->setHost(host);
}

void KateFileTreeModel::setupIcon(ProxyItem *item) const
{
    Q_ASSERT(item != nullptr);

    QString icon_name;

    if (item->flag(ProxyItem::Modified)) {
        icon_name = QLatin1String("document-save");
    } else {
        const QUrl url(item->path());
        icon_name = QMimeDatabase().mimeTypeForFile(url.path(), QMimeDatabase::MatchExtension).iconName();
    }

    QIcon icon = QIcon::fromTheme(icon_name);

    if (item->flag(ProxyItem::ModifiedExternally) || item->flag(ProxyItem::DeletedExternally)) {
        icon = KIconUtils::addOverlay(icon, QIcon(QLatin1String("emblem-important")), Qt::TopLeftCorner);
    }

    item->setIcon(icon);
}

void KateFileTreeModel::resetHistory()
{
    QSet<ProxyItem *> list = QSet<ProxyItem *>::fromList(m_viewHistory);
    list += QSet<ProxyItem *>::fromList(m_editHistory);

    m_viewHistory.clear();
    m_editHistory.clear();
    m_brushes.clear();

    foreach(ProxyItem * item, list) {
        QModelIndex idx = createIndex(item->row(), 0, item);
        dataChanged(idx, idx, QVector<int>(1, Qt::BackgroundRole));
    }
}

