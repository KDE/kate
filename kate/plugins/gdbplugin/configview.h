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
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QResizeEvent>

#include <kate/mainwindow.h>
#include <kconfiggroup.h>
#include <kactioncollection.h>
#include <kselectaction.h>

class ConfigView : public QWidget
{
Q_OBJECT
public:
    ConfigView( QWidget* parent, Kate::MainWindow* mainWin );
    ~ConfigView();

public:
    void registerActions( KActionCollection* actionCollection );

    void readConfig( KConfigBase* config, QString const& groupPrefix );
    void writeConfig( KConfigBase* config, QString const& groupPrefix );

    QString currentExecutable() const;
    QString currentWorkingDirectory() const;
    QString currentArgs() const;
    bool    takeFocusAlways() const;
    bool    showIOTab() const;

Q_SIGNALS:
    void showIO(bool show);

private Q_SLOTS:
    void slotTargetEdited( QString updatedTarget );
    void slotTargetSelected( int index );
    void slotAddTarget();
    void slotDeleteTarget();
    void slotWorkingDirectoryEdited( QString updatedWorkingDirectory );
    void slotArgListEdited( QString updatedArgList );
    void slotArgListSelected( int index );
    void slotAddArgList();
    void slotDeleteArgList();

protected:
    void resizeEvent ( QResizeEvent * event );

private:
    void updateCurrentTargetDescription( int field, QString const& value );

    /* helper class to ensure state change is exception safe */
    class ChangingTarget
    {
    public:
        ChangingTarget( ConfigView* view );
        ~ChangingTarget();

    private:
        ConfigView* m_view;
    };
    friend class ChangingTarget;

private:
    Kate::MainWindow*   m_mainWindow;
    QComboBox*          m_targets;
    QLineEdit*          m_workingDirectory;
    QComboBox*          m_argumentLists;
    QCheckBox*          m_takeFocus;
    QCheckBox*          m_redirectTerminal;
    bool                m_useBottomLayout;
    int                 m_widgetHeights;
    QLabel*             m_targetLabel;
    QLabel*             m_workDirLabel;
    QLabel*             m_argumentsLabel;
    QToolButton*        m_addTarget;
    QToolButton*        m_deleteTarget;
    QToolButton*        m_addArgList;
    QToolButton*        m_deleteArgList;
    KSelectAction*      m_targetSelectAction;
    KSelectAction*      m_argListSelectAction;
    int                 m_changingTarget;
};

#endif
