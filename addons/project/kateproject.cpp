/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateproject.h"
#include "kateprojectitem.h"
#include "kateprojectplugin.h"
#include "kateprojectworker.h"

#include <KIO/CopyJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <QTextDocument>

#include <json_utils.h>
#include <ktexteditor_utils.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMimeData>
#include <QPlainTextDocumentLayout>
#include <utility>

QMimeData *KateProjectModel::mimeData(const QModelIndexList &indexes) const
{
    auto *data = new QMimeData;
    QList<QUrl> urls;
    for (auto &index : indexes) {
        if (index.data(KateProjectItem::TypeRole) == KateProjectItem::File) {
            urls.push_back(QUrl::fromLocalFile(index.data(Qt::UserRole).toString()));
        }
    }
    data->setUrls(urls);
    return data;
}

bool KateProjectModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent)) {
        return false;
    }

    const auto index = this->index(row, column, parent);
    const auto type = (KateProjectItem::Type)index.data(KateProjectItem::TypeRole).toInt();
    const auto parentType = (KateProjectItem::Type)parent.data(KateProjectItem::TypeRole).toInt();
    QString targetPath;
    if (!index.isValid() && parent.isValid() && parentType == KateProjectItem::Directory) {
        targetPath = parent.data(Qt::UserRole).toString();
    } else if (index.isValid() && type == KateProjectItem::File) {
        if (index.parent().isValid()) {
            targetPath = index.parent().data(Qt::UserRole).toString();
        } else {
            targetPath = m_project->baseDir();
        }
    } else if (!index.isValid() && !parent.isValid()) {
        targetPath = m_project->baseDir();
    }

    // the `if (!d.exists())` after this won't work for empty paths
    // and will end up moving / copying stuff to the kate current working directory, which can be anything...
    // so check for empty paths here
    // this can happen when trying to drop a file between 2 folders
    if (targetPath.isEmpty()) {
        return false;
    }

    const QDir d = targetPath;
    if (!d.exists()) {
        return false;
    }

    const auto urls = data->urls();
    const QString destDir = d.absolutePath();
    const QUrl dest = QUrl::fromLocalFile(destDir);

    for (auto &url : urls) {
        // don't allow moving it to the same folder its already in
        if (url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash | QUrl::RemoveScheme)
            == dest.adjusted(QUrl::StripTrailingSlash | QUrl::RemoveScheme)) {
            return false;
        }
    }

    QPointer<KIO::CopyJob> job;

    if (action == Qt::CopyAction) {
        job = KIO::copy(urls, dest);
    } else {
        job = KIO::move(urls, dest);
    }

    KJobWidgets::setWindow(job, QApplication::activeWindow());
    connect(job, &KIO::Job::finished, this, [this, action, originalUrls = urls, job, destDir] {
        if (!job || job->error() != 0 || !m_project) {
            return;
        }

        if (action == Qt::MoveAction) {
            // remove the files we are moving if they are from this project
            // this avoids some files being invisible until reloaded
            // this is kinda hacky tbh, idk if there's a better solution
            for (auto &url : originalUrls) {
                auto path = url.toLocalFile();
                if (m_project->itemForFile(path)) {
                    m_project->removeFile(path);
                }
            }
        }

        bool needsReload = false;
        QStandardItem *item = invisibleRootItem();
        if (destDir != m_project->baseDir()) {
            const auto indexes = match(this->index(0, 0), Qt::UserRole, destDir, 1, Qt::MatchStartsWith | Qt::MatchRecursive);
            if (indexes.empty()) {
                needsReload = true;
            } else {
                item = itemFromIndex(indexes.constFirst());
            }
        }

        const auto srcUrls = job->srcUrls();
        if (!needsReload) {
            for (const auto &url : srcUrls) {
                const QString newFile = destDir + QStringLiteral("/") + url.fileName();
                const QFileInfo fi(newFile);
                if (fi.exists() && fi.isFile()) {
                    auto *i = new KateProjectItem(KateProjectItem::File, url.fileName(), fi.absoluteFilePath());
                    if (m_project->addFile(newFile, i)) {
                        item->appendRow(i);
                    } else {
                        delete i;
                    }
                } else {
                    // not a file? Just do a reload of the project on finish
                    needsReload = true;
                    break;
                }
            }
        }
        if (needsReload && m_project) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    m_project->reload(true);
                },
                Qt::QueuedConnection);
        }
    });
    job->start();

    return true;
}

