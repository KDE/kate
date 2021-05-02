/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katequickopenmodel.h"

#include "kateapp.h"
#include "katemainwindow.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QFileInfo>
#include <QIcon>
#include <QMimeDatabase>

KateQuickOpenModel::KateQuickOpenModel(QObject *parent)
    : QAbstractTableModel(parent)
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
    case Qt::DisplayRole:
    case Role::FileName:
        return entry.fileName;
    case Role::FilePath:
        return QString(entry.filePath).remove(m_projectBase);
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
        return entry.url.isEmpty() ? QUrl::fromLocalFile(entry.filePath) : entry.url;
    case Role::Score:
        return entry.score;
    default:
        return {};
    }

    return {};
}

void KateQuickOpenModel::refresh(KateMainWindow *mainWindow)
{
    QObject *projectView = mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    const QList<KTextEditor::View *> sortedViews = mainWindow->viewManager()->sortedViews();
    const QList<KTextEditor::Document *> openDocs = KateApp::self()->documentManager()->documentList();
    const QStringList projectDocs = projectView
        ? (m_listMode == CurrentProject ? projectView->property("projectFiles") : projectView->property("allProjectsFiles")).toStringList()
        : QStringList();
    const QString projectBase = [projectView]() -> QString {
        if (!projectView) {
            return QString();
        }
        QString ret;
        // open files are always included in the listing, even if list mode == CurrentProject
        // those open files may belong to another project than the current one
        // so we should always consistently strip the common base
        // otherwise it will be confusing and the opened files of anther project
        // end up with an unstripped complete file path
        ret = projectView->property("allProjectsCommonBaseDir").toString();
        if (!ret.endsWith(QLatin1Char('/'))) {
            ret.append(QLatin1Char('/'));
        }
        return ret;
    }();

    m_projectBase = projectBase;

    QVector<ModelEntry> allDocuments;
    allDocuments.reserve(sortedViews.size() + projectDocs.size());

    QSet<QString> openedDocUrls;
    openedDocUrls.reserve(sortedViews.size());

    for (auto *view : qAsConst(sortedViews)) {
        auto doc = view->document();
        auto path = doc->url().toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile);
        openedDocUrls.insert(path);
        allDocuments.push_back({doc->url(), doc->documentName(), path, true, -1});
    }

    for (auto *doc : qAsConst(openDocs)) {
        auto path = doc->url().toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile);
        if (openedDocUrls.contains(path)) {
            continue;
        }
        openedDocUrls.insert(path);
        allDocuments.push_back({doc->url(), doc->documentName(), path, true, -1});
    }

    for (const auto &file : qAsConst(projectDocs)) {
        QFileInfo fi(file);
        // projectDocs items have full path already, reuse that
        if (openedDocUrls.contains(file)) {
            continue;
        }
        allDocuments.push_back({QUrl(), fi.fileName(), file, false, -1});
    }

    beginResetModel();
    m_modelEntries = std::move(allDocuments);
    endResetModel();
}
