/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef _PLUGIN_KATE_PROJECTVIEW_H_
#define _PLUGIN_KATE_PROJECTVIEW_H_

#include "kateprojectplugin.h"
#include "kateproject.h"
#include "kateprojectview.h"
#include "kateprojectinfoview.h"

#include <QPointer>
#include <QComboBox>
#include <QStackedWidget>
#include <QToolButton>

#include <KXMLGUIClient>

class QAction;

class KateProjectPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    Q_PROPERTY(QString projectFileName READ projectFileName NOTIFY projectFileNameChanged)
    Q_PROPERTY(QString projectName READ projectName)
    Q_PROPERTY(QString projectBaseDir READ projectBaseDir)
    Q_PROPERTY(QVariantMap projectMap READ projectMap NOTIFY projectMapChanged)
    Q_PROPERTY(QStringList projectFiles READ projectFiles)

    Q_PROPERTY(QString allProjectsCommonBaseDir READ allProjectsCommonBaseDir)
    Q_PROPERTY(QStringList allProjectsFiles READ allProjectsFiles)

public:
    KateProjectPluginView(KateProjectPlugin *plugin, KTextEditor::MainWindow *mainWindow);
    ~KateProjectPluginView() override;

    /**
     * content of current active project, as variant map
     * @return empty map if no project active, else content of project JSON
     */
    QVariantMap projectMap() const;

    /**
     * which project file is currently active?
     * @return empty string if none, else project file name
     */
    QString projectFileName() const;

    /**
     * Returns the name of the project
     */
    QString projectName() const;

    /**
     * Returns the base directory of the project
     */
    QString projectBaseDir() const;

    /**
     * files for the current active project?
     * @return empty list if none, else project files as stringlist
     */
    QStringList projectFiles() const;

    /**
     * Example: Two projects are loaded with baseDir1="/home/dev/project1" and
     * baseDir2="/home/dev/project2". Then "/home/dev/" is returned.
     * @see projectBaseDir().
     * Used for the Search&Replace plugin for option "Search in all open projects".
     */
    QString allProjectsCommonBaseDir() const;

    /**
     * @returns a flat list of files for all open projects (@see also projectFiles())
     */
    QStringList allProjectsFiles() const;

    /**
     * the main window we belong to
     * @return our main window
     */
    KTextEditor::MainWindow *mainWindow() const {
        return m_mainWindow;
    }

public Q_SLOTS:
    /**
     * Create views for given project.
     * Either gives existing ones or creates new one
     * @param project project we want view for
     * @return views (normal + info view)
     */
    QPair<KateProjectView *, KateProjectInfoView *> viewForProject(KateProject *project);

private Q_SLOTS:
    /**
     * New view got created, we need to update our connections
     * @param view new created view
     */
    void slotViewCreated(KTextEditor::View *view);

    /**
     * View got destroyed.
     * @param view deleted view
     */
    void slotViewDestroyed(QObject *view);

    /**
     * Activate the previous project.
     */
    void slotProjectPrev();

    /**
     * Activate the next project.
     */
    void slotProjectNext();

    /**
     * Reload current project, if any.
     * This will trigger a reload with force.
     */
    void slotProjectReload();

    /**
     * Lookup current word
     */
    void slotProjectIndex();

Q_SIGNALS:
    /**
     * Emitted if projectFileName changed.
     */
    void projectFileNameChanged();

    /**
     * Emitted if projectMap changed.
     */
    void projectMapChanged();

    /**
     * Emitted when a ctags lookup in requested
     * @param word lookup word
     */
    void projectLookupWord(const QString &word);

private Q_SLOTS:
    /**
     * This slot is called whenever the active view changes in our main window.
     */
    void slotViewChanged();

    /**
     * Current project changed.
     * @param index index in toolbox
     */
    void slotCurrentChanged(int index);

    /**
     * Url changed, to auto-load projects
     */
    void slotDocumentUrlChanged(KTextEditor::Document *document);

    /**
     * Show context menu
     */
    void slotContextMenuAboutToShow();

private:
    /**
     * find current selected or under cursor word
     */
    QString currentWord() const;

private:
    /**
     * our plugin
     */
    KateProjectPlugin *m_plugin;

    /**
     * the main window we belong to
     */
    KTextEditor::MainWindow *m_mainWindow;

    /**
     * our projects toolview
     */
    QWidget *m_toolView;

    /**
     * our projects info toolview
     */
    QWidget *m_toolInfoView;

    /**
     * combo box with all loaded projects inside
     */
    QComboBox *m_projectsCombo;

    /**
     * Reload button
     */
    QToolButton *m_reloadButton;

    /**
     * stacked widget will all currently created project views
     */
    QStackedWidget *m_stackedProjectViews;

    /**
     * stacked widget will all currently created project info views
     */
    QStackedWidget *m_stackedProjectInfoViews;

    /**
     * project => view
     */
    QMap<KateProject *, QPair<KateProjectView *, KateProjectInfoView *> > m_project2View;

    /**
     * remember current active view text editor view
     * might be 0
     */
    QPointer<KTextEditor::View> m_activeTextEditorView;

    /**
     * remember for which text views we might need to cleanup stuff
     */
    QSet<QObject *> m_textViews;

    /**
     * lookup action
     */
    QAction *m_lookupAction;
};

#endif

