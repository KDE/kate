/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>

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
#include "kateviewhelpers.h"
#include "kateviglobal.h"

#include <ktexteditor/attribute.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/document.h>
#include <ktexteditor/configpage.h>

#include <kdialog.h>
#include <kfiledialog.h>
#include <kmimetype.h>

#include <sonnet/configwidget.h>
#include <sonnet/dictionarycombobox.h>

#include <QtCore/QStringList>
#include <QtGui/QColor>
#include <QtGui/QTabWidget>
#include <QtGui/QTreeWidget>

struct syntaxContextData;

class ModeConfigPage;
class KateDocument;
class KateView;
class KatePartPluginInfo;

namespace KIO
{
  class Job;
  class TransferJob;
}

class KComboBox;
class KShortcutsEditor;
class KTemporaryFile;
class KIntNumInput;
class KIntSpinBox;
class KPluginSelector;
class KPluginInfo;
class KProcess;

class QCheckBox;
class QLabel;
class QCheckBox;
class QKeyEvent;
class QTableWidget;

namespace Ui
{
  class ModOnHdWidget;
  class TextareaAppearanceConfigWidget;
  class BordersAppearanceConfigWidget;
  class NavigationConfigWidget;
  class EditConfigWidget;
  class IndentationConfigWidget;
  class OpenSaveConfigWidget;
  class OpenSaveConfigAdvWidget;
  class CompletionConfigTab;
  class ViInputModeConfigWidget;
  class SpellCheckConfigWidget;
}

class KateConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT

  public:
    explicit KateConfigPage ( QWidget *parent=0, const char *name=0 );
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

class KateGotoBar : public KateViewBarWidget
{
  Q_OBJECT

  public:
    explicit KateGotoBar(KTextEditor::View *view, QWidget *parent = 0);

    void updateData();

  protected Q_SLOTS:
    void gotoLine();

  protected:
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    KTextEditor::View *const m_view;
    KIntSpinBox *gotoRange;
};

class KateDictionaryBar : public KateViewBarWidget
{
  Q_OBJECT

  public:
    explicit KateDictionaryBar(KateView *view, QWidget *parent = NULL);
    virtual ~KateDictionaryBar();

  public Q_SLOTS:
    void updateData();

  protected Q_SLOTS:
    void dictionaryChanged(const QString& dictionary);

  private:
    KateView* m_view;
    Sonnet::DictionaryComboBox *m_dictionaryComboBox;
};

class KateIndentConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateIndentConfigTab(QWidget *parent);
    ~KateIndentConfigTab();

  protected:
    Ui::IndentationConfigWidget *ui;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}

  private Q_SLOTS:
    void slotChanged ();
    void showWhatsThis(const QString& text);
};

class KateCompletionConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateCompletionConfigTab(QWidget *parent);
    ~KateCompletionConfigTab();

  protected:
    Ui::CompletionConfigTab *ui;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}

  private Q_SLOTS:
    void showWhatsThis(const QString& text);
};

class KateEditGeneralConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateEditGeneralConfigTab(QWidget *parent);
    ~KateEditGeneralConfigTab();

  private:
    Ui::EditConfigWidget *ui;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}
};

class KateNavigationConfigTab : public KateConfigPage
{
  Q_OBJECT

public:
  KateNavigationConfigTab(QWidget *parent);
  ~KateNavigationConfigTab();

private:
  Ui::NavigationConfigWidget *ui;

public Q_SLOTS:
  void apply ();
  void reload ();
  void reset () {}
  void defaults () {}
};

class KateViInputModeConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateViInputModeConfigTab(QWidget *parent);
    ~KateViInputModeConfigTab();

  protected:
    Ui::ViInputModeConfigWidget *ui;

  private:
    void applyTab(QTableWidget *mappingsTable, KateViGlobal::MappingMode mode);
    void reloadTab(QTableWidget *mappingsTable, KateViGlobal::MappingMode mode);

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}

  private Q_SLOTS:
    void showWhatsThis(const QString& text);
    void addMappingRow();
    void removeSelectedMappingRows();
    void importNormalMappingRow();
};

class KateSpellCheckConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateSpellCheckConfigTab(QWidget *parent);
    ~KateSpellCheckConfigTab();

  protected:
    Ui::SpellCheckConfigWidget *ui;
    Sonnet::ConfigWidget *m_sonnetConfigWidget;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset () {}
    void defaults () {}

  private Q_SLOTS:
    void showWhatsThis(const QString& text);
};

class KateEditConfigTab : public KateConfigPage
{
  Q_OBJECT

public:
  KateEditConfigTab(QWidget *parent);
  ~KateEditConfigTab();

public Q_SLOTS:
  void apply ();
  void reload ();
  void reset ();
  void defaults ();

private:
  KateEditGeneralConfigTab *editConfigTab;
  KateNavigationConfigTab *navigationConfigTab;
  KateIndentConfigTab *indentConfigTab;
  KateCompletionConfigTab *completionConfigTab;
  KateViInputModeConfigTab *viInputModeConfigTab;
  KateSpellCheckConfigTab *spellCheckConfigTab;
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
  Ui::TextareaAppearanceConfigWidget *const textareaUi;
  Ui::BordersAppearanceConfigWidget *const bordersUi;
};

class KateSaveConfigTab : public KateConfigPage
{
  Q_OBJECT

  public:
    KateSaveConfigTab( QWidget *parent );
    ~KateSaveConfigTab();

  public Q_SLOTS:
    void apply();
    void reload();
    void reset();
    void defaults();

  protected:
    //why?
    //KComboBox *m_encoding, *m_encodingDetection, *m_eol;
    QCheckBox *cbLocalFiles, *cbRemoteFiles;
    QCheckBox *replaceTabs, *removeSpaces, *allowEolDetection;
    KIntNumInput *dirSearchDepth;
    class KIntSpinBox *blockCount;
    class QLabel *blockCountLabel;

  private:
    Ui::OpenSaveConfigWidget* ui;
    Ui::OpenSaveConfigAdvWidget* uiadv;
    ModeConfigPage *modeConfigPage;
};

class KatePartPluginConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    KatePartPluginConfigPage (QWidget *parent);
    ~KatePartPluginConfigPage ();

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset ();
    void defaults ();

  private:
    KPluginSelector *selector;
    QList<KPluginInfo> plugins;
};


class KateHlDownloadDialog: public KDialog
{
  Q_OBJECT

  public:
    KateHlDownloadDialog(QWidget *parent, const char *name, bool modal);
    ~KateHlDownloadDialog();

  private:
    static unsigned parseVersion(const QString&);
    class QTreeWidget  *list;
    class QString listData;
    KIO::TransferJob *transferJob;

  private Q_SLOTS:
    void listDataReceived(KIO::Job *, const QByteArray &data);

  public Q_SLOTS:
    void slotUser1();
};

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
    void slotDataAvailable(); ///< read data from the process
    void slotPDone(); ///< Runs the diff file when done

  private:
    Ui::ModOnHdWidget* ui;
    KateDocument *m_doc;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason m_modtype;
    KProcess *m_proc;
    KTemporaryFile *m_diffFile;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
