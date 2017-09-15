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
# include <KTextEditor/Editor>
# include <KTextEditor/Plugin>
# include <ktexteditor/sessionconfiginterface.h>
# include <KActionMenu>
# include <KTextEditor/Document>
# include <KTextEditor/View>
# include <KTextEditor/Message>
# include <KToggleAction>
# include <KConfigGroup>
# include <QtCore/QSignalMapper>
# include <QPointer>
# include <cassert>
# include <set>
 
namespace kate {
class CloseExceptPlugin;                                    // forward declaration

/**
 * \brief Plugin to close docs grouped by extension or location
 */
class CloseExceptPluginView
  : public QObject
  , public KXMLGUIClient
{
    Q_OBJECT
    typedef QMap<QString, QPointer<QAction> > actions_map_type;

public:
    /// Default constructor
    CloseExceptPluginView(KTextEditor::MainWindow*, CloseExceptPlugin*);
    /// Destructor
    ~CloseExceptPluginView() override;

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
    void displayMessage(const QString&, const QString&, KTextEditor::Message::MessageType);
    void connectToDocument(KTextEditor::Document*);
    void updateMenu();
    QPointer<QSignalMapper> updateMenu(
        const std::set<QUrl>&
      , const std::set<QString>&
      , actions_map_type&
      , KActionMenu*
      );
    void appendActionsFrom(
        const std::set<QUrl>&
      , actions_map_type&
      , KActionMenu*
      , QSignalMapper*
      );
    void appendActionsFrom(
        const std::set<QString>& masks
      , actions_map_type& actions
      , KActionMenu* menu
      , QSignalMapper* mapper
      );

    CloseExceptPlugin* m_plugin;
    QPointer<KToggleAction> m_show_confirmation_action;
    QPointer<KActionMenu> m_except_menu;
    QPointer<KActionMenu> m_like_menu;
    QPointer<QSignalMapper> m_except_mapper;
    QPointer<QSignalMapper> m_like_mapper;
    actions_map_type m_except_actions;
    actions_map_type m_like_actions;
    KTextEditor::MainWindow *m_mainWindow;
    QPointer<KTextEditor::Message> m_infoMessage;
};

/**
 * \brief Plugin view class
 */
class CloseExceptPlugin : public KTextEditor::Plugin, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)
public:
    /// Default constructor
    CloseExceptPlugin(QObject* = nullptr, const QList<QVariant>& = QList<QVariant>());
    /// Destructor
    ~CloseExceptPlugin() override {}
    /// Create a new view of this plugin for the given main window
    QObject* createView(KTextEditor::MainWindow*) override;
    /// \name Plugin interface implementation
    //@{
    void readSessionConfig(const KConfigGroup&) override;
    void writeSessionConfig(KConfigGroup&) override;
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
