/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger
   Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>

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
*/


#ifndef PLUGIN_KATEOPENHEADER_H
#define PLUGIN_KATEOPENHEADER_H

#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <ktexteditor/commandinterface.h>

class PluginKateOpenHeader : public Kate::Plugin
{
  Q_OBJECT

  public:
    explicit PluginKateOpenHeader( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~PluginKateOpenHeader();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

  public slots:
    void slotOpenHeader ();
    void tryOpen( const KUrl& url, const QStringList& extensions );
};

class PluginViewKateOpenHeader
  : public Kate::PluginView
  , public Kate::XMLGUIClient
  , public KTextEditor::Command
{
    Q_OBJECT
    public:
        PluginViewKateOpenHeader(PluginKateOpenHeader* plugin, Kate::MainWindow *mainwindow);
        virtual ~PluginViewKateOpenHeader();

        virtual const QStringList &cmds ();
        virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
        virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg);

    private:
        PluginKateOpenHeader* m_plugin;
};

#endif // PLUGIN_KATEOPENHEADER_H