bool KateProjectModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int, int, const QModelIndex &) const
{
    return data && data->hasUrls() && (action == Qt::CopyAction || action == Qt::MoveAction);
}

Qt::ItemFlags KateProjectModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);

    auto type = index.data(KateProjectItem::TypeRole);
    if (type == KateProjectItem::File) {
        flags.setFlag(Qt::ItemIsDropEnabled, false);
        flags.setFlag(Qt::ItemIsDragEnabled, true);
    } else if (type == KateProjectItem::Directory) {
        flags.setFlag(Qt::ItemIsDropEnabled, true);
        flags.setFlag(Qt::ItemIsDragEnabled, false);
    }

    return flags;
}

static bool matchesAny(QStringView pathView, const QList<GitUtils::StatusItem> &items)
{
    auto pathParent = [](QByteArrayView path) {
        int lastSlash = path.lastIndexOf('/');
        return lastSlash == -1 ? QByteArrayView() : path.mid(0, lastSlash);
    };
    for (const auto &m : items) {
        if (pathView == QLatin1String(m.file)) {
            return true;
        } else {
            QByteArrayView parent = pathParent(m.file);
            while (!parent.isEmpty()) {
                if (pathView == QLatin1String(parent.data(), parent.size())) {
                    return true;
                }
                parent = pathParent(parent);
            }
        }
    }
    return false;
}

KateProjectModel::StatusType KateProjectModel::getStatusTypeForPath(const QString &path) const
{
    if (auto cached = m_cachedStatusByPath.value(path, Invalid); cached != Invalid) {
        return cached;
    } else {
        QStringView pathView = QStringView(path).mid(m_project->baseDir().size() + 1);
        if (matchesAny(pathView, m_status.changed)) {
            m_cachedStatusByPath[path] = Modified;
            return Modified;
        } else if (matchesAny(pathView, m_status.staged)) {
            m_cachedStatusByPath[path] = Added;
            return Added;
        } else {
            m_cachedStatusByPath[path] = None;
            return None;
        }
    }
}

QVariant KateProjectModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole) {
        auto type = getStatusTypeForPath(index.data(Qt::UserRole).toString());
        if (type == None) {
            return {};
        } else if (type == Modified) {
            return tr("Modified");
        } else if (type == Added) {
            return tr("Staged");
        }
        return {};
    } else if (role == StatusRole) {
        return getStatusTypeForPath(index.data(Qt::UserRole).toString());
    }

    return QStandardItemModel::data(index, role);
}

KateProject::KateProject(QThreadPool &threadPool, KateProjectPlugin *plugin, const QString &fileName)
    : m_threadPool(threadPool)
    , m_plugin(plugin)
    , m_fileBacked(true)
    , m_fileName(QFileInfo(fileName).absoluteFilePath())
    , m_baseDir(QFileInfo(fileName).absolutePath())
{
    // link model
    m_model.m_project = this;

    // ensure we get notified for project file changes
    connect(&m_plugin->fileWatcher(), &QFileSystemWatcher::fileChanged, this, &KateProject::slotFileChanged);
    m_plugin->fileWatcher().addPath(m_fileName);

    // try to load the project map from our file, will start worker thread, too
    reload();

    // fix roots
    updateProjectRoots();
}

