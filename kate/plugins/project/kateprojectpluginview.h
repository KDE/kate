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

#include "plugin_kateproject.h"
#include "kateproject.h"
#include "kateprojectview.h"

#include <QPointer>
#include <QToolBox>

class KateProjectPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

    Q_PROPERTY(QString projectFileName READ projectFileName NOTIFY projectFileNameChanged)
    Q_PROPERTY(QVariantMap projectMap READ projectMap)
    Q_PROPERTY(QStringList projectFiles READ projectFiles)

  public:
    KateProjectPluginView( KateProjectPlugin *plugin, Kate::MainWindow *mainWindow );
    ~KateProjectPluginView();

    virtual void readSessionConfig( KConfigBase* config, const QString& groupPrefix );
    virtual void writeSessionConfig( KConfigBase* config, const QString& groupPrefix );

    /**
     * content of current active project, as variant map
     * @return empty map if no project active, else content of project JSON
     */
    QVariantMap projectMap () const;
    
    /**
     * which project file is currently active?
     * @return empty string if none, else project file name
     */
    QString projectFileName () const;

    /**
     * files for the current active project?
     * @return empty list if none, else project files as stringlist
     */
    QStringList projectFiles () const;

  public slots:
    /**
     * Create view for given project.
     * Either gives existing one or creates new one
     * @param project project we want view for
     * @return view
     */
    KateProjectView *viewForProject (KateProject *project);

  Q_SIGNALS:
    /**
     * Emitted if currentProjectFileName changed.
     */
    void projectFileNameChanged ();

  private slots:
    /**
     * This slot is called whenever the active view changes in our main window.
     */
    void slotViewChanged ();
    
    /**
     * Current project changed.
     * @param index index in toolbox
     */
    void slotCurrentChanged (int index);
    
    /**
     * Url changed, to auto-load projects
     */
    void slotDocumentUrlChanged (KTextEditor::Document *document);

  private:
    /**
     * our plugin
     */
    KateProjectPlugin *m_plugin;

    /**
     * our projects toolview
     */
    QWidget *m_toolView;

    /**
     * the toolbox for the projects
     */
    QToolBox *m_toolBox;

    /**
     * project => view
     */
    QMap<KateProject *, KateProjectView *> m_project2View;
    
    /**
     * remember current active view text editor view
     * might be 0
     */
    QPointer<KTextEditor::View> m_activeTextEditorView;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

