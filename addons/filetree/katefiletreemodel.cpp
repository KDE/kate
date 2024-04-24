/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katefiletreemodel.h"

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QList>
#include <QMimeData>
#include <QStack>
#include <QWidget>

#include <KColorScheme>
#include <KColorUtils>
#include <KLocalizedString>

#include <KTextEditor/MainWindow>
#include <ktexteditor/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

#include "katefiletreedebug.h"
#include "ktexteditor_utils.h"

#include <variant>

static constexpr int MaxHistoryItems = 10;

class FileTreeMimeData : public QMimeData
{
    Q_OBJECT
public:
    FileTreeMimeData(const QModelIndex &index)
        : m_index(index)
    {
    }

    QModelIndex index() const
    {
        return m_index;
    }

private:
    QPersistentModelIndex m_index;
};

class ProxyItemDir;
class ProxyItem
{
    friend class KateFileTreeModel;

public:
    enum Flag {
        None = 0,
        Dir = 1,
        Modified = 2,
        ModifiedExternally = 4,
        DeletedExternally = 8,
        Empty = 16,
        ShowFullPath = 32,
        Host = 64,
        Widget = 128,
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    ProxyItem(const QString &n, ProxyItemDir *p = nullptr, Flags f = ProxyItem::None);
    ~ProxyItem();

    int addChild(ProxyItem *p);
    void removeChild(ProxyItem *p);

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

    const std::vector<ProxyItem *> &children() const;
    std::vector<ProxyItem *> &children();

    void setDoc(KTextEditor::Document *doc);
    KTextEditor::Document *doc() const;

    void setWidget(QWidget *);
    QWidget *widget() const;

