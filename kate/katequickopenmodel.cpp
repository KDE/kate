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

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kateapp.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

KateQuickOpenModel::KateQuickOpenModel(KateMainWindow *mainWindow, QObject *parent) :
    QAbstractTableModel (parent), m_mainWindow(mainWindow)
{
}

int KateQuickOpenModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_modelEntries.size();
}

int KateQuickOpenModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant KateQuickOpenModel::data(const QModelIndex& idx, int role) const
{
    if(! idx.isValid()) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::FontRole) {
        return {};
    }

    auto entry = m_modelEntries.at(idx.row());
    if (role == Qt::DisplayRole) {
        switch(idx.column()) {
            case Columns::FileName: return entry.fileName;
            case Columns::FilePath: return entry.filePath;
        }
    } else if (role == Qt::FontRole) {
        if (entry.bold) {
            QFont font;
            font.setBold(true);
            return font;
        }
    }

    return {};
}

void KateQuickOpenModel::refresh()
{
    QObject *projectView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    const QList<KTextEditor::View *> sortedViews = m_mainWindow->viewManager()->sortedViews();
    const QList<KTextEditor::Document *> openDocs = KateApp::self()->documentManager()->documentList();
    const QStringList projectDocs = projectView ? projectView->property("projectFiles").toStringList() : QStringList();

    QVector<ModelEntry> allDocuments;
    allDocuments.resize(sortedViews.size() + openDocs.size() + projectDocs.size());

    for (auto *view : qAsConst(sortedViews)) {
        auto doc = view->document();
        allDocuments.push_back({ doc->documentName(), doc->url().toString(QUrl::NormalizePathSegments), false });
    }

    QStringList openedUrls;
    openedUrls.reserve(openDocs.size());
    for (auto *doc : qAsConst(openDocs)) {
        const auto normalizedUrl = doc->url().toString(QUrl::NormalizePathSegments);
        allDocuments.push_back({ doc->documentName(), normalizedUrl, false });
        openedUrls.push_back(normalizedUrl);
    }

    for (const auto& file : qAsConst(projectDocs)) {
        QFileInfo fi(file);
        allDocuments.push_back({ fi.fileName(), QUrl::fromLocalFile(file).toString(QUrl::NormalizePathSegments), false });
    }

    /** Sort the arrays via Url. */
    std::sort(std::begin(allDocuments), std::end(allDocuments),
        [](const ModelEntry& a, const ModelEntry& b) {
        return a.filePath < b.filePath;
    });

    /** remove Duplicates. */
    allDocuments.erase(
        std::unique(allDocuments.begin(), allDocuments.end(),
        [](const ModelEntry& a, const ModelEntry& b) {
            return a.filePath == b.filePath;
        }),
        std::end(allDocuments));

    for(auto& doc : allDocuments) {
        if (Q_UNLIKELY(openedUrls.indexOf(doc.filePath) != -1)) {
            doc.bold = true;
        }
    }

    /** sort the arrays via boldness (open or not */
    std::sort(std::begin(allDocuments), std::end(allDocuments),
        [](const ModelEntry& a, const ModelEntry& b) {
        return a.bold > b.bold;
    });

    beginResetModel();
    m_modelEntries = allDocuments;
    endResetModel();
}
