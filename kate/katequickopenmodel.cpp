/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
    ---
    Copyright (C) 2018 Tomaz Canabrava <tcanabrava@kde.org>
*/

#include "katequickopenmodel.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

KateQuickOpenModel::KateQuickOpenModel(KateMainWindow *mainWindow, QObject *parent)
    : QAbstractTableModel(parent)
    , m_mainWindow(mainWindow)
{
}

int KateQuickOpenModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_modelEntries.size();
}

int KateQuickOpenModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant KateQuickOpenModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid()) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::FontRole && role != Qt::UserRole) {
        return {};
    }

    auto entry = m_modelEntries.at(idx.row());
    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
        case Columns::FileName:
            return entry.fileName;
        case Columns::FilePath:
            return entry.filePath;
        }
    } else if (role == Qt::FontRole) {
        if (entry.bold) {
            QFont font;
            font.setBold(true);
            return font;
        }
    } else if (role == Qt::UserRole) {
        return entry.url;
    }

    return {};
}

void KateQuickOpenModel::refresh()
{
    QObject *projectView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    const QList<KTextEditor::View *> sortedViews = m_mainWindow->viewManager()->sortedViews();
    const QList<KTextEditor::Document *> openDocs = KateApp::self()->documentManager()->documentList();
    const QStringList projectDocs = projectView ? (m_listMode == CurrentProject ? projectView->property("projectFiles") : projectView->property("allProjectsFiles")).toStringList() : QStringList();

    QVector<ModelEntry> allDocuments;
    allDocuments.reserve(sortedViews.size() + openDocs.size() + projectDocs.size());

    size_t sort_id = (size_t)-1;
    for (auto *view : qAsConst(sortedViews)) {
        auto doc = view->document();
        allDocuments.push_back({doc->url(), doc->documentName(), doc->url().toDisplayString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile), true, sort_id--});
    }

    for (auto *doc : qAsConst(openDocs)) {
        const auto normalizedUrl = doc->url().toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile);
        allDocuments.push_back({doc->url(), doc->documentName(), normalizedUrl, true, 0});
    }

    for (const auto &file : qAsConst(projectDocs)) {
        QFileInfo fi(file);
        const auto localFile = QUrl::fromLocalFile(fi.absoluteFilePath());
        allDocuments.push_back({localFile, fi.fileName(), localFile.toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile), false, 0});
    }

    /** Sort the arrays by filePath. */
    std::stable_sort(std::begin(allDocuments), std::end(allDocuments), [](const ModelEntry &a, const ModelEntry &b) { return a.filePath < b.filePath; });

    /** remove Duplicates.
     * Note that the stable_sort above guarantees that the items that the
     * bold/sort_id fields of the items added first are correctly preserved.
     */
    allDocuments.erase(std::unique(allDocuments.begin(), allDocuments.end(), [](const ModelEntry &a, const ModelEntry &b) { return a.filePath == b.filePath; }), std::end(allDocuments));

    /** sort the arrays via boldness (open or not */
    std::stable_sort(std::begin(allDocuments), std::end(allDocuments), [](const ModelEntry &a, const ModelEntry &b) {
        if (a.bold == b.bold)
            return a.sort_id > b.sort_id;
        return a.bold > b.bold;
    });

    beginResetModel();
    m_modelEntries = allDocuments;
    endResetModel();
}