    /**
     * the view uses this to close all the documents under the folder
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
    std::vector<ProxyItem *> m_children;
    int m_row;
    Flags m_flags;

    QString m_display;
    QIcon m_icon;
    std::variant<KTextEditor::Document *, QWidget *> m_object;
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

    dbg.nospace() << "ProxyItem(" << item << ",";
    dbg.nospace() << parent << "," << item->row() << ",";
    dbg.nospace() << item->doc() << "," << item->path() << ") ";
    return dbg.maybeSpace();
}

class ProxyItemDir : public ProxyItem
{
public:
    ProxyItemDir(const QString &n, ProxyItemDir *p = nullptr)
        : ProxyItem(n, p)
    {
        setFlag(ProxyItem::Dir);
        updateDisplay();

        setIcon(QIcon::fromTheme(QStringLiteral("folder")));
    }
};

QDebug operator<<(QDebug dbg, ProxyItemDir *item)
{
    if (!item) {
        dbg.nospace() << "ProxyItemDir(0x0) ";
        return dbg.maybeSpace();
    }

    const void *parent = static_cast<void *>(item->parent());

    dbg.nospace() << "ProxyItemDir(" << item << ",";
    dbg.nospace() << parent << "," << item->row() << ",";
    dbg.nospace() << item->path() << ", children:" << item->childCount() << ") ";
    return dbg.maybeSpace();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(ProxyItem::Flags)

// BEGIN ProxyItem
ProxyItem::ProxyItem(const QString &d, ProxyItemDir *p, ProxyItem::Flags f)
    : m_path(d)
    , m_parent(Q_NULLPTR)
    , m_row(-1)
    , m_flags(f)
{
    updateDisplay();

    if (f.testFlag(Widget) && f.testFlag(Dir)) {
        m_documentName = display();
    }

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
            m_display.replace(0, QDir::homePath().length(), QStringLiteral("~"));
        }
    } else {
        m_display = m_path.section(QLatin1Char('/'), -1, -1);
        if (flag(ProxyItem::Host) && (!m_parent || (m_parent && !m_parent->m_parent))) {
            const QString hostPrefix = QStringLiteral("[%1]").arg(host());
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
        item->m_parent->removeChild(item);
    }

    const int item_row = m_children.size();
    item->m_row = item_row;
    m_children.push_back(item);
    item->m_parent = static_cast<ProxyItemDir *>(this);

    item->updateDisplay();

    return item_row;
}

void ProxyItem::removeChild(ProxyItem *item)
{
    auto it = std::find(m_children.begin(), m_children.end(), item);
    Q_ASSERT(it != m_children.end());
    m_children.erase(it);

    auto idx = std::distance(m_children.begin(), it);
    for (size_t i = idx; i < m_children.size(); i++) {
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
    return (size_t(idx) >= m_children.size()) ? nullptr : m_children[idx];
}

int ProxyItem::childCount() const
{
    return m_children.size();
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

const std::vector<ProxyItem *> &ProxyItem::children() const
{
    return m_children;
}

std::vector<ProxyItem *> &ProxyItem::children()
{
    return m_children;
}

void ProxyItem::setDoc(KTextEditor::Document *doc)
{
    Q_ASSERT(doc);
    m_object = doc;
    updateDocumentName();
}

void ProxyItem::setWidget(QWidget *w)
{
    Q_ASSERT(w);
    m_object = w;
    updateDocumentName();
}

QWidget *ProxyItem::widget() const
{
    if (!std::holds_alternative<QWidget *>(m_object))
        return nullptr;
    return std::get<QWidget *>(m_object);
}

KTextEditor::Document *ProxyItem::doc() const
{
    if (!std::holds_alternative<KTextEditor::Document *>(m_object))
        return nullptr;
    return std::get<KTextEditor::Document *>(m_object);
}

QList<KTextEditor::Document *> ProxyItem::docTree() const
{
    QList<KTextEditor::Document *> result;

    if (doc()) {
        result.append(doc());
        return result;
    }

    for (const ProxyItem *item : m_children) {
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
    QString name;
    if (doc()) {
        name = doc()->documentName();
    } else if (widget()) {
        name = widget()->windowTitle();
    }

    if (flag(ProxyItem::Host)) {
        m_documentName = QStringLiteral("[%1]%2").arg(m_host, name);
    } else {
        m_documentName = name;
    }
}

// END ProxyItem

KateFileTreeModel::KateFileTreeModel(KTextEditor::MainWindow *mainWindow, QObject *p)
    : QAbstractItemModel(p)
    , m_mainWindow(mainWindow)
    , m_root(new ProxyItemDir(QStringLiteral("m_root"), nullptr))
{
    // setup default settings
    // session init will set these all soon
    const KColorScheme colors(QPalette::Active);
    const QColor bg = colors.background().color();
    m_editShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::ActiveText).color(), 0.5);
    m_viewShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::VisitedText).color(), 0.5);
    m_inactiveDocColor = colors.foreground(KColorScheme::InactiveText).color();
    m_shadingEnabled = true;
    m_listMode = false;

    initModel();

    // ensure palette change updates the colors properly
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, [this]() {
        m_inactiveDocColor = KColorScheme(QPalette::Active).foreground(KColorScheme::InactiveText).color();
        updateBackgrounds(true);
    });
}

KateFileTreeModel::~KateFileTreeModel()
{
    delete m_root;
}

void KateFileTreeModel::setShadingEnabled(bool se)
{
    if (m_shadingEnabled != se) {
        updateBackgrounds(true);
        m_shadingEnabled = se;
    }

    if (!se) {
        m_viewHistory.clear();
        m_editHistory.clear();
        m_brushes.clear();
    }
}

void KateFileTreeModel::setEditShade(const QColor &es)
{
    m_editShade = es;
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

    const auto rootChildren = m_root->children();
    for (ProxyItem *root : rootChildren) {
        root->updateDisplay();
    }
}

void KateFileTreeModel::initModel()
{
    beginInsertRows(QModelIndex(), 0, 0);
    Q_ASSERT(!m_widgetsRoot);
    m_widgetsRoot = new ProxyItem(i18nc("Open here is a description, i.e. 'list of widgets that are open' not a verb", "Open Widgets"),
                                  nullptr,
                                  ProxyItem::Flags(ProxyItem::Dir | ProxyItem::Widget));
    m_widgetsRoot->setFlags(ProxyItem::Flags(ProxyItem::Dir | ProxyItem::Widget));
    m_widgetsRoot->setIcon(QIcon::fromTheme(QStringLiteral("folder-windows")));
    m_root->addChild(m_widgetsRoot);
    endInsertRows();

    // add already existing documents
    const auto documents = KTextEditor::Editor::instance()->application()->documents();
    for (KTextEditor::Document *doc : documents) {
        documentOpened(doc);
    }

    if (m_mainWindow) {
        QWidgetList widgets = m_mainWindow->widgets();
        for (auto *w : std::as_const(widgets)) {
            addWidget(w);
        }
    }
}

void KateFileTreeModel::clearModel()
{
    // remove all items
    // can safely ignore documentClosed here

    beginResetModel();

    delete m_root;
    m_root = new ProxyItemDir(QStringLiteral("m_root"), nullptr);

    m_widgetsRoot = nullptr;

    m_docmap.clear();
    m_viewHistory.clear();
    m_editHistory.clear();
    m_brushes.clear();

    endResetModel();
}

void KateFileTreeModel::connectDocument(const KTextEditor::Document *doc)
{
    connect(doc, &KTextEditor::Document::documentNameChanged, this, &KateFileTreeModel::documentNameChanged);
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, &KateFileTreeModel::documentNameChanged);
    connect(doc, &KTextEditor::Document::modifiedChanged, this, &KateFileTreeModel::documentModifiedChanged);
    connect(doc, &KTextEditor::Document::modifiedOnDisk, this, &KateFileTreeModel::documentModifiedOnDisc);
}

QModelIndex KateFileTreeModel::docIndex(const KTextEditor::Document *doc) const
{
    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return {};
    }
    auto item = it.value();
    return createIndex(item->row(), 0, item);
}

QModelIndex KateFileTreeModel::widgetIndex(QWidget *widget) const
{
    ProxyItem *item = nullptr;
    const auto items = m_widgetsRoot->children();
    for (auto *it : items) {
        if (it->widget() == widget) {
            item = it;
            break;
        }
    }
    if (!item) {
        return {};
    }

    return createIndex(item->row(), 0, item);
}

Qt::ItemFlags KateFileTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;

    if (!index.isValid()) {
        return Qt::NoItemFlags | Qt::ItemIsDropEnabled;
    }

    const ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
    if (item) {
        if (!item->flag(ProxyItem::Dir)) {
            flags |= Qt::ItemIsSelectable;
        }

        if (item->flag(ProxyItem::Dir) && !item->flag(ProxyItem::Widget)) {
            flags |= Qt::ItemIsDropEnabled;
        }

        if (item->doc() && item->doc()->url().isValid()) {
            flags |= Qt::ItemIsDragEnabled;
        }
    }

    return flags;
}

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

    case KateFileTreeModel::WidgetRole:
        return QVariant::fromValue(item->widget());

    case KateFileTreeModel::OpeningOrderRole:
        return item->row();

    case KateFileTreeModel::DocumentTreeRole:
        return QVariant::fromValue(item->docTree());

    case Qt::DisplayRole:
        // in list mode we want to use kate's fancy names.
        if (index.column() == 0) {
            if (m_listMode) {
                return item->documentName();
            } else {
                return item->display();
            }
        }
        break;
    case Qt::DecorationRole:
        if (index.column() == 0) {
            return item->icon();
        }
        break;
    case Qt::ToolTipRole: {
        QString tooltip = item->path();
        if (item->flag(ProxyItem::DeletedExternally) || item->flag(ProxyItem::ModifiedExternally)) {
            tooltip = i18nc("%1 is the full path", "<p><b>%1</b></p><p>The document has been modified by another application.</p>", item->path());
        }

        return tooltip;
    }

    case Qt::ForegroundRole: {
        if (!item->flag(ProxyItem::Widget) && !item->flag(ProxyItem::Dir) && (!item->doc() || item->doc()->openingError())) {
            return m_inactiveDocColor;
        }
    } break;

    case Qt::BackgroundRole:
        // TODO: do that funky shading the file list does...
        if (m_shadingEnabled) {
            if (auto it = m_brushes.find(item); it != m_brushes.end()) {
                return it->second;
            }
        }
        break;
    }

    return QVariant();
}

Qt::DropActions KateFileTreeModel::supportedDropActions() const
{
    Qt::DropActions a = QAbstractItemModel::supportedDropActions();
    a |= Qt::MoveAction;
    return a;
}

QMimeData *KateFileTreeModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.size() != columnCount()) {
        return nullptr;
    }

    ProxyItem *item = static_cast<ProxyItem *>(indexes.at(0).internalPointer());
    QList<QUrl> urls;
    if (!item || !item->doc() || !item->doc()->url().isValid()) {
        return nullptr;
    }
    urls.append(item->doc()->url());

    FileTreeMimeData *mimeData = new FileTreeMimeData(indexes.at(0));
    mimeData->setUrls(urls);
    return mimeData;
}

bool KateFileTreeModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int, int, const QModelIndex &parent) const
{
    if (auto md = qobject_cast<const FileTreeMimeData *>(data)) {
        return action == Qt::MoveAction && md->index().parent() == parent;
    }
    return false;
}

bool KateFileTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction, int row, int, const QModelIndex &parent)
{
    auto md = qobject_cast<const FileTreeMimeData *>(data);
    if (!md) {
        return false;
    }

    const auto index = md->index();
    Q_ASSERT(parent == index.parent()); // move is only allowed within the same parent
    if (!index.isValid() || index.row() > rowCount(parent) || index.row() == row) {
        return false;
    }

    auto parentItem = parent.isValid() ? static_cast<ProxyItemDir *>(parent.internalPointer()) : m_root;
    auto &childs = parentItem->children();
    int sourceRow = index.row();

    beginMoveRows(index.parent(), index.row(), index.row(), parent, row);
    childs.insert(childs.begin() + row, childs.at(index.row()));
    if (sourceRow > row) {
        sourceRow++;
    }
    childs.erase(childs.begin() + sourceRow);
    // update row number of children
    for (size_t i = 0; i < childs.size(); i++) {
        childs[i]->m_row = i;
    }

    endMoveRows();
    return true;
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

    // we only have children for column 0
    if (parent.column() != 0) {
        return 0;
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
    return 2;
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
    if (column != 0 && column != 1) {
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

    return createIndex(row, column, p->child(row));
}

bool KateFileTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_root->childCount() > 0;
    }

    // we only have children for column 0
    if (parent.column() != 0) {
        return false;
    }

    const ProxyItem *item = static_cast<ProxyItem *>(parent.internalPointer());
    if (!item) {
        return false;
    }

    return item->childCount() > 0;
}

ProxyItem *KateFileTreeModel::itemForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return m_root;
    }

    ProxyItem *item = static_cast<ProxyItem *>(index.internalPointer());
    if (!item) {
        return nullptr;
    }
    return item;
}

bool KateFileTreeModel::isDir(const QModelIndex &index) const
{
    if (auto item = itemForIndex(index)) {
        return item->flag(ProxyItem::Dir) && !item->flag(ProxyItem::Widget);
    }
    return false;
}

bool KateFileTreeModel::isWidgetDir(const QModelIndex &index) const
{
    if (auto item = itemForIndex(index)) {
        return item->flag(ProxyItem::Dir) && item->flag(ProxyItem::Widget);
    }
    return false;
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
    for (KTextEditor::Document *doc : docs) {
        if (m_docmap.contains(doc)) {
            documentNameChanged(doc);
        } else {
            documentOpened(doc);
        }
    }
}

void KateFileTreeModel::documentModifiedChanged(KTextEditor::Document *doc)
{
    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return;
    }

    ProxyItem *item = it.value();

    if (doc->isModified()) {
        item->setFlag(ProxyItem::Modified);
    } else {
        item->clearFlag(ProxyItem::Modified);
        item->clearFlag(ProxyItem::ModifiedExternally);
        item->clearFlag(ProxyItem::DeletedExternally);
    }

    setupIcon(item);

    const QModelIndex idx = createIndex(item->row(), 0, item);
    Q_EMIT dataChanged(idx, idx);
}

void KateFileTreeModel::documentModifiedOnDisc(KTextEditor::Document *doc, bool modified, KTextEditor::Document::ModifiedOnDiskReason reason)
{
    Q_UNUSED(modified);
    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return;
    }

    ProxyItem *item = it.value();

    // This didn't do what I thought it did, on an ignore
    // we'd get !modified causing the warning icons to disappear
    if (!modified) {
        item->clearFlag(ProxyItem::ModifiedExternally);
        item->clearFlag(ProxyItem::DeletedExternally);
    } else {
        if (reason == KTextEditor::Document::OnDiskDeleted) {
            item->setFlag(ProxyItem::DeletedExternally);
        } else if (reason == KTextEditor::Document::OnDiskModified) {
            item->setFlag(ProxyItem::ModifiedExternally);
        } else if (reason == KTextEditor::Document::OnDiskCreated) {
            // with out this, on "reload" we don't get the icons removed :(
            item->clearFlag(ProxyItem::ModifiedExternally);
            item->clearFlag(ProxyItem::DeletedExternally);
        }
    }

    setupIcon(item);

    const QModelIndex idx = createIndex(item->row(), 0, item);
    Q_EMIT dataChanged(idx, idx);
}

void KateFileTreeModel::documentActivated(const KTextEditor::Document *doc)
{
    if (!m_shadingEnabled) {
        return;
    }

    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return;
    }

    ProxyItem *item = it.value();

    m_viewHistory.erase(std::remove(m_viewHistory.begin(), m_viewHistory.end(), item), m_viewHistory.end());
    m_viewHistory.insert(m_viewHistory.begin(), item);

    while (m_viewHistory.size() > MaxHistoryItems) {
        m_viewHistory.pop_back();
    }

    updateBackgrounds();
}

void KateFileTreeModel::documentEdited(const KTextEditor::Document *doc)
{
    if (!m_shadingEnabled) {
        return;
    }

    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return;
    }

    ProxyItem *item = it.value();

    m_editHistory.erase(std::remove(m_editHistory.begin(), m_editHistory.end(), item), m_editHistory.end());
    m_editHistory.insert(m_editHistory.begin(), item);

    while (m_editHistory.size() > MaxHistoryItems) {
        m_editHistory.pop_back();
    }

    updateBackgrounds();
}

class EditViewCount
{
public:
    EditViewCount() = default;
    int edit = 0;
    int view = 0;
};

void KateFileTreeModel::updateBackgrounds(bool force)
{
    if (!m_shadingEnabled && !force) {
        return;
    }

    std::unordered_map<ProxyItem *, EditViewCount> helper;
    helper.reserve(m_viewHistory.size() + m_editHistory.size());

    int i = 1;
    for (ProxyItem *item : std::as_const(m_viewHistory)) {
        helper[item].view = i;
        i++;
    }

    i = 1;
    for (ProxyItem *item : std::as_const(m_editHistory)) {
        helper[item].edit = i;
        i++;
    }

    std::unordered_map<ProxyItem *, QBrush> oldBrushes = std::move(m_brushes);

    const int hc = (int)m_viewHistory.size();
    const int ec = (int)m_editHistory.size();
    const QColor base = QPalette().color(QPalette::Base);

    for (const auto &[item, editViewCount] : helper) {
        QColor shade(m_viewShade);
        QColor eshade(m_editShade);

        if (editViewCount.edit > 0) {
            int v = hc - editViewCount.view;
            int e = ec - editViewCount.edit + 1;

            e = e * e;

            const int n = qMax(v + e, 1);

            shade.setRgb(((shade.red() * v) + (eshade.red() * e)) / n,
                         ((shade.green() * v) + (eshade.green() * e)) / n,
                         ((shade.blue() * v) + (eshade.blue() * e)) / n);
        }

        // blend in the shade color; latest is most colored.
        const double t = double(hc - editViewCount.view + 1) / double(hc);

        m_brushes[item] = QBrush(KColorUtils::mix(base, shade, t));
    }

    for (const auto &[item, brush] : m_brushes) {
        oldBrushes.erase(item);
        const QModelIndex idx = createIndex(item->row(), 0, item);
        Q_EMIT dataChanged(idx, idx);
    }

    for (const auto &[item, brush] : oldBrushes) {
        const QModelIndex idx = createIndex(item->row(), 0, item);
        Q_EMIT dataChanged(idx, idx);
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
            parent->removeChild(item);
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
    disconnect(doc, &KTextEditor::Document::documentNameChanged, this, &KateFileTreeModel::documentNameChanged);
    disconnect(doc, &KTextEditor::Document::documentUrlChanged, this, &KateFileTreeModel::documentNameChanged);
    disconnect(doc, &KTextEditor::Document::modifiedChanged, this, &KateFileTreeModel::documentModifiedChanged);
    disconnect(doc, &KTextEditor::Document::modifiedOnDisk, this, &KateFileTreeModel::documentModifiedOnDisc);

    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return;
    }

    if (m_shadingEnabled) {
        ProxyItem *toRemove = it.value();
        m_brushes.erase(toRemove);
        m_viewHistory.erase(std::remove(m_viewHistory.begin(), m_viewHistory.end(), toRemove), m_viewHistory.end());
        m_editHistory.erase(std::remove(m_editHistory.begin(), m_editHistory.end(), toRemove), m_editHistory.end());
    }

    ProxyItem *node = it.value();
    ProxyItemDir *parent = node->parent();

    const QModelIndex parent_index = (parent == m_root) ? QModelIndex() : createIndex(parent->row(), 0, parent);
    beginRemoveRows(parent_index, node->row(), node->row());
    node->parent()->removeChild(node);
    endRemoveRows();

    delete node;
    handleEmptyParents(parent);

    m_docmap.erase(it);
}

void KateFileTreeModel::documentNameChanged(KTextEditor::Document *doc)
{
    auto it = m_docmap.find(doc);
    if (it == m_docmap.end()) {
        return;
    }

    handleNameChange(it.value());
    Q_EMIT triggerViewChangeAfterNameChange(); // FIXME: heh, non-standard signal?
}

ProxyItemDir *KateFileTreeModel::findRootNode(const QString &name, const int r) const
{
    const auto rootChildren = m_root->children();
    for (ProxyItem *item : rootChildren) {
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

ProxyItemDir *KateFileTreeModel::findChildNode(const ProxyItemDir *parent, const QString &name)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(!name.isEmpty());

    if (!parent->childCount()) {
        return nullptr;
    }

    const auto children = parent->children();
    for (ProxyItem *item : children) {
        if (!item->flag(ProxyItem::Dir)) {
            continue;
        }

        if (item->display() == name) {
            return static_cast<ProxyItemDir *>(item);
        }
    }

    return nullptr;
}

void KateFileTreeModel::insertItemInto(ProxyItemDir *root, ProxyItem *item, bool move, ProxyItemDir **moveDest)
{
    Q_ASSERT(root != nullptr);
    Q_ASSERT(item != nullptr);

    QString tail = item->path();
    tail.remove(0, root->path().length());
    QStringList parts = tail.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    ProxyItemDir *ptr = root;
    QStringList current_parts;
    current_parts.append(root->path());

    // seems this can be empty, see bug 286191
    if (!parts.isEmpty()) {
        parts.pop_back();
    }

    for (const QString &part : std::as_const(parts)) {
        current_parts.append(part);
        ProxyItemDir *find = findChildNode(ptr, part);
        if (!find) {
            // One of child's parent dir didn't exist, create it
            // This is like you have a folder:
            // folder/dir/dir2/a.c
            // folder/b.c
            // if only a.c is opened, then we only show dir2,
            // but if you open b.c, we now need to create a new root i.e., "folder"
            // and since a.c lives in a child dir, we create "dir" as well.
            const QString new_name = current_parts.join(QLatin1Char('/'));
            const QModelIndex parent_index = (ptr == m_root) ? QModelIndex() : createIndex(ptr->row(), 0, ptr);
            beginInsertRows(parent_index, ptr->childCount(), ptr->childCount());
            ptr = new ProxyItemDir(new_name, ptr);
            endInsertRows();
        } else {
            ptr = find;
        }
    }

    if (!move) {
        // We are not moving rows, this is all new stuff
        const QModelIndex parent_index = (ptr == m_root) ? QModelIndex() : createIndex(ptr->row(), 0, ptr);
        beginInsertRows(parent_index, ptr->childCount(), ptr->childCount());
        ptr->addChild(item);
        endInsertRows();
    } else {
        // We are moving
        *moveDest = ptr;
    }
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
    const auto rootChildren = m_root->children();
    for (ProxyItem *root : rootChildren) {
        if (root == new_root || !root->flag(ProxyItem::Dir)) {
            continue;
        }

        if (root->path().startsWith(base)) {
            // We can't move directly because this items parent directories might not be in the model yet
            // so check and insert them first. Then find out where we need to move
            ProxyItemDir *moveDest = nullptr;
            insertItemInto(new_root, root, true, &moveDest);

            const QModelIndex destParent = (moveDest == m_root) ? QModelIndex() : createIndex(moveDest->row(), 0, moveDest);
            // We are moving from topLevel root to maybe some child node
            // We MUST move, otherwise if "root" was expanded, it will be collapsed if we did a remove + insert instead.
            // This is the reason for added complexity in insertItemInto
            beginMoveRows(QModelIndex(), root->row(), root->row(), destParent, moveDest->childCount());
            m_root->removeChild(root);
            moveDest->addChild(root);
            endMoveRows();
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

        const auto rootChildren = m_root->children();
        for (ProxyItem *root : rootChildren) {
            if (root == check_root || !root->flag(ProxyItem::Dir)) {
                continue;
            }

            if (check_root->display() == root->display()) {
                bool changed = false;
                bool check_root_removed = false;

                const QString rdir = root->path().section(QLatin1Char('/'), 0, -2);
                if (!rdir.isEmpty()) {
                    beginRemoveRows(QModelIndex(), root->row(), root->row());
                    m_root->removeChild(root);
                    endRemoveRows();

                    ProxyItemDir *irdir = new ProxyItemDir(rdir);
                    beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
                    m_root->addChild(irdir);
                    endInsertRows();

                    insertItemInto(irdir, root);

                    const auto children = m_root->children();
                    for (ProxyItem *node : children) {
                        if (node == irdir || !root->flag(ProxyItem::Dir)) {
                            continue;
                        }

                        const QString xy = rdir + QLatin1Char('/');
                        if (node->path().startsWith(xy)) {
                            beginRemoveRows(QModelIndex(), node->row(), node->row());
                            // check_root_removed must be sticky
                            check_root_removed = check_root_removed || (node == check_root);
                            m_root->removeChild(node);
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
                        m_root->removeChild(check_root);
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
        } // for root
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
        Q_EMIT dataChanged(idx, idx);
        return;
    }

    // in either case (new/change) we want to remove the item from its parent
    ProxyItemDir *parent = item->parent();

    const QModelIndex parent_index = (parent == m_root) ? QModelIndex() : createIndex(parent->row(), 0, parent);
    beginRemoveRows(parent_index, item->row(), item->row());
    parent->removeChild(item);
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

void KateFileTreeModel::updateItemPathAndHost(ProxyItem *item)
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
            path = QStringLiteral("[%1]%2").arg(host, path);
        }
    }

    // for some reason we get useless name changes [should be fixed in 5.0]
    if (item->path() == path) {
        return;
    }

    item->setPath(path);
    item->setHost(host);
}

void KateFileTreeModel::setupIcon(ProxyItem *item)
{
    Q_ASSERT(item != nullptr);
    Q_ASSERT(item->doc() != nullptr);

    // use common method as e.g. in tabbar, too
    item->setIcon(Utils::iconForDocument(item->doc()));
}

void KateFileTreeModel::resetHistory()
{
    QSet<ProxyItem *> list{m_viewHistory.begin(), m_viewHistory.end()};
    list += QSet<ProxyItem *>{m_editHistory.begin(), m_editHistory.end()};

    m_viewHistory.clear();
    m_editHistory.clear();
    m_brushes.clear();

    for (ProxyItem *item : std::as_const(list)) {
        QModelIndex idx = createIndex(item->row(), 0, item);
        Q_EMIT dataChanged(idx, idx, QList<int>(1, Qt::BackgroundRole));
    }
}

void KateFileTreeModel::addWidget(QWidget *w)
{
    if (!w) {
        return;
    }

    const QModelIndex parentIdx = createIndex(m_widgetsRoot->row(), 0, m_widgetsRoot);
    beginInsertRows(parentIdx, m_widgetsRoot->childCount(), m_widgetsRoot->childCount());
    auto item = new ProxyItem(w->windowTitle());
    item->setFlag(ProxyItem::Widget);
    item->setIcon(w->windowIcon());
    item->setWidget(w);
    m_widgetsRoot->addChild(item);
    endInsertRows();
}

void KateFileTreeModel::removeWidget(QWidget *w)
{
    ProxyItem *item = nullptr;
    const auto items = m_widgetsRoot->children();
    for (auto *it : items) {
        if (it->widget() == w) {
            item = it;
            break;
        }
    }
    if (!item) {
        return;
    }

    const QModelIndex parentIdx = createIndex(m_widgetsRoot->row(), 0, m_widgetsRoot);
    beginRemoveRows(parentIdx, item->row(), item->row());
    m_widgetsRoot->removeChild(item);
    endRemoveRows();
    delete item;
}

#include "katefiletreemodel.moc"
#include "moc_katefiletreemodel.cpp"
