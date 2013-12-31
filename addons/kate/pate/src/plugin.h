// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2012, 2013 Shaheed Haque <srhaque@theiet.org>
// Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.
//

#ifndef _PATE_PLUGIN_H_
# define _PATE_PLUGIN_H_

# include <Python.h>
# include <kate/mainwindow.h>
# include <kate/plugin.h>
# include <ktexteditor/configpageinterface.h>

# include <QList>

# include "engine.h"
# include "ui_manager.h"

// Fwd decls
class QPushButton;
class QCheckBox;
class QTreeView;

namespace Pate
{
/**
 * The Pate plugin supports the creation of Kate scripts in Python. The script
 * modules must provide a \c .desktop file describing the plugin name, comment
 * and particular Python module to load.
 *
 * The modules which are found, along with three pre-defined support modules
 * (\c kate, \c kate.gui and \c pate) are displayed and managed through this
 * plugin.
 *
 * The modules which are failed to load gets disabled in a manager. Usable
 * modules can be enabled by the user, and any methods decorated with
 * \c kate.action or \c kate.configPage will be hooked into Kate as appropriate.
 *
 * Configuration support has these elements:
 *
 *  - Configuration of the Pate plugin itself. This is stored in \c katerc as for
 *    other Kate plugins in appropriately named groups.
 *
 *  - Configuration of modules. This is provided via \c kate.configuration which
 *    allows each module native Python object configuration storage via
 *    katepaterc. Plugins which wish to can hook their configuration into
 *    Kate's configuration system using \c kate.configPage.
 */
class Plugin
  : public Kate::Plugin
  , public KTextEditor::ConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::ConfigPageInterface)

public:
    explicit Plugin(QObject* = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~Plugin();

    Kate::PluginView* createView(Kate::MainWindow*);

    /**
     *  Read the configuration for the plugin.
     */
    virtual void readSessionConfig(KConfigBase* config, const QString& groupPrefix);

    /**
     * Write the configuration for the plugin.
     */
    virtual void writeSessionConfig(KConfigBase* config, const QString& groupPrefix);

    //BEGIN PluginConfigPageInterface
    uint configPages() const;
    KTextEditor::ConfigPage* configPage(uint number = 0, QWidget* parent = 0, const char* name = 0);
    QString configPageName(uint number = 0) const;
    QString configPageFullName(uint number = 0) const;
    KIcon configPageIcon(uint number = 0) const;
    //END PluginConfigPageInterface

    /// Read-only access to the engine
    const Pate::Engine& engine() const;
    /// Read-write access to the engine
    Pate::Engine& engine();
    /// Check if engine is initialized properly,
    /// show a passive popup if it doesn't
    bool checkEngineShowPopup() const;
    void setFailureReason(QString);

private:
    friend class ConfigPage;
    void reloadModuleConfigPages() const;
    static QString getSessionPrivateStorageFilename(KConfigBase*);

    mutable QList<PyObject*> m_moduleConfigPages;
    Pate::Engine m_engine;
    QString m_engineFailureReason;
    bool m_autoReload;
};

/**
 * The Pate plugin view.
 */
class PluginView
  : public Kate::PluginView
  , public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    PluginView(Kate::MainWindow*, Plugin*);
    ~PluginView();

public Q_SLOTS:
    void aboutPate();

private:
    Plugin* m_plugin;
};

/**
 * The Pate plugin's configuration view.
 */
class ConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit ConfigPage(QWidget* = 0, Plugin* = 0);
    virtual ~ConfigPage();

public Q_SLOTS:
    virtual void apply();
    virtual void reset();
    virtual void defaults();

private:
    Plugin* m_plugin;
    Ui::ManagerPage m_manager;
};

/**
 * A page used if an error occurred trying to load a plugin's config page.
 */
class ErrorConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit ErrorConfigPage(QWidget* parent = 0, const QString& traceback = QString());

public Q_SLOTS:
    virtual void apply() {}
    virtual void reset() {}
    virtual void defaults() {}
};

}                                                           // namespace Pate
#endif                                                      // _PATE_PLUGIN_H_
// kate: indent-width 4;