KateProject::KateProject(QThreadPool &threadPool, KateProjectPlugin *plugin, const QVariantMap &globalProject, const QString &directory)
    : m_threadPool(threadPool)
    , m_plugin(plugin)
    , m_fileBacked(false)
    , m_fileName(QDir(QDir(directory).absolutePath()).filePath(QStringLiteral(".kateproject")))
    , m_baseDir(QDir(directory).absolutePath())
    , m_globalProject(globalProject)
{
    // link model
    m_model.m_project = this;

    // try to load the project map, will start worker thread, too
    load(globalProject);

    // fix roots
    updateProjectRoots();
}

KateProject::~KateProject()
{
    saveNotesDocument();

    // stop watching if we have some real project file
    if (m_fileBacked && !m_fileName.isEmpty()) {
        m_plugin->fileWatcher().removePath(m_fileName);
    }
}

bool KateProject::reload(bool force)
{
    const QVariantMap map = readProjectFile();
    if (!map.isEmpty()) {
        m_globalProject = map;
    }

    return load(m_globalProject, force);
}

void KateProject::renameFile(const QString &newName, const QString &oldName)
{
    auto it = m_file2Item->find(oldName);
    if (it == m_file2Item->end()) {
        qWarning("renameFile() File not found, new: %ls old: %ls", qUtf16Printable(newName), qUtf16Printable(oldName));
        return;
    }
    (*m_file2Item)[newName] = it.value();
    m_file2Item->erase(it);
}

void KateProject::removeFile(const QString &file)
{
    auto it = m_file2Item->find(file);
    if (it == m_file2Item->end()) {
        qWarning("removeFile() File not found: %ls", qUtf16Printable(file));
        return;
    }
    m_file2Item->erase(it);
}

/**
 * Read a JSON document from file.
 *
 * In case of an error, the returned object verifies isNull() is true.
 */
QJsonDocument KateProject::readJSONFile(const QString &fileName) const
{
    /**
     * keep each project file last modification time to warn the user only once per malformed file.
     */
    static QHash<QString, QDateTime> lastModifiedTimes;

    if (fileName.isEmpty()) {
        return {};
    }

    QFile file(fileName);
    if (!file.exists() || !file.open(QFile::ReadOnly)) {
        return {};
    }

    /**
     * parse the whole file, bail out again on error!
     */
    const QByteArray jsonData = file.readAll();
    QJsonParseError parseError{};
    QJsonDocument document(QJsonDocument::fromJson(jsonData, &parseError));

    if (parseError.error != QJsonParseError::NoError) {
        QDateTime lastModified = QFileInfo(fileName).lastModified();
        if (lastModified > lastModifiedTimes.value(fileName, QDateTime())) {
            lastModifiedTimes[fileName] = lastModified;
            m_plugin->sendMessage(i18n("Malformed JSON file '%1': %2", fileName, parseError.errorString()), true);
        }
        return {};
    }

    return document;
}

QVariantMap KateProject::readProjectFile() const
{
    // not file back => will not work
    if (!m_fileBacked) {
        return {};
    }

    // bail out on error
    QJsonDocument project(readJSONFile(m_fileName));
    if (project.isNull()) {
        return {};
    }

    /**
     * convenience; align with auto-generated projects
     * generate 'name' and 'files' if not specified explicitly,
     * so those parts need not be given if only wishes to specify additional
     * project configuration (e.g. build, ctags)
     */
    if (project.isObject()) {
        auto dir = QFileInfo(m_fileName).dir();
        QJsonObject object = project.object();

        // if there are local settings (.kateproject.local), override values
        {
            const auto localSettings = readJSONFile(projectLocalFileName(QStringLiteral("local")));
            if (!localSettings.isNull() && localSettings.isObject()) {
                object = json::merge(object, localSettings.object());
            }
        }

        auto name = object[QLatin1String("name")];
        if (name.isUndefined() || name.isNull()) {
            name = dir.dirName();
        }
        auto files = object[QLatin1String("files")];
        if (files.isUndefined() || files.isNull()) {
            // support all we can, could try to detect,
            // but it will be sorted out upon loading anyway
            QJsonArray afiles;
            for (const auto &t : {QStringLiteral("git"), QStringLiteral("hg"), QStringLiteral("svn"), QStringLiteral("darcs")}) {
                afiles.push_back(QJsonObject{{t, true}});
            }
            files = afiles;
        }
        project.setObject(object);
    }

    return project.toVariant().toMap();
}

