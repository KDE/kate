/***************************************************************************
                           plugin_katetabbarextension.h
                           -------------------
    begin                : 2004-04-20
    copyright            : (C) 2004-2005 by Dominik Haumann
    email                : dhdev@gmx.de
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

#ifndef PLUGIN_KATETABBAREXTENSION_H
#define PLUGIN_KATETABBAREXTENSION_H

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/modificationinterface.h>

#include <klibloader.h>
#include <klocale.h>
#include <ktoolbar.h>

#include <qpushbutton.h>
#include <QMap>
#include <QList>

class KateTabBarExtension;
class KTinyTabBar;

class PluginView : public Kate::PluginView
{
    Q_OBJECT
    friend class KatePluginTabBarExtension;

public:
    PluginView( Kate::MainWindow* mainwindow );
    virtual ~PluginView();

    void updateLocation();

    void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

public slots:
    void currentTabChanged( int button_id );
    void closeTabRequest( int button_id );
    void slotDocumentCreated( KTextEditor::Document* document );
    void slotDocumentDeleted( KTextEditor::Document* document );
    void slotViewChanged();
    void slotDocumentChanged( KTextEditor::Document* );
    void slotModifiedOnDisc( KTextEditor::Document* document, bool modified,
                             KTextEditor::ModificationInterface::ModifiedOnDiskReason reason );
    void slotNameChanged( KTextEditor::Document* document );

private:
    KTinyTabBar* tabbar;
    QMap<int, KTextEditor::Document*> id2doc;
    QMap<KTextEditor::Document*, int> doc2id;
};

class KatePluginTabBarExtension : public Kate::Plugin
{
  Q_OBJECT

  public:
    explicit KatePluginTabBarExtension( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KatePluginTabBarExtension();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

  protected slots:
    void tabbarSettingsChanged( KTinyTabBar* tabbar );
    void tabbarHighlightMarksChanged( KTinyTabBar* tabbar );

  private:
    QList<PluginView*> m_views;
};

#endif // PLUGIN_KATETABBAREXTENSION_H
