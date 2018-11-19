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

#ifndef KATEQUICKOPENMODEL_H
#define KATEQUICKOPENMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <tuple>
#include <QVariant>

#include "katemainwindow.h"

struct ModelEntry {
    QUrl url; // used for actually opening a selected file (local or remote)
    QString fileName; // display string for left column
    QString filePath; // display string for right column
    bool bold; // format line in bold text or not
};

class KateQuickOpenModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Columns : int { FileName, FilePath, Bold };
    explicit KateQuickOpenModel(KateMainWindow *mainWindow, QObject *parent=nullptr);
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    void refresh();

private:
    QVector<ModelEntry> m_modelEntries;

    /* TODO: don't rely in a pointer to the main window.
    * this is bad engineering, but current code is too tight
    * on this and it's hard to untangle without breaking existing
    * code.
    */
    KateMainWindow *m_mainWindow;
};

#endif
