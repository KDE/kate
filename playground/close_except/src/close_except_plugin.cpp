/**
 * \file
 *
 * \brief Class \c kate::CloseExceptPlugin (implementation)
 *
 * \date Thu Mar  8 08:13:43 MSK 2012 -- Initial design
 */
/*
 * KateCloseExceptPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateCloseExceptPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project specific includes
#include <src/config.h>
#include <src/close_except_plugin.h>

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KAboutData>
#include <KActionCollection>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KTextEditor/Editor>
#include <QtCore/QFileInfo>

K_PLUGIN_FACTORY(CloseExceptPluginFactory, registerPlugin<kate::CloseExceptPlugin>();)
K_EXPORT_PLUGIN(
    CloseExceptPluginFactory(
        KAboutData(
            "katecloseexceptplugin"
          , "kate_closeexcept_plugin"
          , ki18n("Close documents depending on path")
          , PLUGIN_VERSION
          , ki18n("Close all documents started from specified path")
          , KAboutData::License_LGPL_V3
          )
      )
  )

namespace kate {
//BEGIN CloseExceptPlugin
CloseExceptPlugin::CloseExceptPlugin(
    QObject* application
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(application), "kate_closeexcept_plugin")
{
}

Kate::PluginView* CloseExceptPlugin::createView(Kate::MainWindow* parent)
{
    return new CloseExceptPluginView(parent, CloseExceptPluginFactory::componentData(), this);
}
//END CloseExceptPlugin

//BEGIN CloseExceptPluginView
CloseExceptPluginView::CloseExceptPluginView(
    Kate::MainWindow* mw
  , const KComponentData& data
  , CloseExceptPlugin* plugin
  )
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
  , m_plugin(plugin)
  , m_except_menu(new KActionMenu(i18n("Close Except"), this))
  , m_like_menu(new KActionMenu(i18n("Close Like"), this))
{
    actionCollection()->addAction("file_close_except", m_except_menu);
    actionCollection()->addAction("file_close_like", m_like_menu);

    // Subscribe self to document creation
    connect(
        m_plugin->application()->editor()
      , SIGNAL(documentCreated(KTextEditor::Editor*, KTextEditor::Document*))
      , this
      , SLOT(documentCreated(KTextEditor::Editor*, KTextEditor::Document*))
      );

    mainWindow()->guiFactory()->addClient(this);
    // Fill menu w/ currently opened document masks/groups
    updateMenu();
}

CloseExceptPluginView::~CloseExceptPluginView() {
    mainWindow()->guiFactory()->removeClient(this);
}

void CloseExceptPluginView::documentCreated(KTextEditor::Editor*, KTextEditor::Document* document)
{
    kDebug() << "A new document opened " << document->url();

    // Subscribe self to document close and name changes
    connect(
        document
      , SIGNAL(aboutToClose(KTextEditor::Document*))
      , this
      , SLOT(updateMenuSlotStub(KTextEditor::Document*))
      );
    connect(
        document
      , SIGNAL(documentNameChanged(KTextEditor::Document*))
      , this
      , SLOT(updateMenuSlotStub(KTextEditor::Document*))
      );
}

void CloseExceptPluginView::updateMenuSlotStub(KTextEditor::Document*)
{
    updateMenu();
}

void CloseExceptPluginView::appendActionsFrom(
    const std::set<QString>& paths
  , actions_map_type& actions
  , KActionMenu* menu
  , QSignalMapper* mapper
  )
{
    Q_FOREACH(const QString& path, paths)
    {
        actions[path] = QPointer<KAction>(new KAction(path, menu));
        menu->addAction(actions[path]);
        connect(actions[path], SIGNAL(triggered()), mapper, SLOT(map()));
        mapper->setMapping(actions[path], path);
    }
}

QPointer<QSignalMapper> CloseExceptPluginView::updateMenu(
    const std::set<QString>& paths
  , const std::set<QString>& masks
  , actions_map_type& actions
  , KActionMenu* menu
  )
{
    // turn menu ON or OFF depending on collected results
    menu->setEnabled(!paths.empty());

    // Clear previous menus
    for (actions_map_type::iterator it = actions.begin(), last = actions.end(); it !=last;)
    {
        menu->removeAction(*it);
        actions.erase(it++);
    }
    // Form a new one
    QPointer<QSignalMapper> mapper = QPointer<QSignalMapper>(new QSignalMapper(this));
    appendActionsFrom(paths, actions, menu, mapper);
    if (!masks.empty())
    {
        if (!paths.empty())
            menu->addSeparator();                           // Add separator between paths and file's ext filters
        appendActionsFrom(masks, actions, menu, mapper);
    }
    return mapper;
}

void CloseExceptPluginView::updateMenu()
{
    kDebug() << "... updating menu ...";
    const QList<KTextEditor::Document*>& docs = m_plugin->application()->documentManager()->documents();
    if (docs.empty())
    {
        kDebug() << "No docs r opened right now --> disable menu";
        m_except_menu->setEnabled(false);
        m_like_menu->setEnabled(false);
        /// \note It seems there is always a document present... it named \em 'Untitled'
    }
    else
    {
        // Iterate over documents and form a set of candidates
        std::set<QString> paths;
        std::set<QString> masks;
        Q_FOREACH(KTextEditor::Document* document, docs)
        {
            const QString& ext = QFileInfo(document->url().path()).completeSuffix();
            if (!ext.isEmpty())
                masks.insert("*." + ext);
            for (
                KUrl url = document->url().upUrl()
              ; url.hasPath() && url.path() != "/"
              ; url = url.upUrl()
              ) paths.insert(url.path() + "*");
        }
        m_except_mapper = updateMenu(paths, masks, m_except_actions, m_except_menu);
        m_like_mapper = updateMenu(paths, masks, m_like_actions, m_like_menu);
        connect(m_except_mapper, SIGNAL(mapped(const QString&)), this, SLOT(closeExcept(const QString&)));
        connect(m_like_mapper, SIGNAL(mapped(const QString&)), this, SLOT(closeLike(const QString&)));
    }
}

void CloseExceptPluginView::close(const QString& item, const bool close_if_match)
{
    assert("Expect non empty parameter" && item.isEmpty());
    assert(
        "Parameter seems invalid! Is smth changed in the code?"
      && !item.isEmpty() && (item[0] == '*' || item[item.size() - 1] == '*')
      );

    const bool is_path = item[0] != '*';
    const QString mask = is_path ? item.left(item.size() - 1) : item;
    kDebug() << "Going to close items [" << close_if_match << "/" << is_path << "]: " << mask;

    QList<KTextEditor::Document*> docs2close;
    const QList<KTextEditor::Document*>& docs = m_plugin->application()->documentManager()->documents();
    Q_FOREACH(KTextEditor::Document* document, docs)
    {
        const QString& path = document->url().upUrl().path();
        const QString& ext = QFileInfo(document->url().fileName()).completeSuffix();
        const bool match = (!is_path && mask.endsWith(ext))
          || (is_path && (mask.size() < path.size() ? path.startsWith(mask) : mask.startsWith(path)))
          ;
        if (match == close_if_match)
        {
            kDebug() << "*** Will close: " << document->url();
            docs2close.push_back(document);
        }
    }
    // Close 'em all!
    m_plugin->application()->documentManager()->closeDocumentList(docs2close);
    updateMenu();
}
//END CloseExceptPluginView
}                                                           // namespace kate
