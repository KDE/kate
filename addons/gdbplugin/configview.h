//
// configview.h
//
// Description: View for configuring the set of targets to be used with the debugger
//
//
// Copyright (c) 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// Copyright (c) 2012 Kåre Särs <kare.sars@iki.fi>
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

#include "advanced_settings.h"

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QComboBox>
#include <QCheckBox>
#include <QBoxLayout>
#include <QResizeEvent>

#include <QList>

#include <KTextEditor/MainWindow>
#include <kconfiggroup.h>
#include <kactioncollection.h>
#include <kselectaction.h>

struct GDBTargetConf {
    QString     executable;
    QString     workDir;
    QString     arguments;
    QString     gdbCmd;
    QStringList customInit;
    QStringList srcPaths;
};

class ConfigView : public QWidget
{
Q_OBJECT
public:
    enum TargetStringOrder {
        NameIndex = 0,
        ExecIndex,
        WorkDirIndex,
        ArgsIndex,
        GDBIndex,
        CustomStartIndex
    };

    ConfigView(QWidget* parent, KTextEditor::MainWindow* mainWin);
    ~ConfigView() override;

public:
    void registerActions(KActionCollection* actionCollection);

    void readConfig (const KConfigGroup& config);
    void writeConfig (KConfigGroup& config);

    const GDBTargetConf currentTarget() const;
    bool  takeFocusAlways() const;
    bool  showIOTab() const;

Q_SIGNALS:
    void showIO(bool show);

private Q_SLOTS:
    void slotTargetEdited(const QString &newText);
    void slotTargetSelected(int index);
    void slotAddTarget();
    void slotCopyTarget();
    void slotDeleteTarget();
    void slotAdvancedClicked();
    void slotBrowseExec();
    void slotBrowseDir();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void saveCurrentToIndex(int index);
    void loadFromIndex(int index);
    void setAdvancedOptions();

private:
    KTextEditor::MainWindow*   m_mainWindow;
    QComboBox*          m_targetCombo;
    int                 m_currentTarget;
    QToolButton*        m_addTarget;
    QToolButton*        m_copyTarget;
    QToolButton*        m_deleteTarget;
    QFrame*             m_line;

    QLineEdit*          m_executable;
    QToolButton*        m_browseExe;

    QLineEdit*          m_workingDirectory;
    QToolButton*        m_browseDir;

    QLineEdit*          m_arguments;

    QCheckBox*          m_takeFocus;
    QCheckBox*          m_redirectTerminal;
    QPushButton*        m_advancedSettings;
    QBoxLayout*         m_checBoxLayout;

    bool                m_useBottomLayout;
    QLabel*             m_execLabel;
    QLabel*             m_workDirLabel;
    QLabel*             m_argumentsLabel;
    KSelectAction*      m_targetSelectAction;

    AdvancedGDBSettings* m_advanced;
};

#endif
