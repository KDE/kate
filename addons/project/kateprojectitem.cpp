/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectitem.h"
#include "kateproject.h"

#include <QCoreApplication>
#include <QIcon>
#include <QMimeDatabase>
#include <QThread>

#include <KIconUtils>

KateProjectItem::KateProjectItem(Type type, const QString &text, const QString &path)
    : QStandardItem(text)
    , m_type(type)
    , m_path(path)
{
}

void KateProjectItem::slotModifiedChanged(KTextEditor::Document *doc)
{
    m_icon.reset();

    if (doc && doc->isModified()) {
        if (m_emblem.isEmpty()) {
            m_icon.reset(new QIcon(QIcon::fromTheme(QStringLiteral("document-save"))));
        } else {
            m_icon.reset(new QIcon(KIconUtils::addOverlay(QIcon::fromTheme(QStringLiteral("document-save")), QIcon(m_emblem), Qt::TopLeftCorner)));
        }
    }

    emitDataChanged();
}

void KateProjectItem::slotModifiedOnDisk(KTextEditor::Document *document, bool isModified, KTextEditor::Document::ModifiedOnDiskReason reason)
{
    Q_UNUSED(document)
    Q_UNUSED(isModified)

    m_icon.reset();
    m_emblem.clear();

    if (reason != KTextEditor::Document::OnDiskUnmodified) {
        m_emblem = QStringLiteral("emblem-important");
    }

    emitDataChanged();
}

QVariant KateProjectItem::data(int role) const
{
    if (role == Qt::UserRole) {
        return m_path;
    }

    if (role == Qt::DecorationRole) {
        /**
         * this should only happen in main thread
         * the background thread should only construct this elements and fill data
         * but never query gui stuff!
         */
        Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
        return QVariant(*icon());
    }

    if (role == TypeRole) {
        return QVariant(m_type);
    }

    return QStandardItem::data(role);
}

bool KateProjectItem::operator<(const QStandardItem &other) const
{
    // let directories stay first
    const auto thisType = data(TypeRole).toInt();
    const auto otherType = other.data(TypeRole).toInt();
    if (thisType != otherType) {
        return thisType < otherType;
    }

    // case-insensitive compare of the filename
    return data(Qt::DisplayRole).toString().compare(other.data(Qt::DisplayRole).toString(), Qt::CaseInsensitive) < 0;
}

QIcon *KateProjectItem::icon() const
{
    if (m_icon) {
        return m_icon.get();
    }

    switch (m_type) {
    case LinkedProject:
    case Project:
        m_icon.reset(new QIcon(QIcon::fromTheme(QStringLiteral("folder-documents"))));
        break;

    case Directory:
        m_icon.reset(new QIcon(QIcon::fromTheme(QStringLiteral("folder"))));
        break;

    case File: {
        // ensure we have no empty icons, that breaks layout in tree views
        QIcon icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(m_path, QMimeDatabase::MatchExtension).iconName());
        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("unknown"));
        }
        if (!m_emblem.isEmpty()) {
            m_icon.reset(new QIcon(KIconUtils::addOverlay(icon, QIcon(m_emblem), Qt::TopLeftCorner)));
        } else {
            m_icon.reset(new QIcon(icon));
        }
        break;
    }
    }

    return m_icon.get();
}

void KateProjectItem::setData(const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        // we can only rename files
        if (m_type != File) {
            return;
        }

        /**
         *  retrieve the ref to project that we stored
         *  in KateProjectTreeViewContextMenu
         */
        auto *project = data(KateProjectItem::ProjectRole).value<KateProject *>();
        if (!project) {
            return;
        }

        const QString newFileName = value.toString();
        if (newFileName.isEmpty() || newFileName == text()) {
            return;
        }

        // Rename later: it closes and reopens the document which can delete this item (e.g. untracked items).
        // The project updates our path and text once the rename succeeded.
        QMetaObject::invokeMethod(
            project,
            [project, oldPath = m_path, newFileName] {
                project->renameFileOnDisk(oldPath, newFileName);
            },
            Qt::QueuedConnection);
        return;
    } else if (role == Qt::UserRole) {
        m_path = value.toString();
    }

    QStandardItem::setData(value, role);
}
