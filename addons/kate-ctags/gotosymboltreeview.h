#pragma once
/*
    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QTreeView>

namespace KTextEditor
{
class MainWindow;
}

class GotoSymbolTreeView : public QTreeView
{
public:
    explicit GotoSymbolTreeView(KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);
    int sizeHintWidth() const;
    void setGlobalMode(bool value)
    {
        globalMode = value;
    }

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

private:
    KTextEditor::MainWindow *m_mainWindow;
    bool globalMode = false;
};
