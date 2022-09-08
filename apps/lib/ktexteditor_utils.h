/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "kateprivate_export.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QAction;
class QFont;
class QIcon;
QT_END_NAMESPACE

namespace KTextEditor
{
class View;
class Document;
class MainWindow;
}

namespace Utils
{

// A helper class that maintains scroll position
struct KATE_PRIVATE_EXPORT KateScrollBarRestorer {
    KateScrollBarRestorer(KTextEditor::View *view);
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
 * returns the font for view @p view
 */
KATE_PRIVATE_EXPORT QFont viewFont(KTextEditor::View *view);

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
}
