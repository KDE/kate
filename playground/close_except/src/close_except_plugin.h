/**
 * \file
 *
 * \brief Class \c kate::CloseExceptPlugin (interface)
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

#ifndef __SRC__CLOSE_EXCEPT_PLUGIN_H__
#  define __SRC__CLOSE_EXCEPT_PLUGIN_H__

// Project specific includes

// Standard includes
#  include <kate/plugin.h>
#  include <kate/pluginconfigpageinterface.h>
#  include <KActionMenu>
#  include <KTextEditor/Document>
#  include <KTextEditor/View>
#  include <QtCore/QSignalMapper>
#  include <cassert>
#  include <set>

namespace kate {
class CloseExceptPlugin;                                    // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class CloseExceptPluginView
  : public Kate::PluginView
  , public Kate::XMLGUIClient
{
    Q_OBJECT
    typedef QMap<QString, QPointer<KAction> > actions_map_type;

public:
    /// Default constructor
    CloseExceptPluginView(Kate::MainWindow*, const KComponentData&, CloseExceptPlugin*);
    /// Destructor
    virtual ~CloseExceptPluginView();

private Q_SLOTS:
    void documentCreated(KTextEditor::Editor*, KTextEditor::Document*);
    void updateMenuSlotStub(KTextEditor::Document*);
    void close(const QString&, const bool);
    void closeExcept(const QString& item)
    {
        close(item, false);
    }
    void closeLike(const QString& item)
    {
        close(item, true);
    }

private:
    void updateMenu();
    QPointer<QSignalMapper> updateMenu(
        const std::set<QString>&
      , const std::set<QString>&
      , actions_map_type&
      , KActionMenu*
      );
    void appendActionsFrom(
        const std::set<QString>&
      , actions_map_type&
      , KActionMenu*
      , QSignalMapper*
      );

    CloseExceptPlugin* m_plugin;
    QPointer<KActionMenu> m_except_menu;
    QPointer<KActionMenu> m_like_menu;
    QPointer<QSignalMapper> m_except_mapper;
    QPointer<QSignalMapper> m_like_mapper;
    actions_map_type m_except_actions;
    actions_map_type m_like_actions;
};

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class CloseExceptPlugin : public Kate::Plugin
{
    Q_OBJECT

public:
    /// Default constructor
    CloseExceptPlugin(QObject* = 0, const QList<QVariant>& = QList<QVariant>());
    /// Destructor
    virtual ~CloseExceptPlugin() {}
    /// Create a new view of this plugin for the given main window
    Kate::PluginView* createView(Kate::MainWindow*);
};

}                                                           // namespace kate
#endif                                                      // __SRC__CLOSE_EXCEPT_PLUGIN_H__
