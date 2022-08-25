/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "kateprivate_export.h"

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QFontDatabase>
#include <QIcon>
#include <QPointer>
#include <QScrollBar>

namespace Utils
{

// A helper class that maintains scroll position
struct KateScrollBarRestorer {
    KateScrollBarRestorer(KTextEditor::View *view)
    {
        // Find KateScrollBar
        const auto scrollBars = view->findChildren<QScrollBar *>();
        kateScrollBar = [scrollBars] {
            for (auto scrollBar : scrollBars) {
                if (qstrcmp(scrollBar->metaObject()->className(), "KateScrollBar") == 0) {
                    return scrollBar;
                }
            }
            return static_cast<QScrollBar *>(nullptr);
        }();

        if (kateScrollBar) {
            oldScrollValue = kateScrollBar->value();
        }
    }

    void restore()
    {
        if (kateScrollBar) {
            kateScrollBar->setValue(oldScrollValue);
        }
        restored = true;
    }

    ~KateScrollBarRestorer()
    {
        if (!restored) {
            restore();
        }
    }

private:
    QPointer<QScrollBar> kateScrollBar = nullptr;
    int oldScrollValue = 0;
    bool restored = false;
};

/**
 * returns the current active global font
 */
inline QFont editorFont()
{
    if (KTextEditor::Editor::instance()) {
        return KTextEditor::Editor::instance()->font();
    }
    qWarning() << __func__ << "Editor::instance() is null! falling back to system fixed font";
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}

/**
 * returns the font for view @p view
 */
inline QFont viewFont(KTextEditor::View *view)
{
    if (const auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(view)) {
        return ciface->configValue(QStringLiteral("font")).value<QFont>();
    }
    return editorFont();
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
