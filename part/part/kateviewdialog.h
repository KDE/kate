/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_VIEWDIALOG_H_
#define _KATE_VIEWDIALOG_H_

#include "kateglobal.h"

class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class KColorButton;
class KIntNumInput;
class KComboBox;
class KRegExpDialog;
class KKeyButton;
class KPushButton;
class KMainWindow;
class KAccel;
class KKeyChooser;
class KSpellConfig;

#include <kdialogbase.h>
#include "kateview.h"
#include "katedocument.h"

class SearchDialog : public KDialogBase
{
  Q_OBJECT

  public:
    SearchDialog( QWidget *parent, QStringList &searchFor, QStringList &replaceWith, int flags );
    QString getSearchFor();
    QString getReplaceWith();
    int getFlags();
    void setSearchText( const QString &searchstr );

  protected slots:
    void slotOk();
    void selectedStateChanged (int);
    void slotEditRegExp();

  protected:
    KComboBox *m_search;
    KComboBox *m_replace;
    QCheckBox *m_opt1;
    QCheckBox *m_opt2;
    QCheckBox *m_opt3;
    QCheckBox *m_optRegExp;
    QCheckBox *m_opt4;
    QCheckBox *m_opt5;
    QCheckBox *m_opt6;
    QDialog *m_regExpDialog;
};

class ReplacePrompt : public KDialogBase
{
    Q_OBJECT

  public:

    ReplacePrompt(QWidget *parent);

  signals:

    void clicked();

  protected slots:

    void slotUser1( void ); // All
    void slotUser2( void ); // No
    void slotUser3( void ); // Yes
    virtual void done(int);
};

class GotoLineDialog : public KDialogBase
{
    Q_OBJECT

  public:

    GotoLineDialog(QWidget *parent, int line, int max);
    int getLine();

  protected:

    KIntNumInput *e1;
    QPushButton *btnOK;
};

class IndentConfigTab : public Kate::ConfigPage
{
    Q_OBJECT

  public:

    IndentConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);

  protected:
    KateDocument *myDoc;

    static const int numFlags = 6;
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];
    
  public slots:
    void apply ();
    void reload ();
};

class SelectConfigTab : public Kate::ConfigPage
{
    Q_OBJECT

 public:

    SelectConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);
    KateDocument *myDoc;

  protected:

    static const int numFlags = 5;
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];

  public slots:
    void apply ();
    void reload ();
};

class EditConfigTab : public Kate::ConfigPage
{
    Q_OBJECT

  public:

    EditConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);

  protected:

    static const int numFlags = 9;
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];

    KIntNumInput *e1;
    KIntNumInput *e2;
    KIntNumInput *e3;
    KateDocument *myDoc;
  
  public slots:
    void apply ();
    void reload ();
};

class ColorConfig : public Kate::ConfigPage
{
  Q_OBJECT

public:

  ColorConfig( QWidget *parent = 0, char *name = 0, KateDocument *doc=0 );
  ~ColorConfig();

  void setColors( QColor * );
  void getColors( QColor * );

private:
  KateDocument *myDoc;

  KColorButton *m_back;
  KColorButton *m_selected;

  public slots:
    void apply ();
    void reload ();
};

class FontConfig : public Kate::ConfigPage
{
  Q_OBJECT

public:

  FontConfig( QWidget *parent = 0, char *name = 0, KateDocument *doc=0 );
  ~FontConfig();

  void setFont ( const QFont &font );
  QFont getFont ( ) { return myFont; };
  
  void setFontPrint ( const QFont &font );
  QFont getFontPrint ( ) { return myFontPrint; };

  private:
    class KFontChooser *m_fontchooser;
    class KFontChooser *m_fontchooserPrint;
    QFont myFont;
    QFont myFontPrint;
    KateDocument *myDoc;

  private slots:
    void slotFontSelected( const QFont &font );
    void slotFontSelectedPrint ( const QFont &font );

  public slots:
    void apply ();
    void reload ();
};


class EditKeyConfiguration: public Kate::ConfigPage
{
	Q_OBJECT
public:
	EditKeyConfiguration(QWidget *parent=0, char *name=0);
	~EditKeyConfiguration();
	void save();
private:
	KMainWindow *tmpWin;
	KAccel *m_editAccels;
	KKeyChooser *chooser;
	void setupEditKeys();
protected slots:
	void dummy();
  
  public slots:
    void apply ();
    void reload ();
};


class KSpellConfigPage : public Kate::ConfigPage
{
  Q_OBJECT

  public:
    KSpellConfigPage (QWidget *parent, KateDocument *doc);
    ~KSpellConfigPage ();

  private:
    KateDocument *myDoc;
    class KSpellConfig *ksc;

  public slots:
    void apply ();
    void reload ();
};

#endif
