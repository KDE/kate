 /***************************************************************************
                          plugin_kateinsertcommand.h  -  description
                             -------------------
    begin                : THU Apr 19 2001
    copyright            : (C) 2001 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PLUGIN_KATE_INSERT_COMMAND_H_
#define _PLUGIN_KATE_INSERT_COMMAND_H_

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>
#include <kate/pluginconfiginterface.h>
#include <kate/pluginconfiginterfaceextension.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/view.h>

#include <kcombobox.h>
#include <kdialogbase.h>
#include <klibloader.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <qcheckbox.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3PtrList>


class InsertCommandConfigPage;
namespace Kate {
  class PluginConfigPage;
  class PluginView;
}
class KConfig;
class K3Process;
class K3ShellProcess;
class Q3ButtonGroup;
class QCheckBox;
class QSpinBox;
class QStringList;
class WaitDlg;

class PluginKateInsertCommand : public Kate::Plugin,
                                Kate::PluginViewInterface,
                                Kate::PluginConfigInterfaceExtension
{
  Q_OBJECT

  public:
    explicit PluginKateInsertCommand( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );
    virtual ~PluginKateInsertCommand();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

    KTextEditor::View *kv;

    WaitDlg *wdlg;
    Q3PtrList<class PluginView> m_views;

    uint configPages () const { return 1; }
    Kate::PluginConfigPage *configPage (uint , QWidget *w, const char *name=0);
    QString configPageName(uint) const { return i18n("Insert Command"); };
    QString configPageFullName(uint) const {
      return i18n("Configure Insert Command Plugin"); };
    QPixmap configPagePixmap (uint /*number = 0*/,
                              int /*size = KIconLoader::SizeSmall*/) const
                               { return 0L; };

  private:
    void initConfigPage( InsertCommandConfigPage* );

    K3ShellProcess *sh;
    QString workingdir;
    QString cmd;
    QStringList cmdhist;
    bool bInsStdErr;
    int dialogSettings;
    KConfig *config;

  public slots:
    void slotInsertCommand();
    void slotAbort();
    void applyConfig( InsertCommandConfigPage* );

  private slots:
    void slotReceivedStdout(K3Process*, char*, int);
    void slotReceivedStderr(K3Process*, char*, int);
    void slotProcessExited(K3Process*);
    void slotShowWaitDlg();
};

class CmdPrompt : public KDialogBase
{
Q_OBJECT
public:
    explicit CmdPrompt(QWidget* parent=0,
	              const char* name=0,
    		      const QStringList& cmdhist=QStringList(),
	              const QString& dir=QString(),
	              const QString& docdir=QString(),
	              int   settings=0);
    ~CmdPrompt();
    QString command()const { return cmb_cmd->currentText(); }
    bool insertStdErr()const { return cb_insStdErr->isChecked(); }
    bool printCmd()const { return cb_printCmd->isChecked(); }
    QString wd()const { return wdreq->url(); }
private slots:
    void slotTextChanged(const QString &text);


  private:
    KHistoryCombo *cmb_cmd;
    KUrlRequester *wdreq;
    QCheckBox *cb_insStdErr;
    QCheckBox *cb_printCmd;

};

class WaitDlg : public KDialogBase
{
  public:
    explicit WaitDlg(QWidget* parent,
	            const QString& text=QString(),
    		    const QString& title=i18n("Please Wait"));
    ~WaitDlg();
};

/** Config page for the plugin. */
class InsertCommandConfigPage : public Kate::PluginConfigPage
{
  Q_OBJECT
  friend class PluginKateInsertCommand;

  public:
    explicit InsertCommandConfigPage(QObject* parent = 0L, QWidget *parentWidget = 0L);
    ~InsertCommandConfigPage() {};

    /** Reimplemented from Kate::PluginConfigPage
     * just emits configPageApplyRequest( this ).
     */
    void apply();

    void reset () { ; };
    void defaults () { ; };

  signals:
    /** Ask the plugin to set initial values */
    void configPageApplyRequest( InsertCommandConfigPage* );
    /** Ask the plugin to apply changes */
    void configPageInitRequest( InsertCommandConfigPage* );

  protected:
    QSpinBox *sb_cmdhistlen;
    //QCheckBox *cb_startindocdir;
    Q3ButtonGroup *rg_startin;
};

#endif // _PLUGIN_KATE_INSERT_COMMAND_H_
// kate: space-indent on; indent-width 2; replace-tabs on;
