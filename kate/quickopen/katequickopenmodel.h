/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEQUICKOPENMODEL_H
#define KATEQUICKOPENMODEL_H

#include <QAbstractTableModel>
#include <QUrl>
#include <QVector>

class KateMainWindow;

struct ModelEntry {
    QUrl url;
    QString fileName; // display string for left column
    QString filePath; // display string for right column
    bool bold; // format line in bold text or not
    int score;
};

// needs to be defined outside of class to support forward declaration elsewhere
enum KateQuickOpenModelList : int { CurrentProject, AllProjects };

class KateQuickOpenModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Role { FileName = Qt::UserRole + 1, FilePath, Score };
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
        return row >= 0 && row < m_modelEntries.size();
    }

    void setScoreForIndex(int row, int score)
    {
        m_modelEntries[row].score = score;
    }

    const QString &idxToFileName(int row) const
    {
        return m_modelEntries.at(row).fileName;
    }

    const QString &idxToFilePath(int row) const
    {
        return m_modelEntries.at(row).filePath;
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
        return m_modelEntries.at(idx.row()).bold;
    }

private:
    QVector<ModelEntry> m_modelEntries;
    QString m_projectBase;
    List m_listMode{};
};

#endif
