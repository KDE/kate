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

#include <klibloader.h>
#include <klocale.h>
#include <kurl.h>
#include <k3wizard.h>

#include <q3ptrlist.h>

class KatePluginFactory : public KLibFactory
{
  Q_OBJECT

  public:
    KatePluginFactory();
    virtual ~KatePluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const QStringList &args = QStringList() );

  private:
    KComponentData m_componentData;
};

/**
 * This template system has the following features:
 * It allows to create new documents that allready has some contents and a meaningfull
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
class KateFileTemplates : public Kate::Plugin, public Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    KateFileTemplates( QObject* parent = 0, const char* name = 0 );
    virtual ~KateFileTemplates();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

    /**
     * @return a list of unique group names in the template list.
     */
    QStringList groups();

    /**
     * @return a pointer to the templateinfo collection
     */
    Q3PtrList<class TemplateInfo> templates() { return m_templates; };

    /**
     * @return a pointer to the templateInfo for the template at @p index
     * in the template collection.
     */
    class TemplateInfo *templateInfo( int index ) { return m_templates.at( index ); }

    /**
     * @return a a pointer to the active main window
     */
    QWidget * parentWindow();

    /**
     * Update the template collection by rereading the template
     * directories. Also updates the menu.
     */
    void updateTemplateDirs(const QString &s=QString::null);

  private slots:
    /**
     * Show a file dialog, so that any file can be opened as a template.
     * If the chosen file has the .katetemplate extension, it is parsed,
     * otherwise it is just copied to the new document.
     */
    void slotAny();

    /**
     * Open the template found at @p index in the colletion
     */
    void slotOpenTemplate( int index );

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

  private:
    void refreshMenu( class PluginView */*class QPopupMenu **/ );

    Q3PtrList<class PluginView> m_views;
    class KActionCollection *m_actionCollection;
    class KRecentFilesAction *m_acRecentTemplates;
    Q3PtrList<class TemplateInfo> m_templates;
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
    KateTemplateInfoWidget( QWidget *parent=0, TemplateInfo *info=0, KateFileTemplates *kft=0 );
    ~KateTemplateInfoWidget() {};

    TemplateInfo *info;

    class QLineEdit *leTemplate, *leDocumentName, *leDescription, *leAuthor;
    class QComboBox *cmbGroup;
    class QPushButton *btnHighlight;
    class KIconButton *ibIcon;

  private slots:
    void slotHlSet( int id );

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
class KateTemplateWizard : public K3Wizard
{
  friend class KateFileTemplates;
  Q_OBJECT
  public:
    KateTemplateWizard( QWidget* parent, KateFileTemplates *ktf );
    ~KateTemplateWizard() {};

  public slots:
    void accept();

  private slots:
    void slotTmplateSet( int );
    void slotStateChanged();
    void slotStateChanged( int ) { slotStateChanged(); }
    void slotStateChanged( const QString& ) { slotStateChanged(); }

  private:
    KateFileTemplates *kft;
    KateTemplateInfoWidget *kti;

    // origin page
    class Q3ButtonGroup *bgOrigin;
    class KUrlRequester *urOrigin;
    class QPushButton *btnTmpl;
    uint selectedTemplateIdx;

    // location page
    class Q3ButtonGroup *bgLocation;
    class KUrlRequester *urLocation;
    class QLineEdit *leTemplateFileName;

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
    KateTemplateManager( KateFileTemplates *kft=0, QWidget *parent=0, const char *name=0 );
    ~KateTemplateManager() {};

  public slots:
    void apply();
    void reload();
    void reset() { reload(); };

  private slots:
    void slotUpload();
    void slotDownload();
    void slotUpdateState();
    void slotEditTemplate();
    void slotRemoveTemplate();

  private:
    class K3ListView *lvTemplates;
    class QPushButton *btnNew, *btnEdit, *btnRemove, *btnDownload, *btnUpload;
    KateFileTemplates *kft;
    Q3PtrList<class TemplateInfo> *remove;

};

#endif // _PLUGIN_KATE_FILETEMPLATES_H_
// kate: space-indent on; indent-width 2; replace-tabs on;
