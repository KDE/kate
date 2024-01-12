/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include "kateprivate_export.h"

#include <QString>
#include <QUrl>
#include <QWidgetList>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QAction;
class QFont;
class QIcon;
class QVariant;
class QWidget;
typedef QMap<QString, QVariant> QVariantMap;
QT_END_NAMESPACE

namespace KTextEditor
{
class View;
class Document;
class MainWindow;
class Range;
class Cursor;
}
class DiagnosticsProvider;
struct DiffParams;

enum MessageType {
    Log = 0,
    Info,
    Warning,
    Error,
};

namespace Utils
{

// A helper class that maintains scroll position
struct KATE_PRIVATE_EXPORT KateScrollBarRestorer {
    explicit KateScrollBarRestorer(KTextEditor::View *view);
    ~KateScrollBarRestorer();

    void restore();

private:
    class KateScrollBarRestorerPrivate *const d = nullptr;
};

/**
 * returns the current active global font
 */
KATE_PRIVATE_EXPORT QFont editorFont();

/**
 * Returns the range that is currently visible in the @p view
 */
KATE_PRIVATE_EXPORT KTextEditor::Range getVisibleRange(KTextEditor::View *view);

/**
 * @brief Given path "/home/user/xyz.txt" returns "xyz.txt"
 */
KATE_PRIVATE_EXPORT inline QString fileNameFromPath(const QString &path)
{
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    return lastSlash == -1 ? path : path.mid(lastSlash + 1);
}

/**
 * Return a matching icon for the given document.
 * We use the mime type icon for unmodified stuff and the modified one for modified docs.
 * @param doc document to get icon for
 * @return icon, always valid
 */
KATE_PRIVATE_EXPORT QIcon iconForDocument(KTextEditor::Document *doc);

KATE_PRIVATE_EXPORT QAction *toolviewShowAction(KTextEditor::MainWindow *, const QString &toolviewName);

KATE_PRIVATE_EXPORT QWidget *toolviewForName(KTextEditor::MainWindow *, const QString &toolviewName);

/*** BEGIN KTextEditor::MainWindow extensions **/
KATE_PRIVATE_EXPORT void
showMessage(const QString &message, const QIcon &icon, const QString &category, MessageType type, KTextEditor::MainWindow *mainWindow = nullptr);
KATE_PRIVATE_EXPORT void showMessage(const QVariantMap &map, KTextEditor::MainWindow *mainWindow = nullptr);

KATE_PRIVATE_EXPORT void showDiff(const QByteArray &diff, const DiffParams &params, KTextEditor::MainWindow *mainWindow);

KATE_PRIVATE_EXPORT void addWidget(QWidget *widget, KTextEditor::MainWindow *mainWindow);
KATE_PRIVATE_EXPORT void activateWidget(QWidget *widget, KTextEditor::MainWindow *mainWindow);
/// @returns list of document area widgets that are not KTextEditor::Views (added by addWidget()
/// TODO: Maybe it would be more versatile to return *all* widgets, including KTextEditor::Views, here, or perhaps
///       controlled by a flag to filter which widgets to return. Think e.g. about the document switching plugin.
///       activeWidget() (not yet "public") uses those semantics, already.
KATE_PRIVATE_EXPORT QWidgetList widgets(KTextEditor::MainWindow *mainWindow);

KATE_PRIVATE_EXPORT void insertWidgetInStatusbar(QWidget *widget, KTextEditor::MainWindow *mainWindow);

KATE_PRIVATE_EXPORT void addPositionToHistory(const QUrl &url, KTextEditor::Cursor c, KTextEditor::MainWindow *mainWindow);
/*** END KTextEditor::MainWindow extensions **/

/**
 * Returns project base dir for provided document
 */
KATE_PRIVATE_EXPORT QString projectBaseDirForDocument(KTextEditor::Document *doc);

/**
 * Returns project map for provided document
 */
KATE_PRIVATE_EXPORT QVariantMap projectMapForDocument(KTextEditor::Document *doc);

/**
 * Translates offset to line, col aka KTextEditor::Cursor
 */
KATE_PRIVATE_EXPORT KTextEditor::Cursor cursorFromOffset(KTextEditor::Document *doc, int offset);
}
