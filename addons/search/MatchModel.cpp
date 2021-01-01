/***************************************************************************
 *   This file is part of Kate build plugin
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 ***************************************************************************/

#include "MatchModel.h"
#include <KLocalizedString>
#include <QDebug>
#include <QTimer>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <algorithm>    // std::count_if


static const quintptr InvalidIndex = 0xFFFFFFFF;

static QUrl localFileDirUp(const QUrl &url)
{
    if (!url.isLocalFile())
        return url;

    // else go up
    return QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).dir().absolutePath());
}

MatchModel::MatchModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}
MatchModel::~MatchModel()
{
}

void MatchModel::clear()
{
    beginResetModel();
    m_matchFiles.clear();
    m_matchFileIndexHash.clear();
    endResetModel();
}

/** This function returns the row index of the specified file.
 * If the file does not exist in the model, the file will be added to the model. */
int MatchModel::matchFileRow(const QUrl& fileUrl)
{
    return m_matchFileIndexHash.value(fileUrl, -1);
}

static const int totalContectLen = 150;

/** This function is used to add a match to a new file */
void MatchModel::addMatch(const QUrl &fileUrl, const QString &lineContent, int matchLen, int line, int column, int endLine, int endColumn)
{
    int fileIndex = matchFileRow(fileUrl);
    if (fileIndex == -1) {
        fileIndex = m_matchFiles.size();
        m_matchFileIndexHash.insert(fileUrl, fileIndex);
        beginInsertRows(QModelIndex(), fileIndex, fileIndex);
        // We are always starting the insert at the end, so we could optimize by delaying/grouping the signaling of the updates
        m_matchFiles.append(MatchFile());
        m_matchFiles[fileIndex].fileUrl = fileUrl;
        endInsertRows();
    }
    MatchModel::Match match;
    match.matchLen = matchLen;
    match.startLine = line;
    match.startColumn = column;
    match.endLine = endLine;
    match.endColumn = endColumn;

    int contextLen = totalContectLen - matchLen;
    int preLen = qMin(contextLen/3, column);
    int postLen = contextLen - preLen;

    match.preMatchStr = lineContent.mid(column-preLen, preLen);
    if (column > preLen) match.preMatchStr.prepend(QLatin1String("..."));

    match.postMatchStr = lineContent.mid(endColumn, postLen);
    if (endColumn+preLen < lineContent.size()) match.postMatchStr.append(QLatin1String("..."));

    match.matchStr = lineContent.mid(column, matchLen);

    int matchIndex = m_matchFiles[fileIndex].matches.size();
    beginInsertRows(index(fileIndex, 0), matchIndex, matchIndex);
    // We are always starting the insert at the end, so we could optimize by delaying/grouping the signaling of the updates
    m_matchFiles[fileIndex].matches.append(match);
    endInsertRows();
}

void MatchModel::setMatchColors(const QColor &foreground, const QColor &background, const QColor &replaseBackground)
{
    m_foregroundColor = foreground;
    m_searchBackgroundColor = background;
    m_replaceHighlightColor = replaseBackground;
}


// /** This function is used to modify a match */
// void MatchModel::replaceMatch(const QModelIndex &matchIndex, const QRegularExpression &regexp, const QString &replaceText)
// {
//     if (!matchIndex.isValid()) return;
// }
//
// /** Replace all matches that have been checked */
// void MatchModel::replaceChecked(const QRegularExpression &regexp, const QString &replace)
// {
// }

QString MatchModel::matchToHtmlString(const Match &match) const
{
    QString pre =match.preMatchStr.toHtmlEscaped();
    QString matchStr = match.matchStr;
    matchStr.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    matchStr = matchStr.toHtmlEscaped();
    matchStr = QStringLiteral("<span style=\"background-color:%1; color:%2;\">%3</span>")
    .arg(m_searchBackgroundColor.name(), m_foregroundColor.name(), matchStr);
    QString post = match.postMatchStr.toHtmlEscaped();

    // (line:col)[space][space] ...Line text pre [highlighted match] Line text post....
    QString displayText = QStringLiteral("(<b>%1:%2</b>) &nbsp;").arg(match.startLine + 1).arg(match.startColumn + 1) + pre + matchStr + post;

    return displayText;
}

QString MatchModel::fileItemToHtmlString(const MatchFile &matchFile) const
{
    QString path = matchFile.fileUrl.isLocalFile() ? localFileDirUp(matchFile.fileUrl).path() : matchFile.fileUrl.url();
    if (!path.isEmpty() && !path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }

    QString tmpStr = QStringLiteral("%1<b>%2: %3</b>").arg(path, matchFile.fileUrl.fileName()).arg(matchFile.matches.size());

    return tmpStr;
}


