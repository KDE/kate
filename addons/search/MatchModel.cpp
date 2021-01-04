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


static const quintptr InfoItemId = 0xFFFFFFFF;
static const quintptr FileItemId = 0x7FFFFFFF;

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
void MatchModel::addMatches(const QUrl &fileUrl, const QVector<KateSearchMatch> &searchMatches)
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

    int matchIndex = m_matchFiles[fileIndex].matches.size();
    beginInsertRows(index(fileIndex, 0), matchIndex, matchIndex + searchMatches.size()-1);
    for (const auto &sMatch: searchMatches) {
        MatchModel::Match match;
        match.matchLen = sMatch.matchLen;
        match.startLine = sMatch.matchRange.start().line();
        match.startColumn = sMatch.matchRange.start().column();
        match.endLine = sMatch.matchRange.end().line();
        match.endColumn = sMatch.matchRange.end().column();

        int contextLen = totalContectLen - sMatch.matchLen;
        int preLen = qMin(contextLen/3, match.startColumn);
        int postLen = contextLen - preLen;

        match.preMatchStr = sMatch.lineContent.mid(match.startColumn-preLen, preLen);
        if (match.startColumn > preLen) match.preMatchStr.prepend(QLatin1String("..."));

        match.postMatchStr = sMatch.lineContent.mid(match.endColumn, postLen);
        if (match.endColumn+preLen < sMatch.lineContent.size()) match.postMatchStr.append(QLatin1String("..."));

        match.matchStr = sMatch.lineContent.mid(match.startColumn, sMatch.matchLen);

        m_matchFiles[fileIndex].matches.append(match);
    }
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

    int fileRow = index.internalId() == InfoItemId ? -1 : index.internalId() == FileItemId ? index.row() : (int)index.internalId();
    int matchRow = index.internalId() == InfoItemId || index.internalId() == FileItemId ? -1 : index.row();

    if (fileRow == -1) {
        // Info Item
        switch (role) {
            case Qt::DisplayRole:
                return m_infoHtmlString;
            case Qt::CheckStateRole:
                return m_infoCheckState;
        }
        return QVariant();
    }

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
            case FileUrlRole:
                return m_matchFiles[fileRow].fileUrl;
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
            case FileUrlRole:
                return m_matchFiles[fileRow].fileUrl;
            case StartLineRole:
                return match.startLine;
            case StartColumnRole:
                return match.startColumn;
            case EndLineRole:
                return match.endLine;
            case EndColumnRole:
                return match.endColumn;
            case MatchLenRole:
                return match.matchLen;
            case PreMatchRole:
                return match.preMatchStr;
            case MatchRole:
                return match.matchStr;
            case PostMatchRole:
                return match.postMatchStr;
            case ReplacedRole:
                return !match.replaceText.isEmpty();
            case ReplaceTextRole:
                return match.replaceText;
        }
    }
    else {
        qDebug() << "bad index";
        return QVariant();
    }


    return QVariant();
}

static bool isChecked(const MatchModel::Match &match) { return match.checked; }

bool MatchModel::setFileChecked(int fileRow, bool checked)
{
    if (fileRow < 0 || fileRow >= m_matchFiles.size()) return false;
    QVector<Match> &matches = m_matchFiles[fileRow].matches;
    for (int i = 0; i < matches.size(); ++i) {
        matches[i].checked = checked;
    }
    m_matchFiles[fileRow].checkState = checked ? Qt::Checked : Qt::Unchecked;
    QModelIndex rootFileIndex = index(fileRow, 0, createIndex(0, 0, InfoItemId));
    dataChanged(index(0, 0, rootFileIndex), index(matches.count()-1, 0, rootFileIndex), QVector<int>(Qt::CheckStateRole));
    dataChanged(rootFileIndex, rootFileIndex, QVector<int>(Qt::CheckStateRole));
    return true;
}


bool MatchModel::setData(const QModelIndex &itemIndex, const QVariant &, int role)
{
    if (role != Qt::CheckStateRole)
        return false;
    if (!itemIndex.isValid())
        return false;
    if (itemIndex.column() != 0)
        return false;

    // Check/un-check the File Item and it's children
    if (itemIndex.internalId() == InfoItemId) {
        bool checked = m_infoCheckState != Qt::Checked;
        for (int i=0; i<m_matchFiles.size(); ++i) {
            setFileChecked(i, checked);
        }
        m_infoCheckState = checked ? Qt::Checked : Qt::Unchecked;
        QModelIndex infoIndex = createIndex(0, 0, InfoItemId);
        dataChanged(infoIndex, infoIndex, QVector<int>(Qt::CheckStateRole));
        return true;
    }


    if (itemIndex.internalId() == FileItemId) {
        int fileRrow = itemIndex.row();
        if (fileRrow < 0 || fileRrow >= m_matchFiles.size()) return false;
        bool checked = m_matchFiles[fileRrow].checkState != Qt::Checked; // we toggle the current value
        setFileChecked(fileRrow, checked);

        // compare file items
        Qt::CheckState checkState = m_matchFiles[0].checkState;
        for (int i=1; i<m_matchFiles.size(); ++i) {
            if (checkState != m_matchFiles[i].checkState) {
                checkState = Qt::PartiallyChecked;
                break;
            }
        }
        m_infoCheckState = checkState;
        QModelIndex infoIndex = createIndex(0, 0, InfoItemId);
        dataChanged(infoIndex, infoIndex, QVector<int>(Qt::CheckStateRole));
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
        return 1;
    }

    if (parent.internalId() == InfoItemId) {
        return m_matchFiles.size();
    }

    if (parent.internalId() != FileItemId) {
        // matches do not have children
        return 0;
    }

    // If we get here parent.internalId() == FileItemId
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
    // Create the Info Item
    if (!parent.isValid()) {
        return createIndex(0, 0, InfoItemId);
    }

    // File Item
    if (parent.internalId() == InfoItemId) {
        return createIndex(row, column, FileItemId);
    }

    // Match Item
    if (parent.internalId() == FileItemId) {
        return createIndex(row, column, parent.row());
    }

    // Parent is a match which does not have children
    return QModelIndex();
}

QModelIndex MatchModel::parent(const QModelIndex &child) const
{
    if (child.internalId() == InfoItemId) {
        return QModelIndex();
    }

    if (child.internalId() == FileItemId) {
        return createIndex(0, 0, InfoItemId);
    }

    return createIndex(child.internalId(), 0, FileItemId);
}
