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

#include <ktexteditor/plugin.h>
#include <ktexteditor/mainwindow.h>
#include <KTextEditor/Command>
#include <KXMLGUIClient>
#include <kpluginfactory.h>
#include <QObject>
#include <QUrl>

class PluginKateOpenHeader : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit PluginKateOpenHeader( QObject* parent = nullptr, const QList<QVariant>& = QList<QVariant>() );
    ~PluginKateOpenHeader() override;

    QObject *createView (KTextEditor::MainWindow *mainWindow) override;

  public Q_SLOTS:
    void slotOpenHeader ();
    void tryOpen( const QUrl& url, const QStringList& extensions );
    bool tryOpenInternal( const QUrl& url, const QStringList& extensions );
  private:
    bool fileExists(const QUrl &url);    
    void setFileName(QUrl *url,const QString &_txt);
};

class PluginViewKateOpenHeader
  : public KTextEditor::Command
  , public KXMLGUIClient
{
    Q_OBJECT
    public:
        PluginViewKateOpenHeader(PluginKateOpenHeader* plugin, KTextEditor::MainWindow *mainwindow);
        ~PluginViewKateOpenHeader() override;

        bool exec (KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
        bool help (KTextEditor::View *view, const QString &cmd, QString &msg) override;

    private:
        PluginKateOpenHeader* m_plugin;
        KTextEditor::MainWindow *m_mainWindow;
};

#endif // PLUGIN_KATEOPENHEADER_H
