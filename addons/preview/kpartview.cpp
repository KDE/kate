/*
 *  Copyright (C) 2017 by Friedrich W. H. Kossebau <kossebau@kde.org>
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

#include "kpartview.h"

#include <ktexteditorpreview_debug.h>

// KF
#include <KTextEditor/Document>

#include <KLocalizedString>
#include <KPluginLoader>
#include <KPluginFactory>
#include <KParts/ReadOnlyPart>
#include <KParts/BrowserExtension>

// Qt
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QLabel>


using namespace KTextEditorPreview;

// 300 ms as initial proposal, was found to be delay which worked for the
// author with the use-case of quickly peeking over to the preview while
// typing to see if things are working out as intended, without getting a
// having-to-wait feeling.
// Surely could get some more serious research what is a proper (default) value.
// Perhaps the whole update logic cpuld also be overhauled, to only do an
// update once there was at least xxx ms idle time to meet the use-case of
// quickly-peeking-over. And otherwise update in bigger intervals of
// 500-2000(?) ms, to cover the use-case of seeing from the corner of one's
// eye that something is changing while one is editing the sources.
static const int updateDelay = 300; // ms

KPartView::KPartView(const KService::Ptr& service, QObject* parent)
    : QObject(parent)
{
    QString errorString;
    m_part = service->createInstance<KParts::ReadOnlyPart>(nullptr, this, QVariantList(), &errorString);

    if (!m_part) {
        m_errorLabel = new QLabel(errorString);
    } else if (!m_part->widget()) {
        // should not happen, but just be safe
        delete m_part;
        m_errorLabel = new QLabel(QStringLiteral("KPart provides no widget."));
    } else {
        m_updateSquashingTimer.setSingleShot(true);
        m_updateSquashingTimer.setInterval(updateDelay);
        connect(&m_updateSquashingTimer, &QTimer::timeout, this, &KPartView::updatePreview);

        auto browserExtension = m_part->browserExtension();
        if (browserExtension) {
            connect(browserExtension, &KParts::BrowserExtension::openUrlRequestDelayed,
                    this, &KPartView::handleOpenUrlRequest);
        }
        m_part->widget()->installEventFilter(this);
    }
}

KPartView::~KPartView()
{
    delete m_errorLabel;
}

QWidget* KPartView::widget() const
{
    return m_part ? m_part->widget() : m_errorLabel;
}

KParts::ReadOnlyPart* KPartView::kPart() const
{
    return m_part;
}

KTextEditor::Document* KPartView::document() const
{
    return m_document;
}

bool KPartView::isAutoUpdating() const
{
    return m_autoUpdating;
}

void KPartView::setDocument(KTextEditor::Document* document)
{
    if (m_document == document) {
        return;
    }
    if (!m_part) {
        return;
    }

    if (m_document) {
        disconnect(m_document, &KTextEditor::Document::textChanged, this, &KPartView::triggerUpdatePreview);
        m_updateSquashingTimer.stop();
    }

    m_document = document;

    // delete any temporary file, to trigger creation of a new if needed
    // for some unique url/path of the temporary file for the new document (or use a counter ourselves?)
    // but see comment for stream url
    delete m_bufferFile;
    m_bufferFile = nullptr;

    if (m_document) {
        m_previewDirty = true;
        updatePreview();
        connect(m_document, &KTextEditor::Document::textChanged, this, &KPartView::triggerUpdatePreview);
    } else {
        m_part->closeUrl();
    }
}

void KPartView::setAutoUpdating(bool autoUpdating)
{
    if (m_autoUpdating == autoUpdating) {
        return;
    }

    m_autoUpdating = autoUpdating;

    if (m_autoUpdating) {
        if (m_document && m_part && m_previewDirty) {
            updatePreview();
        }
    } else {
        m_updateSquashingTimer.stop();
    }
}

void KPartView::triggerUpdatePreview()
{
    m_previewDirty = true;

    if (m_part->widget()->isVisible() && m_autoUpdating && !m_updateSquashingTimer.isActive()) {
        m_updateSquashingTimer.start();
    }
}

void KPartView::updatePreview()
{
    if (!m_part->widget()->isVisible()) {
        return;
    }

    // TODO: some kparts seem to steal the focus after they have loaded a file, sometimes also async
    // that possibly needs fixing in the respective kparts, as that could be considered non-cooperative

    // TODO: investigate if pushing of the data to the kpart could be done in a non-gui-thread,
    // so their loading of the file (e.g. ReadOnlyPart::openFile() is sync design) does not block

    const auto mimeType = m_document->mimeType();
    KParts::OpenUrlArguments arguments;
    arguments.setMimeType(mimeType);
    m_part->setArguments(arguments);

    // try to stream the data to avoid filesystem I/O
    // create url unique for this document
    // TODO: encode existing url instead, and for yet-to-be-stored docs some other unique id
    const QUrl streamUrl(QStringLiteral("ktexteditorpreview:/object/%1")
                         .arg(reinterpret_cast<quintptr>(m_document), 0, 16));
    if (m_part->openStream(mimeType, streamUrl)) {
        qCDebug(KTEPREVIEW) << "Pushing data via streaming API, url:" << streamUrl.url();
        m_part->writeStream(m_document->text().toUtf8());
        m_part->closeStream();

        m_previewDirty = false;
        return;
    }

    // have to go via filesystem for now, not nice
    if (!m_bufferFile) {
        m_bufferFile = new QTemporaryFile(this);
        m_bufferFile->open();
    } else {
        // reset position
        m_bufferFile->seek(0);
    }
    const QUrl tempFileUrl(QUrl::fromLocalFile(m_bufferFile->fileName()));
    qCDebug(KTEPREVIEW) << "Pushing data via temporary file, url:" << tempFileUrl.url();

    // write current data
    m_bufferFile->write(m_document->text().toUtf8());
    // truncate at end of new content
    m_bufferFile->resize(m_bufferFile->pos());
    m_bufferFile->flush();

    // TODO: find out why we need to send this queued
    QMetaObject::invokeMethod(m_part, "openUrl", Qt::QueuedConnection, Q_ARG(QUrl, tempFileUrl));

    m_previewDirty = false;
}

void KPartView::handleOpenUrlRequest(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

bool KPartView::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_part->widget() && event->type() == QEvent::Show) {
        if (m_document && m_autoUpdating && m_previewDirty) {
            updatePreview();
        }
        return true;
    }

    return QObject::eventFilter(object, event);
}
