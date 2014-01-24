/*
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
 */

#ifndef _PLUGIN_KATE_FILETEMPLATES_H_
#define _PLUGIN_KATE_FILETEMPLATES_H_

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/view.h>

#include <klocalizedstring.h>
#include <kurl.h>

#include <qlist.h>
#include <qwizard.h>

/**
 * This template system has the following features:
 * It allows to create new documents that already has some contents and a meaningfull
 * document name.
 *
 * Any file can b e used as a template.
 *
 * Special template files can contain macros that are expanded when the document
 * is created, and the cursor can be positioned in the new document.
 *
 * A menu is provided, allowing access to templates located in the KDE file system
 * in the plugins data directory. The menu is dynamically updated.
 *
 * Simple tools are provided for creating/eidting templates.
 *
 * The main class has methods to do all of the work related to use file templates:
 * @li Maintain the Template menu (File, New from Template)
 * @li Load templates
 * @li Provide simple tools for creating/editing templates
*/
class KateFileTemplates : public Kate::Plugin
{
  Q_OBJECT

  public:
    KateFileTemplates( QObject* parent = 0, const QList<QVariant>& dummy=QList<QVariant>());
    virtual ~KateFileTemplates();
    
    virtual Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    /**
     * @return a list of unique group names in the template list.
     */
    QStringList groups();

    /**
     * @return a pointer to the templateinfo collection
     */
    QList<class TemplateInfo*> templates() { return m_templates; }

    /**
     * @return a pointer to the templateInfo for the template at @p index
     * in the template collection.
     */
    class TemplateInfo *templateInfo( int index ) { return m_templates.at( index ); }

    /**
     * @return a a pointer to the active main window
     */
    QWidget * parentWindow();
  
  public Q_SLOTS:
    /**
     * Update the template collection by rereading the template
     * directories. Also updates the menu.
     */
    void updateTemplateDirs(const QString &s=QString());
  protected Q_SLOTS:
    /**
     * Show a file dialog, so that any file can be opened as a template.
     * If the chosen file has the .katetemplate extension, it is parsed,
     * otherwise it is just copied to the new document.
     */
    void slotAny();

    /**
     * Open the template found at @p index in the colletion
     */
    void slotOpenTemplate();

    /**
     * Open the file at @p url as a template. If it has the .katetemplate
     * extension it is parsed, otherwise its content is just copied to the new
     * document.
     */
    void slotOpenTemplate( const KUrl &url );

    void slotEditTemplate();

    /**
     * Show a KateTemplateWizard wizard.
     */
    void slotCreateTemplate();

  Q_SIGNALS:
    void triggerMenuRefresh();
  public:
    void refreshMenu( class KMenu */*class QPopupMenu **/ );

  private:
    class KAction *mActionAny;
    QList<class TemplateInfo*> m_templates;
    class KDirWatch *m_dw;
    class KUser *m_user;
    class KConfig *m_emailstuff;
    class KActionMenu *m_menu;
};

class TemplateInfo;

/**
 * This widget provide a GUI for editing template properties.
 */
class KateTemplateInfoWidget : public QWidget
{
  Q_OBJECT
  public:
    explicit KateTemplateInfoWidget( QWidget *parent=0, TemplateInfo *info=0, KateFileTemplates *kft=0 );
    ~KateTemplateInfoWidget() {}

    TemplateInfo *info;

    class KLineEdit *leTemplate, *leDocumentName, *leDescription, *leAuthor;
    class KComboBox *cmbGroup;
    class QPushButton *btnHighlight;
    class KIconButton *ibIcon;
    class QString highlightName;

  private Q_SLOTS:
    void slotHlSet( class QAction *action );

  private:
    KateFileTemplates *kft;
};

/**
  * This wizard helps creating a new template, which is then opened for the user
  * to edit.
  * Basically, the user is offered to select an existing file or template to start
  * from, set template properties, and if a file is loaded, some replacements is
  * done in the text:
  * @li % characters are protected (% -> %%)
  * @li ^ characters are protected (with a backsplash)
  * @li The users name, username and email is replaced by the corresponding macros
  * If so chosen, the file is saved to either the template directory, or a location
  * set by the user.
*/
class KateTemplateWizard : public QWizard
{
  friend class KateFileTemplates;
  Q_OBJECT
  public:
    KateTemplateWizard( QWidget* parent, KateFileTemplates *ktf );
    ~KateTemplateWizard() {}

   virtual int nextId() const;

  public Q_SLOTS:
    void accept();

  private Q_SLOTS:
    void slotTmplateSet( class QAction* );
    void slotStateChanged();
    void slotStateChanged( int ) { slotStateChanged(); }
    void slotStateChanged( const QString& ) { slotStateChanged(); }

  private:
    KateFileTemplates *kft;
    KateTemplateInfoWidget *kti;

    // origin page
    class QButtonGroup *bgOrigin;
    class KUrlRequester *urOrigin;
    class QPushButton *btnTmpl;
    uint selectedTemplateIdx;

    // location page
    class QButtonGroup *bgLocation;
    class KUrlRequester *urLocation;
    class KLineEdit *leTemplateFileName;

    // macro replacement page
    class QCheckBox *cbRRealname, *cbRUsername, *cbREmail;
    QString sFullname, sEmail/*, sUsername*/;

    // final
    class QCheckBox *cbOpenTemplate;
};

class KateTemplateManager : public QWidget
{
  Q_OBJECT
  public:
    explicit KateTemplateManager( KateFileTemplates *kft=0, QWidget *parent=0 );
    ~KateTemplateManager() {}

  public Q_SLOTS:
    void apply();
    void reload();
    void reset() { reload(); }

  private Q_SLOTS:
    void slotUpdateState();
    void slotEditTemplate();
    void slotRemoveTemplate();

  private:
    class QTreeWidget *lvTemplates;
    class QPushButton *btnNew, *btnEdit, *btnRemove;
    KateFileTemplates *kft;
//     QList<class TemplateInfo> *remove;

};

class PluginViewKateFileTemplates : public Kate::PluginView, public Kate::XMLGUIClient
{
  Q_OBJECT
  public:
    PluginViewKateFileTemplates(KateFileTemplates *plugin,Kate::MainWindow* mainwindow);
    virtual ~PluginViewKateFileTemplates();
  public Q_SLOTS:
    void refreshMenu();
  private:
    KateFileTemplates *m_plugin;
};

#endif // _PLUGIN_KATE_FILETEMPLATES_H_
// kate: space-indent on; indent-width 2; replace-tabs on;
