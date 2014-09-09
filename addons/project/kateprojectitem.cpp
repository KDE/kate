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

#include "kateprojectitem.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QThread>
#include <QCoreApplication>
#include <QMimeDatabase>

#include <KIconLoader>
#include <KTextEditor/Document>

KateProjectItem::KateProjectItem(Type type, const QString &text)
    : QStandardItem(text)
    , m_type(type)
    , m_icon(0)
    , m_emblem(0)
{
}

KateProjectItem::~KateProjectItem()
{
    /**
     * cleanup
     */
    delete m_icon;
    delete m_emblem;
}

void KateProjectItem::slotModifiedChanged(KTextEditor::Document *doc)
{
    if (m_icon) {
        delete m_icon;
        m_icon = 0;
    }

    if (doc->isModified()) {
        if (m_emblem) {
            QStringList emblems;
            emblems << *m_emblem;
            m_icon = new QIcon(KIconLoader::global()->loadIcon(QStringLiteral("document-save") , KIconLoader::Small, 0, KIconLoader::DefaultState, emblems));
        } else {
            m_icon = new QIcon(KIconLoader::global()->loadIcon(QStringLiteral("document-save"), KIconLoader::Small));
        }
    }
    emitDataChanged();
}

void KateProjectItem::slotModifiedOnDisk(KTextEditor::Document *document,
        bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
    Q_UNUSED(document)
    Q_UNUSED(isModified)

    if (m_icon) {
        delete m_icon;
        m_icon = 0;
    }

    if (m_emblem) {
        delete m_emblem;
        m_emblem = 0;
    }

    if (reason != KTextEditor::ModificationInterface::OnDiskUnmodified) {
        m_emblem = new QString(QStringLiteral("emblem-important"));
    }
    emitDataChanged();
}

QVariant KateProjectItem::data(int role) const
{
    /**
     * create icons on demand
     */
    if (role == Qt::DecorationRole) {
        /**
         * this should only happen in main thread
         * the background thread should only construct this elements and fill data
         * but never query gui stuff!
         */
        Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

        /**
         * create icon, on demand
         */
        if (!m_icon) {
            /**
             * use right type
             */
            switch (m_type) {
            case Project:
                m_icon = new QIcon(KIconLoader::global()->loadIcon(QStringLiteral("folder-documents"), KIconLoader::Small));
                break;

            case Directory:
                m_icon = new QIcon(KIconLoader::global()->loadIcon(QStringLiteral("folder"), KIconLoader::Small));
                break;

            case File: {
                QString iconName = QMimeDatabase().mimeTypeForUrl(QUrl::fromLocalFile(data(Qt::UserRole).toString())).iconName();
                QStringList emblems;
                if (m_emblem) {
                    emblems << *m_emblem;
                }
                m_icon = new QIcon(KIconLoader::global()->loadMimeTypeIcon(iconName, KIconLoader::Small, 0, KIconLoader::DefaultState, emblems));
                break;
            }
            }
        }

        /**
         * return the cached icon
         */
        return QVariant(*m_icon);
    }

    /**
     * use normal data method
     */
    return QStandardItem::data(role);
}