static void onErrorOccurred(const QString &error)
{
    static QSet<QString> notifiedErrors;
    // we only show the error once
    if (notifiedErrors.contains(error)) {
        return;
    }
    notifiedErrors.insert(error);
    Utils::showMessage(error, QIcon(), i18n("Project"), MessageType::Error);
}

bool KateProject::load(const QVariantMap &globalProject, bool force)
{
    /**
     * no name, bad => bail out
     */
    if (globalProject[QStringLiteral("name")].toString().isEmpty()) {
        return false;
    }

    /**
     * support out-of-source project files
     * ensure we handle relative paths properly => relative to the potential invented .kateproject file name
     */
    const auto baseDir = globalProject[QStringLiteral("directory")].toString();
    if (!baseDir.isEmpty()) {
        m_baseDir = QFileInfo(QFileInfo(m_fileName).dir(), baseDir).absoluteFilePath();
    }

    /**
     * anything changed?
     * else be done without forced reload!
     */
    if (!force && (m_projectMap == globalProject)) {
        return true;
    }

    /**
     * setup global attributes in this object
     */
    m_projectMap = globalProject;

    // fix roots
    updateProjectRoots();

    // emit that we changed stuff
    Q_EMIT projectMapChanged();

    // trigger loading of project in background thread
    QString indexDir;
    if (m_plugin->getIndexEnabled()) {
        indexDir = m_plugin->getIndexDirectory().toLocalFile();
        // if empty, use regular tempdir
        if (indexDir.isEmpty()) {
            indexDir = QDir::tempPath();
        }
    }

    auto column = m_model.invisibleRootItem()->takeColumn(0);
    m_untrackedDocumentsRoot = nullptr;
    m_file2Item.reset();
    auto deleter = QRunnable::create([column = std::move(column)] {
        qDeleteAll(column);
    });
    m_threadPool.start(deleter);

    // let's run the stuff in our own thread pool
    // do manual queued connect, as only run() is done in extra thread, object stays in this one
    auto w = new KateProjectWorker(m_baseDir, indexDir, m_projectMap, force);
    connect(w, &KateProjectWorker::loadDone, this, &KateProject::loadProjectDone, Qt::QueuedConnection);
    connect(w, &KateProjectWorker::loadIndexDone, this, &KateProject::loadIndexDone, Qt::QueuedConnection);
    connect(w, &KateProjectWorker::errorOccurred, this, onErrorOccurred, Qt::QueuedConnection);
    m_threadPool.start(w);

    // we are done here
    return true;
}

void KateProject::loadProjectDone(const KateProjectSharedQStandardItem &topLevel, KateProjectSharedQHashStringItem file2Item)
{
    m_model.clear();
    m_model.invisibleRootItem()->appendColumn(topLevel->takeColumn(0));
    m_untrackedDocumentsRoot = nullptr;
    m_file2Item = std::move(file2Item);

    /**
     * readd the documents that are open atm
     */
    for (auto i = m_documents.constBegin(); i != m_documents.constEnd(); i++) {
        registerDocument(i.key());
    }

    Q_EMIT modelChanged();
}

void KateProject::loadIndexDone(KateProjectSharedProjectIndex projectIndex)
{
    /**
     * move to our project
     */
    m_projectIndex = std::move(projectIndex);

    /**
     * notify external world that data is available
     */
    Q_EMIT indexChanged();
}

QString KateProject::projectLocalFileName(const QString &suffix) const
{
    /**
     * nothing on empty file names for project
     * should not happen
     */
    if (m_baseDir.isEmpty() || suffix.isEmpty()) {
        return {};
    }

    /**
     * compute full file name
     */
    return QDir(m_baseDir).filePath(QStringLiteral(".kateproject.") + suffix);
}

