/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "kateproject.h"
#include "kateprojectworker.h"

#include <klocalizedstring.h>

#include <ktexteditor/document.h>

#include <ThreadWeaver/Queue>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextDocumentLayout>
#include <QJsonDocument>
#include <QJsonParseError>

KateProject::KateProject(ThreadWeaver::Queue *weaver)
    : QObject()
    , m_fileLastModified()
    , m_notesDocument(nullptr)
    , m_untrackedDocumentsRoot(nullptr)
    , m_weaver(weaver)
{
}

KateProject::~KateProject()
{
    saveNotesDocument();
}

bool KateProject::loadFromFile(const QString &fileName)
{
    /**
     * bail out if already fileName set!
     */
    if (!m_fileName.isEmpty()) {
        return false;
    }

    /**
     * set new filename and base directory
     */
    m_fileName = fileName;
    m_baseDir = QFileInfo(m_fileName).canonicalPath();

    /**
     * trigger reload
     */
    return reload();
}

bool KateProject::reload(bool force)
{
    QVariantMap map = readProjectFile();

    if (map.isEmpty()) {
        m_fileLastModified = QDateTime();
    } else {
        m_fileLastModified = QFileInfo(m_fileName).lastModified();
        m_globalProject = map;
    }

    return load(m_globalProject, force);
}

QVariantMap KateProject::readProjectFile() const
{
    QFile file(m_fileName);

    if (!file.open(QFile::ReadOnly)) {
        return QVariantMap();
    }

    /**
     * parse the whole file, bail out again on error!
     */
    const QByteArray jsonData = file.readAll();
    QJsonParseError parseError;
    QJsonDocument project(QJsonDocument::fromJson(jsonData, &parseError));

    if (parseError.error != QJsonParseError::NoError) {
        return QVariantMap();
    }

    return project.toVariant().toMap();
}

bool KateProject::loadFromData(const QVariantMap& globalProject, const QString& directory)
{
    m_baseDir = directory;
    m_fileName = QDir(directory).filePath(QLatin1String(".kateproject"));
    m_globalProject = globalProject;
    return load(globalProject);
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
     */
    if (!globalProject[QStringLiteral("directory")].toString().isEmpty()) {
        m_baseDir = QFileInfo(globalProject[QStringLiteral("directory")].toString()).canonicalFilePath();
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

    /**
     * emit that we changed stuff
     */
    emit projectMapChanged();


    KateProjectWorker * w = new KateProjectWorker(m_baseDir, m_projectMap);
    connect(w, &KateProjectWorker::loadDone, this, &KateProject::loadProjectDone);
    connect(w, &KateProjectWorker::loadIndexDone, this, &KateProject::loadIndexDone);
    m_weaver->stream() << w;

    return true;
}

void KateProject::loadProjectDone(KateProjectSharedQStandardItem topLevel, KateProjectSharedQMapStringItem file2Item)
{
    m_model.clear();
    m_model.invisibleRootItem()->appendColumn(topLevel->takeColumn(0));

    m_file2Item = file2Item;

    /**
     * readd the documents that are open atm
     */
    m_untrackedDocumentsRoot = nullptr;
    for (auto i = m_documents.constBegin(); i != m_documents.constEnd(); i++) {
        registerDocument(i.key());
    }

    emit modelChanged();
}

void KateProject::loadIndexDone(KateProjectSharedProjectIndex projectIndex)
{
    /**
     * move to our project
     */
    m_projectIndex = projectIndex;

    /**
     * notify external world that data is available
     */
    emit indexChanged();
}

QString KateProject::projectLocalFileName(const QString &suffix) const
{
    /**
     * nothing on empty file names for project
     * should not happen
     */
    if (m_baseDir.isEmpty() || suffix.isEmpty()) {
        return QString();
    }

    /**
     * compute full file name
     */
    return m_baseDir + QStringLiteral(".kateproject.") + suffix;
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
        inStream.setCodec("UTF-8");
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
        outStream.setCodec("UTF-8");
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

void KateProject::slotModifiedOnDisk(KTextEditor::Document *document,
                                     bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
    KateProjectItem *item = itemForFile(m_documents.value(document));

    if (!item) {
        return;
    }

    item->slotModifiedOnDisk(document, isModified, reason);
}

void KateProject::registerDocument(KTextEditor::Document *document)
{
    // remember the document, if not already there
    if (!m_documents.contains(document)) {
        m_documents[document] = document->url().toLocalFile();
    }

    // try to get item for the document
    KateProjectItem *item = itemForFile(document->url().toLocalFile());

    // if we got one, we are done, else create a dummy!
    if (item) {
        disconnect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
        disconnect(document, SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(slotModifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));
        item->slotModifiedChanged(document);

        /*FIXME    item->slotModifiedOnDisk(document,document->isModified(),qobject_cast<KTextEditor::ModificationInterface*>(document)->modifiedOnDisk()); FIXME*/

        connect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
        connect(document, SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(slotModifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

        return;
    }

    registerUntrackedDocument(document);
}

void KateProject::registerUntrackedDocument(KTextEditor::Document *document)
{
    // perhaps create the parent item
    if (!m_untrackedDocumentsRoot) {
        m_untrackedDocumentsRoot = new KateProjectItem(KateProjectItem::Directory, i18n("<untracked>"));
        m_model.insertRow(0, m_untrackedDocumentsRoot);
    }

    // create document item
    QFileInfo fileInfo(document->url().toLocalFile());
    KateProjectItem *fileItem = new KateProjectItem(KateProjectItem::File, fileInfo.fileName());
    fileItem->setData(document->url().toLocalFile(), Qt::ToolTipRole);
    fileItem->slotModifiedChanged(document);
    connect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);
    connect(document, SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(slotModifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

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

    fileItem->setData(document->url().toLocalFile(), Qt::UserRole);
    fileItem->setData(QVariant(true), Qt::UserRole + 3);

    if (!m_file2Item) {
        m_file2Item = KateProjectSharedQMapStringItem(new QMap<QString, KateProjectItem *> ());
    }
    (*m_file2Item)[document->url().toLocalFile()] = fileItem;
}

void KateProject::unregisterDocument(KTextEditor::Document *document)
{
    if (!m_documents.contains(document)) {
        return;
    }

    disconnect(document, &KTextEditor::Document::modifiedChanged, this, &KateProject::slotModifiedChanged);

    const QString &file = m_documents.value(document);

    if (m_untrackedDocumentsRoot) {
        KateProjectItem *item = static_cast<KateProjectItem *>(itemForFile(file));
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
