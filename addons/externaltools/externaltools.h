/*
   This file is part of the Kate text editor of the KDE project.
   It describes a "external tools" action for kate and provides a dialog
   page to configure that.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#ifndef KTEXTEDITOR_EXTERNALTOOLS_H
#define KTEXTEDITOR_EXTERNALTOOLS_H

#include "ui_configwidget.h"

#include <KTextEditor/ConfigPage>
#include <KTextEditor/Application>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/Command>

#include <KActionMenu>
#include <KMacroExpander>
#include <QDialog>

#include <QHash>
#include <QPixmap>

class KateExternalToolsPlugin;

/**
 * The external tools action
 * This action creates a menu, in which each item will launch a process
 * with the provided arguments, which may include the following macros:
 * - %URLS: the URLs of all open documents.
 * - %URL: The URL of the active document.
 * - %filedir: The directory of the current document, if that is a local file.
 * - %selection: The selection of the active document.
 * - %text: The text of the active document.
 * - %line: The line number of the cursor in the active view.
 * - %column: The column of the cursor in the active view.
 *
 * Each item has the following properties:
 * - Name: The friendly text for the menu
 * - Exec: The command to execute, including arguments.
 * - TryExec: the name of the executable, if not available, the
 *       item will not be displayed.
 * - MimeTypes: An optional list of mimetypes. The item will be disabled or
 *       hidden if the current file is not of one of the indicated types.
 *
 */
class KateExternalToolsMenuAction : public KActionMenu
{
    friend class KateExternalToolAction;

    Q_OBJECT
public:
    KateExternalToolsMenuAction(const QString& text, class KActionCollection* collection, QObject* parent,
                                class KTextEditor::MainWindow* mw = nullptr);
    virtual ~KateExternalToolsMenuAction();

    /**
     * This will load all the confiured services.
     */
    void reload();

    class KActionCollection* actionCollection() { return m_actionCollection; }

private Q_SLOTS:
    void slotDocumentChanged();

private:
    class KActionCollection* m_actionCollection;
    KTextEditor::MainWindow* mainwindow; // for the actions to access view/doc managers
};

/**
 * This Action contains a KateExternalTool
 */
class KateExternalToolAction : public QAction, public KWordMacroExpander
{
    Q_OBJECT
public:
    KateExternalToolAction(QObject* parent, class KateExternalTool* t);
    ~KateExternalToolAction();

protected:
    bool expandMacro(const QString& str, QStringList& ret) override;

private Q_SLOTS:
    void slotRun();

public:
    class KateExternalTool* tool;
};

/**
 * This class defines a single external tool.
 */
class KateExternalTool
{
public:
    explicit KateExternalTool(const QString& name = QString(), const QString& command = QString(),
                              const QString& icon = QString(), const QString& tryexec = QString(),
                              const QStringList& mimetypes = QStringList(), const QString& acname = QString(),
                              const QString& cmdname = QString(), int save = 0);
    ~KateExternalTool() {}

    QString name; ///< The name used in the menu.
    QString command; ///< The command to execute.
    QString icon; ///< the icon to use in the menu.
    QString tryexec; ///< The name or path of the executable.
    QStringList mimetypes; ///< Optional list of mimetypes for which this action is valid.
    bool hasexec; ///< This is set by the constructor by calling checkExec(), if a
                  ///< value is present.
    QString acname; ///< The name for the action. This is generated first time the
                    ///< action is is created.
    QString cmdname; ///< The name for the commandline.
    int save; ///< We can save documents prior to activating the tool command: 0 =
              ///< nothing, 1 = current document, 2 = all documents.

    /**
     * @return true if mimetypes is empty, or the @p mimetype matches.
     */
    bool valid(const QString& mimetype) const;
    /**
     * @return true if "tryexec" exists and has the executable bit set, or is
     * empty.
     * This is run at least once, and the tool is disabled if it fails.
     */
    bool checkExec();

private:
    QString m_exec; ///< The fully qualified path of the executable.
};

/**
 * The config widget.
 * The config widget allows the user to view a list of services of the type
 * "Kate/ExternalTool" and add, remove or edit them.
 */
class KateExternalToolsConfigWidget : public KTextEditor::ConfigPage, public Ui::ExternalToolsConfigWidget
{
    Q_OBJECT
public:
    KateExternalToolsConfigWidget(QWidget* parent, KateExternalToolsPlugin* plugin, const char* name);
    virtual ~KateExternalToolsConfigWidget();

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reset() override;
    void defaults() override { reset(); } // double sigh

private Q_SLOTS:
    void slotNew();
    void slotEdit();
    void slotRemove();
    void slotInsertSeparator();

    void slotMoveUp();
    void slotMoveDown();

    void slotSelectionChanged();

private:
    QPixmap blankIcon();

    QStringList m_removed;

    class KConfig* config;

    bool m_changed;
    KateExternalToolsPlugin* m_plugin;
};

/**
 * A Singleton class for invoking external tools with the view command line
 */
class KateExternalToolsCommand : public KTextEditor::Command
{
public:
    KateExternalToolsCommand(KateExternalToolsPlugin* plugin);
    virtual ~KateExternalToolsCommand() {}
    void reload();

public:
//     const QStringList& cmds() override; // FIXME
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg,
              const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;

private:
    QStringList m_list;
    QHash<QString, QString> m_map;
    QHash<QString, QString> m_name;
    bool m_inited;
    KateExternalToolsPlugin* m_plugin;
};

/**
 * A Dialog to edit a single KateExternalTool object
 */
class KateExternalToolServiceEditor : public QDialog
{
    Q_OBJECT

public:
    explicit KateExternalToolServiceEditor(KateExternalTool* tool = nullptr, QWidget* parent = nullptr);

    class QLineEdit *leName, *leExecutable, *leMimetypes, *leCmdLine;
    class QTextEdit* teCommand;
    class KIconButton* btnIcon;
    class QComboBox* cmbSave;

private Q_SLOTS:
    /**
     * Run when the OK/Cancel button is clicked, to ensure critical values are
     * provided
     */
    void slotButtonClicked(int button);
    /**
     * show a mimetype chooser dialog
     */
    void showMTDlg();

private:
    KateExternalTool* tool;
};

#endif // KTEXTEDITOR_EXTERNALTOOLS_H

// kate: space-indent on; indent-width 2; replace-tabs on;
