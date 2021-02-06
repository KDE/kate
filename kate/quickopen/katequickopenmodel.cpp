/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katequickopenmodel.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QFileInfo>
#include <QMimeDatabase>

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
    return 1;
}

QVariant KateQuickOpenModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid()) {
        return {};
    }

    const ModelEntry &entry = m_modelEntries.at(idx.row());
    switch (role) {
    case Role::FileName:
        return entry.fileName;
    case Role::FilePath:
        return entry.filePath;
    case Qt::FontRole: {
        if (entry.bold) {
            QFont font;
            font.setBold(true);
            return font;
        }
        return {};
    }
    case Qt::DecorationRole:
        return QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(entry.fileName, QMimeDatabase::MatchExtension).iconName());
    case Qt::UserRole:
        return entry.url;
    case Role::Score:
        return entry.score;
    default:
        return {};
    }

    return {};
}

void KateQuickOpenModel::refresh()
{
    QObject *projectView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    const QList<KTextEditor::View *> sortedViews = m_mainWindow->viewManager()->sortedViews();
    const QList<KTextEditor::Document *> openDocs = KateApp::self()->documentManager()->documentList();
    const QStringList projectDocs = projectView
        ? (m_listMode == CurrentProject ? projectView->property("projectFiles") : projectView->property("allProjectsFiles")).toStringList()
        : QStringList();
    const QString projectBase = [this, projectView]() -> QString {
        if (!projectView) {
            return QString();
        }
        QString ret;
        if (m_listMode == CurrentProject) {
            ret = projectView->property("projectBaseDir").toString();
        } else {
            ret = projectView->property("allProjectsCommonBaseDir").toString();
        }
        if (!ret.endsWith(QLatin1Char('/'))) {
            ret.append(QLatin1Char('/'));
        }
        return ret;
    }();

    QVector<ModelEntry> allDocuments;
    allDocuments.reserve(sortedViews.size() + projectDocs.size());

    QSet<QUrl> openedDocUrls;
    openedDocUrls.reserve(sortedViews.size());

    for (auto *view : qAsConst(sortedViews)) {
        auto doc = view->document();
        const auto url = doc->url();
        if (openedDocUrls.contains(url)) {
            continue;
        }
        openedDocUrls.insert(url);
        allDocuments.push_back(
            {url, doc->documentName(), url.toDisplayString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile).remove(projectBase), true, -1});
    }

    for (auto *doc : qAsConst(openDocs)) {
        auto url = doc->url();
        if (openedDocUrls.contains(url)) {
            continue;
        }
        openedDocUrls.insert(url);
        const auto normalizedUrl = url.toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile).remove(projectBase);
        allDocuments.push_back({doc->url(), doc->documentName(), normalizedUrl, true, -1});
    }

    for (const auto &file : qAsConst(projectDocs)) {
        QFileInfo fi(file);
        const auto localFile = QUrl::fromLocalFile(fi.absoluteFilePath());
        if (openedDocUrls.contains(localFile)) {
            continue;
        }
        allDocuments.push_back({localFile, fi.fileName(), fi.filePath().remove(projectBase), false, -1});
    }

    beginResetModel();
    m_modelEntries = std::move(allDocuments);
    endResetModel();
}
