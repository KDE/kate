//
// configview.h
//
// Description: View for configuring the set of targets to be used with the debugger
//
//
// Copyright (c) 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#ifndef CONFIGVIEW_H
#define CONFIGVIEW_H

#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QResizeEvent>

#include <kate/mainwindow.h>
#include <kconfiggroup.h>

class ConfigView : public QWidget
{
Q_OBJECT
public:
    ConfigView( QWidget* parent, Kate::MainWindow* mainWin );
    ~ConfigView();

public:
    void readConfig( KConfigBase* config, QString const& groupPrefix );
    void writeConfig( KConfigBase* config, QString const& groupPrefix );

    void    snapshotSettings();
    QString currentExecutable() const;
    QString currentWorkingDirectory() const;
    QString currentArgs() const;
    bool    takeFocusAlways() const;
    bool    showIOTab() const;

Q_SIGNALS:
    void showIO(bool show);

private Q_SLOTS:
    void slotTargetChanged( int index );
    void slotChooseTarget();
    void slotDeleteTarget();
    void slotChooseWorkingDirectory();
    void slotDeleteWorkingDirectory();

protected:
    void resizeEvent ( QResizeEvent * event );

private:
    Kate::MainWindow*   mainWindow;
    QComboBox*          targets;
    QComboBox*          workingDirectories;
    QComboBox*          argumentLists;
    QCheckBox*          takeFocus;
    QCheckBox*          redirectTerminal;
    bool                useBottomLayout;
    int                 widgetHeights;
    QLabel*             targetLabel;
    QLabel*             workDirLabel;
    QLabel*             argumentsLabel;
    QToolButton*        chooseTarget;
    QToolButton*        deleteTarget;
    QToolButton*        chooseWorkingDirectory;
    QToolButton*        deleteWorkingDirectory;
};

#endif