QTextDocument *KateProject::notesDocument()
{
    /**
     * already there?
     */
    if (m_notesDocument) {
        return m_notesDocument;
    }

    /**
     * else create it
     */
    m_notesDocument = new QTextDocument(this);
    m_notesDocument->setDocumentLayout(new QPlainTextDocumentLayout(m_notesDocument));

    /**
     * get file name
     */
    const QString notesFileName = projectLocalFileName(QStringLiteral("notes"));
    if (notesFileName.isEmpty()) {
        return m_notesDocument;
    }

    /**
     * and load text if possible
     */
    QFile inFile(notesFileName);
    if (inFile.open(QIODevice::ReadOnly)) {
        QTextStream inStream(&inFile);
        m_notesDocument->setPlainText(inStream.readAll());
    }

    /**
     * and be done
     */
    return m_notesDocument;
}

void KateProject::saveNotesDocument()
{
    /**
     * no notes document, nothing to do
     */
    if (!m_notesDocument) {
        return;
    }

    /**
     * get content & filename
     */
    const QString content = m_notesDocument->toPlainText();
    const QString notesFileName = projectLocalFileName(QStringLiteral("notes"));
    if (notesFileName.isEmpty()) {
        return;
    }

    /**
     * no content => unlink file, if there
     */
    if (content.isEmpty()) {
        if (QFile::exists(notesFileName)) {
            QFile::remove(notesFileName);
        }
        return;
    }

    /**
     * else: save content to file
     */
    QFile outFile(projectLocalFileName(QStringLiteral("notes")));
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream outStream(&outFile);
        outStream << content;
    }
}

void KateProject::slotModifiedChanged(KTextEditor::Document *document)
{
    KateProjectItem *item = itemForFile(m_documents.value(document));

    if (!item) {
        return;
    }

    item->slotModifiedChanged(document);
}

void KateProject::slotModifiedOnDisk(KTextEditor::Document *document, bool isModified, KTextEditor::Document::ModifiedOnDiskReason reason)
{
    KateProjectItem *item = itemForFile(m_documents.value(document));

    if (!item) {
        return;
    }

    item->slotModifiedOnDisk(document, isModified, reason);
}

QStandardItem *KateProject::itemForPath(const QString &path) const
{
    auto findInParent = [](const QString &name, QStandardItem *parent) {
        for (int i = 0; i < parent->rowCount(); ++i) {
            if (parent->child(i)->text() == name) {
                return parent->child(i);
            }
        }
        return static_cast<QStandardItem *>(nullptr);
    };

    const QStringList parts = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
    auto root = m_model.invisibleRootItem();
    if (parts.empty()) {
        return nullptr;
    }
    QStandardItem *parent = root;
    for (const QString &part : parts) {
        parent = findInParent(part, parent);
        if (!parent) {
            return nullptr;
        }
    }
    return parent;
}

void KateProject::registerDocument(KTextEditor::Document *document)
{
    // remember the document, if not already there
    QString path = document->url().toLocalFile();
    if (!m_documents.contains(document)) {
        m_documents[document] = path;
    }

    // try to get item for the document
    KateProjectItem *item = itemForFile(path);

    // a new document that we don't know? If it belong to the project add it
    if (!item && path.startsWith(m_baseDir)) {
        path = path.remove(m_baseDir);
        // remove filename
        int lastSlash = path.lastIndexOf(u'/');
        if (lastSlash != -1) {
            path = path.remove(lastSlash, path.size() - lastSlash);
            // find the item for parent directory of this file
            auto dir = itemForPath(path);
            // if found, add this file to the directory
            if (dir && dir->data(KateProjectItem::TypeRole).value<int>() == KateProjectItem::Directory) {
                QFileInfo fi(document->url().toLocalFile());
                auto *i = new KateProjectItem(KateProjectItem::File, fi.fileName(), fi.absoluteFilePath());
                if (addFile(path, i)) {
                    dir->appendRow(i);
                    dir->sortChildren(0);
                    item = i;
                } else {
                    delete i;
                }
            }
        }
    }

    // if we got one, we are done, else create a dummy!
    // clang-format off
    if (item) {
        disconnect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
        disconnect(document, &KTextEditor::Document::modifiedOnDisk, this, &KateProject::slotModifiedOnDisk);
        item->slotModifiedChanged(document);

        /*FIXME    item->slotModifiedOnDisk(document,document->isModified(),qobject_cast<KTextEditor::ModificationInterface*>(document)->modifiedOnDisk());
         * FIXME*/

        connect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
        connect(document, &KTextEditor::Document::modifiedOnDisk, this, &KateProject::slotModifiedOnDisk);

        return;
    }
    // clang-format on

    registerUntrackedDocument(document);
}

