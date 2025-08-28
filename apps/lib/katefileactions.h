/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2018 Gregor Mi <codestruct@posteo.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 **/

#pragma once

#include "kateprivate_export.h"

#include <utility>

#include <QString>

class QAction;
class QMenu;
class QWidget;
class QUrl;
namespace KTextEditor
{
class Document;
}

namespace KateFileActions
{
/**
 * Copies the file path to clipboard.
 * If the document has no file, the clipboard will be emptied.
 */
KATE_PRIVATE_EXPORT void copyFilePathToClipboard(KTextEditor::Document *document);

/**
 * Copies the file name to clipboard.
 * If the document has no file, the clipboard will be emptied.
 */
KATE_PRIVATE_EXPORT void copyFileNameToClipboard(KTextEditor::Document *document);

/**
 * Tries to open and highlight the underlying url in the filemanager
 */
KATE_PRIVATE_EXPORT void openContainingFolder(KTextEditor::Document *document);

/**
 * Shows a Rename dialog to rename the file associated with the document.
 * The document will be closed an reopened.
 *
 * Nothing is done if the document is nullptr or has no associated file.
 */
KATE_PRIVATE_EXPORT void renameDocumentFile(QWidget *parent, KTextEditor::Document *document);

KATE_PRIVATE_EXPORT void openFilePropertiesDialog(QWidget *parent, KTextEditor::Document *document);

/**
 * Asks the user if the file should really be deleted. If yes, the file
 * is deleted from disk and the document closed.
 *
 * Nothing is done if the document is nullptr or has no associated file.
 */
KATE_PRIVATE_EXPORT void deleteDocumentFile(QWidget *parent, KTextEditor::Document *document);

/**
 * @returns a list of supported diff tools (names of the executables + paths to them, empty if not found in PATH)
 */
QList<std::pair<QString, QString>> supportedDiffTools();

/**
 * Runs an external program to compare the underlying files of two given documents.
 *
 * @p diffExecutable tested to work with "kdiff3", "kompare", and "meld"
 * @see supportedDiffTools()
 *
 * The parameters documentA and documentB must not be nullptr. Otherwise an assertion fails.
 *
 * If @p documentA or @p documentB have an empty url,
 * then an empty string is passed to the diff program instead of a local file path.
 *
 * @returns true if program was started successfully; otherwise false
 * (which can mean the program is not installed)
 *
 * IDEA for later: compare with unsaved buffer data instead of underlying file
 */
void compareWithExternalProgram(KTextEditor::Document *documentA, KTextEditor::Document *documentB, const QString &diffExecutable);

/**
 * Prepares the open with menu
 */
KATE_PRIVATE_EXPORT void prepareOpenWithMenu(const QUrl &url, QMenu *menu);

/**
 * Prepares the open with menu
 */
KATE_PRIVATE_EXPORT void showOpenWithMenu(QWidget *parent, const QUrl &url, QAction *action);
}
