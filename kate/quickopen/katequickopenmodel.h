/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEQUICKOPENMODEL_H
#define KATEQUICKOPENMODEL_H

#include <QAbstractTableModel>
#include <QUrl>

#include <vector>

class KateMainWindow;
namespace KTextEditor
{
class Document;
}

struct ModelEntry {
    QUrl url;
    QString fileName; // display string for left column
    QString filePath; // display string for right column
    KTextEditor::Document *document = nullptr; // document for entry, if already open
    int score = -1;
};

// needs to be defined outside of class to support forward declaration elsewhere
enum KateQuickOpenModelList : int { CurrentProject, AllProjects };

class KateQuickOpenModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Role { FileName = Qt::UserRole + 1, FilePath, Score, Document };
    explicit KateQuickOpenModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void refresh(KateMainWindow *mainWindow);
    // add a convenient in-class alias
    using List = KateQuickOpenModelList;
    List listMode() const
    {
        return m_listMode;
    }
    void setListMode(List mode)
    {
        m_listMode = mode;
    }

    bool isValid(int row) const
    {
        return row >= 0 && (size_t)row < m_modelEntries.size();
    }

    void setScoreForIndex(int row, int score)
    {
        m_modelEntries[row].score = score;
    }

    const QString &idxToFileName(int row) const
    {
        return m_modelEntries.at(row).fileName;
    }

    QStringView idxToFilePath(int row) const
    {
        const QString &path = m_modelEntries.at(row).filePath;
        QStringView pth(path.data(), path.size());
        // handle exceptional non-saved file case
        return pth.startsWith(QStringView(m_projectBase.data(), m_projectBase.size())) ? pth.mid(m_projectBase.size()) : pth;
    }

    int idxScore(const QModelIndex &idx) const
    {
        if (!idx.isValid()) {
            return {};
        }
        return m_modelEntries.at(idx.row()).score;
    }

    bool isOpened(const QModelIndex &idx) const
    {
        if (!idx.isValid()) {
            return {};
        }
        return m_modelEntries.at(idx.row()).document;
    }

    bool isOpened(int row) const
    {
        return m_modelEntries.at(row).document;
    }

private:
    std::vector<ModelEntry> m_modelEntries;
    QString m_projectBase;
    List m_listMode{};
};

#endif
