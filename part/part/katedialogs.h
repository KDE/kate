/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>

   Based on work of:
     Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef __KATE_DIALOGS_H__
#define __KATE_DIALOGS_H__

#include "katehighlight.h"
#include <ktexteditor/attribute.h>
#include <ktexteditor/modificationinterface.h>

#include <ktexteditor/document.h>
#include <ktexteditor/configpage.h>

#include <kdialog.h>
#include <kmimetype.h>

#include <qstringlist.h>
#include <qcolor.h>
#include <qtabwidget.h>
#include <QTreeWidget>

class KatePartPluginListItem;

struct syntaxContextData;

class KateDocument;
class KateView;

namespace KIO
{
  class Job;
  class TransferJob;
}

class KColorButton;
class KComboBox;
class KIntNumInput;
class KKeyButton;
class KKeyChooser;
class KMainWindow;
class KPushButton;
class KRegExpDialog;
class KIntNumInput;

class QGroupBox;
class QCheckBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QCheckBox;

namespace Ui
{
  class ModOnHdWidget;
  class AppearanceConfigWidget;
  class CursorConfigWidget;
  class EditConfigWidget;
  class HlConfigWidget;
  class IndentationConfigWidget;
  class OpenSaveConfigWidget;
}

class KateConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT

  public:
    KateConfigPage ( QWidget *parent=0, const char *name=0 );
    virtual ~KateConfigPage ();

  public:
    bool hasChanged () { return m_changed; }

  protected Q_SLOTS:
    void slotChanged();

  private Q_SLOTS:
    void somethingHasChanged ();

  protected:
    bool m_changed;
};

class KateGotoLineDialog : public KDialog
{
  Q_OBJECT

  public:

    KateGotoLineDialog(QWidget *parent, int line, int max);
    int getLine();

  protected:

    KIntNumInput *e1;
    QPushButton *btnOK;
};

class KateIndentConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateIndentConfigTab(QWidget *parent);

  protected:
    Ui::IndentationConfigWidget *ui;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}

  private Q_SLOTS:
    void showWhatsThis(const QString& text);
};

class KateSelectConfigTab : public KateConfigPage
{
  Q_OBJECT

public:
  KateSelectConfigTab(QWidget *parent);

private:
  Ui::CursorConfigWidget *ui;

public Q_SLOTS:
  void apply ();
  void reload ();
  void reset () {}
  void defaults () {}
};

class KateEditConfigTab : public KateConfigPage
{
  Q_OBJECT

public:
  KateEditConfigTab(QWidget *parent);

protected:
  Ui::EditConfigWidget *ui;

public Q_SLOTS:
  void apply ();
  void reload ();
  void reset () {}
  void defaults () {}
};

class KateViewDefaultsConfig : public KateConfigPage
{
  Q_OBJECT

public:
  KateViewDefaultsConfig( QWidget *parent );
  ~KateViewDefaultsConfig();

public Q_SLOTS:
  void apply ();
  void reload ();
  void reset ();
  void defaults ();
  
private:
  Ui::AppearanceConfigWidget *ui;
};

class KateEditKeyConfiguration: public KateConfigPage
{
  Q_OBJECT

  public:
    KateEditKeyConfiguration( QWidget* parent, KateDocument* doc );

  public Q_SLOTS:
    void apply();
    void reload()   {}
    void reset()    {}
    void defaults() {}

  protected:
    void showEvent ( QShowEvent * );

  private:
    bool m_ready;
    class KateDocument *m_doc;
    KKeyChooser* m_keyChooser;
    class KActionCollection *m_ac;
};

