/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEQUICKOPENMODEL_H
#define KATEQUICKOPENMODEL_H

#include <QAbstractTableModel>
#include <QVariant>
#include <QVector>
#include <QUrl>

class KateMainWindow;

struct ModelEntry {
    QUrl url;         // used for actually opening a selected file (local or remote)
    QString fileName; // display string for left column
    QString filePath; // display string for right column
    bool bold;        // format line in bold text or not
    size_t sort_id;
    int score;
};

// needs to be defined outside of class to support forward declaration elsewhere
enum KateQuickOpenModelList : int { CurrentProject, AllProjects };

class KateQuickOpenModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Columns : int { FileName, FilePath, Bold };
    enum Role { Score = Qt::UserRole + 1 };
    explicit KateQuickOpenModel(KateMainWindow *mainWindow, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void refresh();
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

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!index.isValid())
            return false;
        if (role == Role::Score) {
            auto row = index.row();
            m_modelEntries[row].score = value.toInt();
        }
        return QAbstractTableModel::setData(index, value, role);
    }

private:
    QVector<ModelEntry> m_modelEntries;

    /* TODO: don't rely in a pointer to the main window.
     * this is bad engineering, but current code is too tight
     * on this and it's hard to untangle without breaking existing
     * code.
     */
    KateMainWindow *m_mainWindow;
    List m_listMode {};
};

#endif
