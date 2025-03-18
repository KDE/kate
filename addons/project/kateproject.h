/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/Document>

#include "git/gitstatus.h"
#include <QHash>
#include <QPointer>
#include <QStandardItemModel>
#include <memory>

class QTextDocument;
class KateProjectItem;
class KateProjectIndex;

class KateProjectModel : public QStandardItemModel
{
public:
    using QStandardItemModel::QStandardItemModel;

    enum StatusType {
        Invalid,
        Added,
        Modified,
        None
    };
    enum Role {
        StatusRole = Qt::UserRole + 2
    };

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction;
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void setStatus(const GitUtils::GitParsedStatus &status)
    {
        m_status = status;
        m_cachedStatusByPath = {};
    }

private:
    StatusType getStatusTypeForPath(const QString &) const;

    friend class KateProject;
    QPointer<class KateProject> m_project;
    GitUtils::GitParsedStatus m_status;

    mutable QHash<QString, StatusType> m_cachedStatusByPath;
};

/**
 * Shared pointer data types.
 * Used to pass pointers over queued connected slots
 */
typedef std::shared_ptr<QStandardItem> KateProjectSharedQStandardItem;
Q_DECLARE_METATYPE(KateProjectSharedQStandardItem)

typedef std::shared_ptr<QHash<QString, KateProjectItem *>> KateProjectSharedQHashStringItem;
Q_DECLARE_METATYPE(KateProjectSharedQHashStringItem)

typedef std::shared_ptr<KateProjectIndex> KateProjectSharedProjectIndex;
Q_DECLARE_METATYPE(KateProjectSharedProjectIndex)

class KateProjectPlugin;
class QThreadPool;

/**
 * Class representing a project.
 * Holds project properties like name, groups, contained files, ...
 */
class KateProject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString baseDir READ baseDir)
    Q_PROPERTY(QString name READ name)

public:
    /**
     * Construct project by reading from given file.
     * Success can be checked later by using isValid().
     *
     * @param threadPool thread pool to be used by worker threads
     * @param plugin our plugin instance, for config & file system watcher
     * @param fileName fileName to load the project from
     */
    KateProject(QThreadPool &threadPool, KateProjectPlugin *plugin, const QString &fileName);

    /**
     * Construct project from given data for given base directory
     * Success can be checked later by using isValid().
     *
     * @param threadPool thread pool to be used by worker threads
     * @param plugin our plugin instance, for config & file system watcher
     * @param globalProject globalProject object content
     * @param directory project base directory
     */
    KateProject(QThreadPool &threadPool, KateProjectPlugin *plugin, const QVariantMap &globalProject, const QString &directory);

    /**
     * deconstruct project
     */
    ~KateProject() override;

    /**
     * Is this project valid?
     * @return project valid? we are valid, if we have some name set
     */
    bool isValid() const
    {
        return !name().isEmpty();
    }

    /**
     * Is this a file backed project or just generated from e.g. opening a directory or VCS?
     * @return file backed project? e.g. was this read from a .kateproject file?
     */
    bool isFileBacked() const
    {
        return m_fileBacked;
    }

    /**
     * Try to reload a project.
     * If the reload fails, e.g. because the file is not readable or corrupt, nothing will happen!
     * @param force will enforce the worker to update files list and co even if the content of the file was not changed!
     * @return success
     */
    bool reload(bool force = false);

    /**
     * Accessor to file name.
     * Even for projects generated from version control or by open directory we will create a fake name,
     * as the project file name is used in many places as unique identifier for the project.
     * @return file name
     */
    const QString &fileName() const
    {
        return m_fileName;
    }

    /**
     * Return the base directory of this project.
     * @return base directory of project, might not be the directory of the fileName!
     */
    const QString &baseDir() const
    {
        return m_baseDir;
    }

    /**
     * Accessor to project map containing the whole project info.
     * @return project info
     */
    const QVariantMap &projectMap() const
    {
        return m_projectMap;
    }

    /**
     * Accessor to project name.
     * @return project name
     */
    QString name() const
    {
        return m_projectMap[QStringLiteral("name")].toString();
    }

    /**
     * Accessor for the model.
     * @return model of this project
     */
    QStandardItemModel *model()
    {
        return &m_model;
    }

    /**
     * Flat list of all files in the project
     * @return list of files in project
     */
    QStringList files()
    {
        return m_file2Item ? m_file2Item->keys() : QStringList();
    }

    /**
     * get item for file
     * @param file file to get item for
     * @return item for given file or 0
     */
    KateProjectItem *itemForFile(const QString &file)
    {
        return m_file2Item ? m_file2Item->value(file) : nullptr;
    }

    /**
     * add a new file to the project
     */
    bool addFile(const QString &file, KateProjectItem *item)
    {
        if (m_file2Item && item && !m_file2Item->contains(file)) {
            (*m_file2Item)[file] = item;
            return true;
        }
        return false;
    }

    /**
     * rename a file
     */
    void renameFile(const QString &newName, const QString &oldName);

    /**
     * remove a file
     */
    void removeFile(const QString &file);

    /**
     * Access to project index.
     * May be null.
     * Don't store this pointer, might change.
     * @return project index
     */
    KateProjectIndex *projectIndex()
    {
        return m_projectIndex.get();
    }

    KateProjectPlugin *plugin()
    {
        return m_plugin;
    }

    /**
     * Computes a suitable file name for the given suffix.
     * If you e.g. want to store a "notes" file, you could pass "notes" and get
     * the full path to projectbasedir/.kateproject.notes
     * @param suffix suffix for the file
     * @return full path for project local file, on error => empty string
     */
    QString projectLocalFileName(const QString &suffix) const;

    /**
     * Document with project local notes.
     * Will be stored in a projectLocalFile "notes.txt".
     * @return notes document
     */
    QTextDocument *notesDocument();

    /**
     * Save the notes document to "notes.txt" if any document around.
     */
    void saveNotesDocument();

    /**
     * Register a document for this project.
     * @param document document to register
     */
    void registerDocument(KTextEditor::Document *document);

    /**
     * Unregister a document for this project.
     * @param document document to unregister
     */
    void unregisterDocument(KTextEditor::Document *document);

    /**
     * All project roots, files below these roots are considered to be part of the project.
     * This includes the directory with the project file, the baseDir and the build directory.
     * Paths will be stored as absolute and canonical variants.
     *
     * This is used e.g. in KateProjectPlugin::openProjectForDirectory
     *
     * @return project root directories for fast lookup
     */
    const QSet<QString> &projectRoots() const
    {
        return m_projectRoots;
    }

    /*
     * For a given path, find the item corresponding to
     * the last path part e.g., for "myProject/dir1/dir2/"
     * return the item for "dir2" if found or nullptr otherwise
     */
    QStandardItem *itemForPath(const QString &path) const;