class KateSaveConfigTab : public KateConfigPage
{
  Q_OBJECT
public:
  KateSaveConfigTab( QWidget *parent );

public Q_SLOTS:
  void apply();
  void reload();
  void reset();
  void defaults();

protected:
  KComboBox *m_encoding, *m_eol;
  QCheckBox *cbLocalFiles, *cbRemoteFiles;
  QCheckBox *replaceTabs, *removeSpaces, *allowEolDetection;
  QLineEdit *leBuPrefix;
  QLineEdit *leBuSuffix;
  KIntNumInput *dirSearchDepth;
  class QSpinBox *blockCount;
  class QLabel *blockCountLabel;
  
private:
  Ui::OpenSaveConfigWidget* ui;
};

class KatePartPluginListItem;

class KatePartPluginListView : public QTreeWidget
{
  Q_OBJECT

  friend class KatePartPluginListItem;

  public:
    KatePartPluginListView (QWidget *parent = 0);

  Q_SIGNALS:
    void stateChange(KatePartPluginListItem *, bool);

  private:
    void stateChanged(KatePartPluginListItem *, bool);
};

class KatePartPluginConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    KatePartPluginConfigPage (QWidget *parent);
    ~KatePartPluginConfigPage ();

  public Q_SLOTS:
    void apply ();
    void reload () {}
    void reset () {}
    void defaults () {}

  private Q_SLOTS:
    void slotCurrentChanged( QTreeWidgetItem* );
    void slotConfigure();
    void slotStateChanged( KatePartPluginListItem *, bool );

  private:
    KatePartPluginListView *listView;
    QList<KatePartPluginListItem*> m_items;
    class QPushButton *btnConfigure;
};

class KateScriptNewStuff;

class KateScriptConfigPage : public KateConfigPage
{
  Q_OBJECT
  
  public:
    KateScriptConfigPage(QWidget *parent);
    virtual ~KateScriptConfigPage();
  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}
  private:
    KateScriptNewStuff *m_newStuff;
};

class KateHlConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    KateHlConfigPage (QWidget *parent, KateDocument *doc);
    ~KateHlConfigPage ();

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}

  protected Q_SLOTS:
    void hlChanged(int);
    void hlDownload();
    void showMTDlg();

  private:
    void writeback ();

    Ui::HlConfigWidget *ui;

    QHash<int,KateHlData> hlDataDict;
    int m_currentHlData;

    KateDocument *m_doc;
};

class KateHlDownloadDialog: public KDialog
{
  Q_OBJECT

  public:
    KateHlDownloadDialog(QWidget *parent, const char *name, bool modal);
    ~KateHlDownloadDialog();

  private:
    class QTreeWidget  *list;
    class QString listData;
    KIO::TransferJob *transferJob;

  private Q_SLOTS:
    void listDataReceived(KIO::Job *, const QByteArray &data);

  public Q_SLOTS:
    void slotUser1();
};

class KProcIO;
class KProcess;
/**
 * This dialog will prompt the user for what do with a file that is
 * modified on disk.
 * If the file wasn't deleted, it has a 'diff' button, which will create
 * a diff file (uing diff(1)) and launch that using KRun.
 */
class KateModOnHdPrompt : public KDialog
{
  Q_OBJECT
  public:
    enum Status {
      Reload = 1, // 0 is QDialog::Rejected
      Save,
      Overwrite,
      Ignore
    };
    KateModOnHdPrompt( KateDocument *doc,
                       KTextEditor::ModificationInterface::ModifiedOnDiskReason modtype,
                       const QString &reason, QWidget *parent  );
    ~KateModOnHdPrompt();

  public Q_SLOTS:
    /**
     * Show a diff between the document text and the disk file.
     * This will not close the dialog, since we still need a
     * decision from the user.
     */
    void slotDiff();
  
  protected Q_SLOTS:
    virtual void slotButtonClicked(int button);

  private Q_SLOTS:
    void slotPRead(KProcIO*); ///< Read from the diff process
    void slotPDone(KProcess*); ///< Runs the diff file when done

  private:
    Ui::ModOnHdWidget* ui;
    KateDocument *m_doc;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason m_modtype;
    class KTemporaryFile *m_tmpfile; ///< The diff file. Deleted by KRun when the viewer is exited.

};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
