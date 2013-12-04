/**
 * \file
 *
 * \brief Declate Kate's Close Except/Like plugin classes
 *
 * Copyright (C) 2012 Alex Turbov <i.zaufi@gmail.com>
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
# define __SRC__CLOSE_EXCEPT_PLUGIN_H__

// Project specific includes

// Standard includes
# include <kate/plugin.h>
# include <kate/pluginconfigpageinterface.h>
# include <KActionMenu>
# include <KTextEditor/Document>
# include <KTextEditor/View>
# include <KToggleAction>
# include <QtCore/QSignalMapper>
# include <cassert>
# include <set>

namespace kate {
class CloseExceptPlugin;                                    // forward declaration

/**
 * \brief Plugin to close docs grouped by extension or location
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
    ~CloseExceptPluginView();

private Q_SLOTS:
    void viewCreated(KTextEditor::View*);
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
    void connectToDocument(KTextEditor::Document*);
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
    QPointer<KToggleAction> m_show_confirmation_action;
    QPointer<KActionMenu> m_except_menu;
    QPointer<KActionMenu> m_like_menu;
    QPointer<QSignalMapper> m_except_mapper;
    QPointer<QSignalMapper> m_like_mapper;
    actions_map_type m_except_actions;
    actions_map_type m_like_actions;
};

/**
 * \brief Plugin view class
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
    /// \name Plugin interface implementation
    //@{
    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    //@}
    bool showConfirmationNeeded() const
    {
        return m_show_confirmation_needed;
    }

public Q_SLOTS:
    void toggleShowConfirmation(bool flag)
    {
        m_show_confirmation_needed = flag;
    }

private:
    bool m_show_confirmation_needed;
};

}                                                           // namespace kate
#endif                                                      // __SRC__CLOSE_EXCEPT_PLUGIN_H__
