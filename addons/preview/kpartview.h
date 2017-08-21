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

#ifndef KTEXTEDITORPREVIEW_KPARTVIEW_H
#define KTEXTEDITORPREVIEW_KPARTVIEW_H

// KF
#include <KService>

// Qt
#include <QObject>
#include <QTimer>

namespace KTextEditor {
class Document;
}
namespace KParts {
class ReadOnlyPart;
}
class QLabel;
class QTemporaryFile;

namespace KTextEditorPreview {

/**
 * Wrapper around a KPart which handles feeding it the content of a text document
 *
 * The class creates a KPart from the service description passed in the constructor
 * and then takes care for feeding the content of the currently set text document
 * to the KPart for preview in the target format, both on changes in the document
 * or when a new document is set.
 *
 * The content is pushed via the KParts stream API, if the KPart instance
 * supports it, or as fallback via the filesystem, using a QTemporaryFile instance.
 * Updates are squashed via a timer, to reduce load.
 */
class KPartView : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param service the description of the KPart which should be used, may not be a nullptr
     * @param parent the object taking ownership, can be a nullptr
     */
    KPartView(const KService::Ptr& service, QObject* parent);
    ~KPartView() override;

    /**
     * Returns the widget object, ownership is not transferred.
     */
    QWidget* widget() const;

    KParts::ReadOnlyPart* kPart() const;

    /**
     * Sets the current document whose content should be previewed by the KPart.
     *
     * @param document the document or, if there is none to preview, a nullptr
     */
    void setDocument(KTextEditor::Document* document);

    /**
     * Returns the current document whose content is previewed by the KPart.
     *
     * @return current document or, if there is none, a nullptr
     */
    KTextEditor::Document* document() const;

    /**
     * Sets whether the preview should be updating automatically on document changes or not.
     *
     * @param autoUpdating whether the preview should be updating automatically on document changes or not
     */
    void setAutoUpdating(bool autoUpdating);

    /**
     * Returns @c true if the preview is updating automatically on document changes, @c false otherwise.
     */
    bool isAutoUpdating() const;

    /**
     * Update preview to current document content.
     */
    void updatePreview();

protected:
      bool eventFilter(QObject* object, QEvent* event) override;

private:
    void triggerUpdatePreview();
    void handleOpenUrlRequest(const QUrl& url);

private:
    QLabel* m_errorLabel = nullptr;
    KParts::ReadOnlyPart* m_part = nullptr;
    KTextEditor::Document* m_document = nullptr;

    bool m_autoUpdating = true;
    bool m_previewDirty = true;
    QTimer m_updateSquashingTimer;
    QTemporaryFile* m_bufferFile = nullptr;
};

}

#endif
