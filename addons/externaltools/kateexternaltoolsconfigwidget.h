/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#ifndef KTEXTEDITOR_EXTERNALTOOLS_CONFIGWIDGET_H
#define KTEXTEDITOR_EXTERNALTOOLS_CONFIGWIDGET_H

#include "ui_configwidget.h"
#include "ui_tooldialog.h"

#include <KTextEditor/Application>
#include <KTextEditor/ConfigPage>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>

#include <QDialog>
#include <QStandardItemModel>
#include <QPixmap>

class KConfig;
class KateExternalToolsPlugin;
class KateExternalTool;

/**
 * The config widget.
 * The config widget allows the user to view a list of services of the type
 * "Kate/ExternalTool" and add, remove or edit them.
 */
class KateExternalToolsConfigWidget : public KTextEditor::ConfigPage, public Ui::ExternalToolsConfigWidget
{
    Q_OBJECT
public:
    KateExternalToolsConfigWidget(QWidget* parent, KateExternalToolsPlugin* plugin);
    virtual ~KateExternalToolsConfigWidget();

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reset() override;
    void defaults() override { reset(); }

private Q_SLOTS:
    void slotAddCategory();
    void slotAddTool();
    void slotEdit();
    void slotRemove();
    void slotSelectionChanged();

    /**
     * Helper to open the ToolDialog.
     * Returns true, if the user clicked OK.
     */
    bool editTool(KateExternalTool* tool);

    /**
     * Creates a new category or returns existing one.
     */
    QStandardItem * addCategory(const QString & category);

    /**
     * Returns the currently active category. The returned pointer is always valid.
     */
    QStandardItem * currentCategory() const;

    /**
     * Clears the tools model.
     */
    void clearTools();

private:
    QPixmap blankIcon();

private:
    KConfig* m_config = nullptr;
    bool m_changed = false;
    KateExternalToolsPlugin* m_plugin;
    QStandardItemModel m_toolsModel;
    QStandardItem * m_noCategory = nullptr;
};

/**
 * A Dialog to edit a single KateExternalTool object
 */
class KateExternalToolServiceEditor : public QDialog
{
    Q_OBJECT

public:
    explicit KateExternalToolServiceEditor(KateExternalTool* tool = nullptr, QWidget* parent = nullptr);

private Q_SLOTS:
    /**
     * Run when the OK button is clicked, to ensure critical values are provided.
     */
    void slotOKClicked();

    /**
     * show a mimetype chooser dialog
     */
    void showMTDlg();

public:
    Ui::ToolDialog* ui;

private:
    KateExternalTool* m_tool;
};

#endif // KTEXTEDITOR_EXTERNALTOOLS_CONFIGWIDGET_H

// kate: space-indent on; indent-width 4; replace-tabs on;
