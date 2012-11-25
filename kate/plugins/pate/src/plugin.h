// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2012 Shaheed Haque <srhaque@theiet.org>
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

#ifndef PATE_PLUGIN_H
#define PATE_PLUGIN_H

#include "Python.h"
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>

#include <kxmlguiclient.h>

#include <QList>

#include "ui_info.h"
#include "ui_manager.h"


class QPushButton;
class QCheckBox;
class QTreeView;

namespace Pate
{

/**
 * The Pate plugin supports the creation of Kate scripts in Python. The script
 * modules are either as individual .py files (e.g. bar.py), or a directory
 * which contains a .py file named after the directory (i.e. foo/foo.py for
 * directory foo).
 *
 * The modules which are found, along with three pre-defined support modules
 * ("kate", "kate,gui" and "pate") are displayed and managed through this
 * plugin.
 *
 * The modules are looked for in an ordered series of directories which are
 * added to sys.path. A module found in one directory "hides" similarly named
 * modules in later directories; the hidden modules cannot be used. Usable
 * modules can be enabled by the user, and any methods decorated with
 * "kate.action" or "kate.configPage" will be hooked into Kate as appropriate.
 *
 * Configuration support has these elements:
 *
 *  - Configuration of the Pate plugin itself. This is stored in katerc as for
 *    other Kate plugins in appropriately named groups.
 *
 *  - Configuration of modules. This is provided via "kate.configuration" which
 *    allows each module native Python object configuration storage via
 *    katepaterc. Plugins which wish to can hook their configuration into
 *    Kate's configuration system using "kate.configPage".
 */
class Plugin :
    public Kate::Plugin,
    public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

public:
    explicit Plugin(QObject *parent = 0, const QStringList& = QStringList());
    virtual ~Plugin();

    Kate::PluginView *createView(Kate::MainWindow *mainWindow);

    /**
     * Read the config for the plugin.
     */
    virtual void readSessionConfig(KConfigBase *config, const QString &groupPrefix);

    /**
     * Write the config for the plugin.
     */
    virtual void writeSessionConfig(KConfigBase *config, const QString &groupPrefix);

    // PluginConfigPageInterface
    uint configPages() const;
    Kate::PluginConfigPage *configPage(uint number = 0, QWidget *parent = 0, const char *name = 0);
    QString configPageName(uint number = 0) const;
    QString configPageFullName(uint number = 0) const;
    KIcon configPageIcon(uint number = 0) const;

private:
    friend class ConfigPage;
    bool m_autoReload;
    mutable QList<PyObject *> m_moduleConfigPages;
    void reloadModuleConfigPages() const;
};

/**
 * The Pate plugin view.
 */
class PluginView :
    public Kate::PluginView
{
    Q_OBJECT

public:
    PluginView(Kate::MainWindow *window);
};

/**
 * The Pate plugin's configuration view.
 */
class ConfigPage :
    public Kate::PluginConfigPage
{
    Q_OBJECT

public:
    explicit ConfigPage(QWidget *parent = 0, Plugin *plugin = 0);
    virtual ~ConfigPage();

public Q_SLOTS:
    virtual void apply();
    virtual void reset();
    virtual void defaults();

private:
    Plugin *m_plugin;
    Ui::ManagerPage m_manager;
    Ui::InfoPage m_info;
    PyObject *m_pluginActions;
    PyObject *m_pluginConfigPages;

private slots:
    void reloadPage(bool init);
    void infoTopicChanged(int topicIndex);
    void infoPluginActionsChanged(int actionIndex);
    void infoPluginConfigPagesChanged(int pageIndex);
};

/**
 * A page used if an error occurred trying to load a plugin's config page.
 */
class ErrorConfigPage :
    public Kate::PluginConfigPage
{
    Q_OBJECT

public:
    explicit ErrorConfigPage(QWidget *parent = 0, const QString &traceback = QString());

public slots:
    virtual void apply() {}
    virtual void reset() {}
    virtual void defaults() {}
};

} // namespace Pate

#endif
