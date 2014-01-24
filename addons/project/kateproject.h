/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_PROJECT_H
#define KATE_PROJECT_H

#include <QThread>
#include <QMap>
#include <QSharedPointer>
#include <QTextDocument>
#include <KTextEditor/ModificationInterface>
#include "kateprojectindex.h"
#include "kateprojectitem.h"

/**
 * Shared pointer data types.
 * Used to pass pointers over queued connected slots
 */
typedef QSharedPointer<QStandardItem> KateProjectSharedQStandardItem;
Q_DECLARE_METATYPE(KateProjectSharedQStandardItem)

typedef QSharedPointer<QMap<QString, KateProjectItem *> > KateProjectSharedQMapStringItem;
Q_DECLARE_METATYPE(KateProjectSharedQMapStringItem)

typedef QSharedPointer<KateProjectIndex> KateProjectSharedProjectIndex;
Q_DECLARE_METATYPE(KateProjectSharedProjectIndex)

/**
 * Private worker thread.
 * Will take care of worker object deletion.
 */
class KateProjectWorkerThread : public QThread
{
    public:
        /**
         * Construct thread for given worker
         */
        KateProjectWorkerThread (QObject *worker)
          : QThread()
          , m_worker (worker)
        {
        }

    protected:
        /**
         * start the thread event loop
         */
        virtual void run()
        {
            /**
             * run event loop
             */
            exec ();

            /**
             * kill worker in THIS thread
             */
            delete m_worker;
        }

    private:
        /**
         * Worker object
         */
        QObject *m_worker;
};

/**
 * Class representing a project.
 * Holds project properties like name, groups, contained files, ...
 */
class KateProject : public QObject
{
  Q_OBJECT

  public:
    /**
     * construct empty project
     */
    KateProject ();

    /**
     * deconstruct project
     */
    ~KateProject ();

    /**
     * Load a project.
     * Only works once, afterwards use reload().
     * @param fileName name of project file
     * @return success
     */
    bool load (const QString &fileName);

    /**
     * Try to reload a project.
     * If the reload fails, e.g. because the file is not readable or corrupt, nothing will happen!
     * @param force will enforce the worker to update files list and co even if the content of the file was not changed!
     * @return success
     */
    bool reload (bool force = false);

    /**
     * Accessor to file name.
     * @return file name
     */
    const QString &fileName () const
    {
      return m_fileName;
    }
   
    /**
     * Return the base directory of this project.
     * @return base directory of project, might not be the directory of the fileName!
     */
    const QString &baseDir () const
    {
      return m_baseDir;
    }

    /**
     * Accessor to project map containing the whole project info.
     * @return project info
     */
    const QVariantMap &projectMap () const
    {
      return m_projectMap;
    }

    /**
     * Accessor to project name.
     * @return project name
     */
    QString name () const
    {
      return m_projectMap[QStringLiteral("name")].toString ();
    }

    /**
     * Accessor for the model.
     * @return model of this project
     */
    QStandardItemModel *model ()
    {
      return &m_model;
    }

    /**
     * Flat list of all files in the project
     * @return list of files in project
     */
    QStringList files ()
    {
      return m_file2Item ? m_file2Item->keys () : QStringList ();
    }

    /**
     * get item for file
     * @param file file to get item for
     * @return item for given file or 0
     */
    KateProjectItem *itemForFile (const QString &file)
    {
      return m_file2Item ? m_file2Item->value (file) : 0;
    }

    /**
     * Access to project index.
     * May be null.
     * Don't store this pointer, might change.
     * @return project index
     */
    KateProjectIndex *projectIndex ()
    {
      return m_projectIndex.data();
    }
    
    /**
     * Will try to open a project local file.
     * Such files will be stored as .kateproject.d/file in the project directory.
     * @param file wanted file name, relative to .kateproject.d folder in project directory
     * @return either a pointer to a read-write opened file or null on error
     */
    QFile *projectLocalFile (const QString &file) const;

    /**
     * Document with project local notes.
     * Will be stored in a projectLocalFile "notes.txt".
     * @return notes document
     */
    QTextDocument *notesDocument ();
    
    /**
     * Save the notes document to "notes.txt" if any document around.
     */
    void saveNotesDocument ();
    
    /**
     * Register a document for this project.
     * @param document document to register
     */
    void registerDocument (KTextEditor::Document *document);
    
    /**
     * Unregister a document for this project.
     * @param document document to unregister
     */
    void unregisterDocument (KTextEditor::Document *document);
    
  private Q_SLOTS:
    /**
     * Used for worker to send back the results of project loading
     * @param topLevel new toplevel element for model
     * @param file2Item new file => item mapping
     */
    void loadProjectDone (KateProjectSharedQStandardItem topLevel, KateProjectSharedQMapStringItem file2Item);

    /**
     * Used for worker to send back the results of index loading
     * @param projectIndex new project index
     */
    void loadIndexDone (KateProjectSharedProjectIndex projectIndex);

    void slotModifiedChanged(KTextEditor::Document*);
    
    
    void slotModifiedOnDisk (KTextEditor::Document *document,
      bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);

    
  Q_SIGNALS:
    /**
     * Emitted on project map changes.
     * This includes the name!
     */
    void projectMapChanged ();

    /**
     * Emitted on model changes.
     * This includes the files list, itemForFile mapping!
     */
    void modelChanged ();

    /**
     * Emitted when the index creation is finished.
     * This includes the ctags index.
     */
    void indexChanged ();

  private:
    /**
     * the worker inside the background thread
     * if this is NULL, we are in our deconstruction state and should
     * ignore the feedback of our already stopped thread that
     * may still come in because of queued connects
     * only DELETE all stuff we need to cleanup in the slots
     */
    QObject *m_worker;

    /**
     * our internal thread to load stuff and do things in background
     */
    KateProjectWorkerThread m_thread;

    /**
     * project file name
     */
    QString m_fileName;

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
    QStandardItemModel m_model;

    /**
     * mapping files => items
     */
    KateProjectSharedQMapStringItem m_file2Item;

    /**
     * project index, if any
     */
    KateProjectSharedProjectIndex m_projectIndex;
    
    /**
     * notes buffer for project local notes
     */
    QTextDocument *m_notesDocument;
    
    /**
     * Set of existing documents for this project.
     */
    QMap<KTextEditor::Document *, QString> m_documents;
    
    /**
     * Parent item for existing documents that are not in the project tree
     */
    QStandardItem *m_documentsParent;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