private Q_SLOTS:
    bool load(const QVariantMap &globalProject, bool force = false);

    /**
     * Used for worker to send back the results of project loading
     * @param topLevel new toplevel element for model
     * @param file2Item new file => item mapping
     */
    void loadProjectDone(const KateProjectSharedQStandardItem &topLevel, KateProjectSharedQHashStringItem file2Item);

    /**
     * Used for worker to send back the results of index loading
     * @param projectIndex new project index
     */
    void loadIndexDone(KateProjectSharedProjectIndex projectIndex);

    void slotModifiedChanged(KTextEditor::Document *);

    void slotModifiedOnDisk(KTextEditor::Document *document, bool isModified, KTextEditor::Document::ModifiedOnDiskReason reason);

    /**
     * did some project file change?
     * @param file name of file that did change
     */
    void slotFileChanged(const QString &file);

Q_SIGNALS:
    /**
     * Emitted on project map changes.
     * This includes the name!
     */
    void projectMapChanged();

    /**
     * Emitted on model changes.
     * This includes the files list, itemForFile mapping!
     */
    void modelChanged();

    /**
     * Emitted when the index creation is finished.
     * This includes the ctags index.
     */
    void indexChanged();

private:
    void registerUntrackedDocument(KTextEditor::Document *document);
    void unregisterUntrackedItem(const KateProjectItem *item);
    QVariantMap readProjectFile() const;
    /**
     * Read a JSON document from file.
     *
     * In case of an error, the returned object verifies isNull() is true.
     */
    QJsonDocument readJSONFile(const QString &fileName) const;

    /**
     * update project root directories, see projectRoots()
     */
    void updateProjectRoots();

private:
    /**
     * thread pool used for project worker
     */
    QThreadPool &m_threadPool;

    /**
     * Project plugin (configuration)
     */
    KateProjectPlugin *const m_plugin;

    /**
     * file backed project? e.g. was this read from a .kateproject file?
     */
    const bool m_fileBacked;

    /**
     * project file name, will stay constant
     */
    const QString m_fileName;

    /**
     * base directory of the project
     */
    QString m_baseDir;

    /**
     * project name
     */
    QString m_name;

    /**
     * variant map representing the project
     */
    QVariantMap m_projectMap;

    /**
     * standard item model with content of this project
     */
    KateProjectModel m_model;

    /**
     * mapping files => items
     */
    KateProjectSharedQHashStringItem m_file2Item;

    /**
     * project index, if any
     */
    KateProjectSharedProjectIndex m_projectIndex;

    /**
     * notes buffer for project local notes
     */
    QTextDocument *m_notesDocument = nullptr;

    /**
     * Set of existing documents for this project.
     */
    QHash<KTextEditor::Document *, QString> m_documents;

    /**
     * Parent item for existing documents that are not in the project tree
     */
    QStandardItem *m_untrackedDocumentsRoot = nullptr;

    /**
     * project configuration (read from file or injected)
     */
    QVariantMap m_globalProject;

    /**
     * project root directories, see projectRoots()
     */
    QSet<QString> m_projectRoots;
};
