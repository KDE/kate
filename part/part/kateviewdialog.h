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

#include "katesearch.h"
#include "../interfaces/document.h"

#include <kdialogbase.h>

class KAccel;
class KColorButton;
class KComboBox;
class KIntNumInput;
class KKeyButton;
class KKeyChooser;
class KMainWindow;
class KPushButton;
class KRegExpDialog;

class QButtonGroup;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;

class KateDocument;
class KateView;

class SearchDialog : public KDialogBase
{
  Q_OBJECT

  public:
    SearchDialog( QWidget *parent, QStringList &searchFor,
                  QStringList &replaceWith, SearchFlags flags );
    QString getSearchFor();
    QString getReplaceWith();
    SearchFlags getFlags();
    void setSearchText( const QString &searchstr );

  protected slots:
    void slotOk();
    void selectedStateChanged (int);
    void slotEditRegExp();
    void slotSearchTextChanged( const QString & );

  protected:
    KComboBox *m_search;
    KComboBox *m_replace;
    QPushButton* regexpButton;
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
    KateDocument *m_doc;

    enum { numFlags = 6 };
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];

  public slots:
    void apply ();
    void reload ();
        void reset () {};
    void defaults () {};
};

class SelectConfigTab : public Kate::ConfigPage
{
    Q_OBJECT

 public:

    SelectConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);
    KateDocument *m_doc;

  protected:

    enum { numFlags = 2 };
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];

  public slots:
    void apply ();
    void reload ();
        void reset () {};
    void defaults () {};
};

class EditConfigTab : public Kate::ConfigPage
{
    Q_OBJECT

  public:

    EditConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);

  protected:

    enum { numFlags = 7 };
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];

    KIntNumInput *e1;
    KIntNumInput *e2;
    KIntNumInput *e3;
    KateDocument *m_doc;

  public slots:
    void apply ();
    void reload ();
        void reset () {};
    void defaults () {};

  protected slots:
    void wordWrapToggled();
};

class ViewDefaultsConfig : public Kate::ConfigPage
{
  Q_OBJECT

public:
  ViewDefaultsConfig( QWidget *parent = 0, const char *name = 0, KateDocument *doc=0 );
  ~ViewDefaultsConfig();

private:
  KateDocument *m_doc;

  QCheckBox *m_line;
  QCheckBox *m_folding;
  QCheckBox *m_icons;
  QButtonGroup *m_bmSort;

public slots:
 void apply ();
 void reload ();
 void reset ();
 void defaults ();
};

class ColorConfig : public Kate::ConfigPage
{
  Q_OBJECT

public:

  ColorConfig( QWidget *parent = 0, const char *name = 0, KateDocument *doc=0 );
  ~ColorConfig();

  void setColors( QColor * );
  void getColors( QColor * );

private:
  KateDocument *m_doc;

  KColorButton *m_back;
  KColorButton *m_selected;
  KColorButton *m_current;
  KColorButton *m_bracket;

  public slots:
    void apply ();
    void reload ();
        void reset () {};
    void defaults () {};
};

class FontConfig : public Kate::ConfigPage
{
  Q_OBJECT

public:

  FontConfig( QWidget *parent = 0, const char *name = 0, KateDocument *doc=0 );
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
    KateDocument *m_doc;

  private slots:
    void slotFontSelected( const QFont &font );
    void slotFontSelectedPrint ( const QFont &font );

  public slots:
    void apply ();
    void reload ();
        void reset () {};
    void defaults () {};
};


class EditKeyConfiguration: public Kate::ConfigPage
{
  Q_OBJECT

  public:
    EditKeyConfiguration( QWidget* parent, KateDocument* doc );

  public slots:
    void apply();
    void reload()   {};
    void reset()    {};
    void defaults() {};

  private:
    KKeyChooser* m_keyChooser;
};

#endif
