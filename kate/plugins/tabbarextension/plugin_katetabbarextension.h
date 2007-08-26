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
#include <kate/pluginconfigpageinterface.h>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/modificationinterface.h>

#include <klibloader.h>
#include <klocale.h>
#include <ktoolbar.h>

#include <qpushbutton.h>
//Added by qt3to4:
#include <QPixmap>
#include <QBoxLayout>
#include <QMap>
#include <QList>

class QBoxLayout;
class QCheckBox;

class KateTabBarExtension;
class KateTabBarExtensionConfigPage;
class KTinyTabBar;

class PluginView : public Kate::PluginView
{
    Q_OBJECT
    friend class KatePluginTabBarExtension;

public:
    PluginView( Kate::MainWindow* mainwindow );
    virtual ~PluginView();

    void updateLocation();

    void readSessionConfig (KConfig* config, const QString& groupPrefix);
    void writeSessionConfig (KConfig* config, const QString& groupPrefix);

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

class KatePluginTabBarExtension : public Kate::Plugin, public Kate::PluginConfigPageInterface
{
  Q_OBJECT
  Q_INTERFACES(Kate::PluginConfigPageInterface)
  public:
    explicit KatePluginTabBarExtension( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KatePluginTabBarExtension();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    void readSessionConfig (KConfig* config, const QString& groupPrefix);
    void writeSessionConfig (KConfig* config, const QString& groupPrefix);

    // Kate::PluginConfigPageInterface
    uint configPages () const { return 1; }
    Kate::PluginConfigPage *configPage (uint , QWidget *w, const char *name=0);
    QString configPageName(uint) const { return i18n("Tab Bar Extension"); }
    QString configPageFullName(uint) const { return i18n("Configure Tab Bar Extension"); }
    KIcon configPageIcon(uint) const { return KIcon(); }

  public slots:
    void applyConfig( KateTabBarExtensionConfigPage* );

  protected slots:
    void tabbarSettingsChanged( KTinyTabBar* tabbar );
    void tabbarHighlightMarksChanged( KTinyTabBar* tabbar );

  private:
    void initConfigPage( KateTabBarExtensionConfigPage* );

  private:
    QList<PluginView*> m_views;
    KConfig* pConfig;
};


/**
 * The tabbar's config page
 */
class KateTabBarExtensionConfigPage : public Kate::PluginConfigPage
{
  Q_OBJECT

  friend class KatePluginTabBarExtension;

  public:
    explicit KateTabBarExtensionConfigPage (QObject* parent = 0L, QWidget *parentWidget = 0L);
    ~KateTabBarExtensionConfigPage ();

    /**
     * Reimplemented from Kate::PluginConfigPage
     * just emits configPageApplyRequest( this ).
     */
    virtual void apply();

    virtual void reset () { ; }
    virtual void defaults () { ; }

  signals:
    /**
     * Ask the plugin to set initial values
     */
    void configPageApplyRequest( KateTabBarExtensionConfigPage* );

    /**
     * Ask the plugin to apply changes
     */
    void configPageInitRequest( KateTabBarExtensionConfigPage* );

  private:
    QCheckBox* pSortAlpha;
};

#endif // PLUGIN_KATETABBAREXTENSION_H
