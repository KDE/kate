/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QFontDatabase>

namespace Utils
{

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

}