QVariant MatchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() < 0 || index.column() > 1) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::CheckStateRole) {
        return QVariant();
    }

    // FIXME add one more level
    int fileRow = index.internalId() == InvalidIndex ? index.row() : (int)index.internalId();
    int matchRow = index.internalId() == InvalidIndex ? -1 : index.row();

    if (fileRow < 0 || fileRow >= m_matchFiles.size()) {
        qDebug() << "Should be a file (or the info item in the near future)" << fileRow;
        return QVariant();
    }

    if (matchRow < 0) {
        // File item
        switch (role) {
            case Qt::DisplayRole:
                return fileItemToHtmlString(m_matchFiles[fileRow]);
            case Qt::CheckStateRole:
                return m_matchFiles[fileRow].checkState;
        }
    }
    else if (matchRow < m_matchFiles[fileRow].matches.size()) {
        // Match
        const Match &match = m_matchFiles[fileRow].matches[matchRow];
        switch (role) {
            case Qt::DisplayRole:
                return matchToHtmlString(match);
            case Qt::CheckStateRole:
                return match.checked ? Qt::Checked : Qt::Unchecked;
        }
    }
    else {
        qDebug() << "bad index";
        return QVariant();
    }


    return QVariant();
}

static bool isChecked(const MatchModel::Match &match) { return match.checked; }


bool MatchModel::setData(const QModelIndex &itemIndex, const QVariant &, int role)
{
    // FIXME
    if (role != Qt::CheckStateRole)
        return false;
    if (!itemIndex.isValid())
        return false;
    if (itemIndex.column() != 0)
        return false;

    // Check/un-check the root-item and it's children
    if (itemIndex.internalId() == InvalidIndex) {
        int row = itemIndex.row();
        if (row < 0 || row >= m_matchFiles.size()) return false;
        QVector<Match> &matches = m_matchFiles[row].matches;
        bool checked = m_matchFiles[row].checkState != Qt::Checked; // we toggle the current value
        for (int i = 0; i < matches.size(); ++i) {
            matches[i].checked = checked;
        }
        m_matchFiles[row].checkState = checked ? Qt::Checked : Qt::Unchecked;
        QModelIndex rootFileIndex = index(row, 0);
        dataChanged(rootFileIndex, rootFileIndex, QVector<int>(Qt::CheckStateRole));
        dataChanged(index(0, 0, rootFileIndex), index(matches.count()-1, 0, rootFileIndex), QVector<int>(Qt::CheckStateRole));
        return true;
    }

    int rootRow = itemIndex.internalId();
    if (rootRow < 0 || rootRow >= m_matchFiles.size())
        return false;

    int row = itemIndex.row();
    QVector<Match> &matches = m_matchFiles[rootRow].matches;
    if (row < 0 || row >= matches.size())
        return false;

    // we toggle the current value
    matches[row].checked = !matches[row].checked;

    int checkedCount = std::count_if(matches.begin(), matches.end(), isChecked);

    if (checkedCount == matches.size()) {
        m_matchFiles[rootRow].checkState = Qt::Checked;
    }
    else if (checkedCount == 0) {
        m_matchFiles[rootRow].checkState = Qt::Unchecked;
    }
    else {
        m_matchFiles[rootRow].checkState = Qt::PartiallyChecked;
    }

    QModelIndex rootFileIndex = index(rootRow, 0);
    dataChanged(rootFileIndex, rootFileIndex, QVector<int>(Qt::CheckStateRole));
    dataChanged(index(row, 0, rootFileIndex), index(row, 0, rootFileIndex), QVector<int>(Qt::CheckStateRole));
    return true;
}

Qt::ItemFlags MatchModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    if (index.column() == 0) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int MatchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_matchFiles.size();
    }

    if (parent.internalId() != InvalidIndex) {
        return 0;
    }

    int row = parent.row();
    if (row < 0 || row >= m_matchFiles.size()) {
        return 0;
    }

    return m_matchFiles[row].matches.size();
}

int MatchModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QModelIndex MatchModel::index(int row, int column, const QModelIndex &parent) const
{
    quint32 rootIndex = InvalidIndex;
    if (parent.isValid() && parent.internalId() == InvalidIndex) {
        rootIndex = parent.row();
    }
    return createIndex(row, column, rootIndex);
}

QModelIndex MatchModel::parent(const QModelIndex &child) const
{
    if (child.internalId() == InvalidIndex) {
        return QModelIndex();
    }
    return createIndex(child.internalId(), 0, InvalidIndex);
}