void KateProject::registerUntrackedDocument(KTextEditor::Document *document)
{
    // perhaps create the parent item
    if (!m_untrackedDocumentsRoot) {
        m_untrackedDocumentsRoot = new KateProjectItem(KateProjectItem::Directory, i18n("<untracked>"), QString());
        m_model.insertRow(0, m_untrackedDocumentsRoot);
    }

    // create document item
    QFileInfo fileInfo(document->url().toLocalFile());
    auto *fileItem = new KateProjectItem(KateProjectItem::File, fileInfo.fileName(), document->url().toLocalFile());
    fileItem->slotModifiedChanged(document);
    connect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
    connect(document, &KTextEditor::Document::modifiedOnDisk, this, &KateProject::slotModifiedOnDisk);

    bool inserted = false;
    for (int i = 0; i < m_untrackedDocumentsRoot->rowCount(); ++i) {
        if (m_untrackedDocumentsRoot->child(i)->data(Qt::UserRole).toString() > document->url().toLocalFile()) {
            m_untrackedDocumentsRoot->insertRow(i, fileItem);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        m_untrackedDocumentsRoot->appendRow(fileItem);
    }

    fileItem->setData(QVariant(true), Qt::UserRole + 3);

    if (!m_file2Item) {
        m_file2Item = KateProjectSharedQHashStringItem(new QHash<QString, KateProjectItem *>());
    }
    (*m_file2Item)[document->url().toLocalFile()] = fileItem;
}

void KateProject::unregisterDocument(KTextEditor::Document *document)
{
    if (!m_documents.contains(document)) {
        return;
    }

    // ignore further updates but clear state once
    disconnect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
    const QString &file = m_documents.value(document);
    auto *item = itemForFile(file);
    if (item) {
        item->slotModifiedChanged(nullptr);
    }

    if (m_untrackedDocumentsRoot) {
        if (item && item->data(Qt::UserRole + 3).toBool()) {
            unregisterUntrackedItem(item);
            m_file2Item->remove(file);
        }
    }

    m_documents.remove(document);
}

void KateProject::unregisterUntrackedItem(const KateProjectItem *item)
{
    for (int i = 0; i < m_untrackedDocumentsRoot->rowCount(); ++i) {
        if (m_untrackedDocumentsRoot->child(i) == item) {
            m_untrackedDocumentsRoot->removeRow(i);
            break;
        }
    }

    if (m_untrackedDocumentsRoot->rowCount() < 1) {
        m_model.removeRow(0);
        m_untrackedDocumentsRoot = nullptr;
    }
}

void KateProject::slotFileChanged(const QString &file)
{
    if (file == m_fileName) {
        reload();
    }
}

void KateProject::updateProjectRoots()
{
    m_projectRoots.clear();

    const auto addPath = [this](const QString &dir) {
        if (!dir.isEmpty()) {
            m_projectRoots.insert(QFileInfo(dir).absoluteFilePath());
            if (const auto canonical = QFileInfo(dir).canonicalFilePath(); !canonical.isEmpty()) {
                m_projectRoots.insert(canonical);
            }
        }
    };

    addPath(QFileInfo(fileName()).absolutePath());
    addPath(baseDir());
    addPath(projectMap().value(QStringLiteral("build")).toMap().value(QStringLiteral("directory")).toString());
}

#include "moc_kateproject.cpp"
