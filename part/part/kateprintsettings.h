/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2002 Michael Goffioul <goffioul@imec.be>
 *  Complete rewrite on Sat Jun 15 2002 (c) Anders Lund <anders@alweb.dk>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/
 
#ifndef KATEPRINTSETTINGS_H
#define KATEPRINTSETTINGS_H

#include <kprintdialogpage.h>

class KPrinter;
class QCheckBox;
class KColorButton;
class QSpinBox;
class QLineEdit;
class QGroupBox;
class QLabel;

//BEGIN Text settings
/*
  Text settings page:
  - [ ] Print Selection (enabled if there is a selection in the view)
  - Print Line Numbers
    () Smart () Yes () No
*/
class KatePrintTextSettings : public KPrintDialogPage
{
  Q_OBJECT
  public:
    KatePrintTextSettings( KPrinter *printer, QWidget *parent=0, const char *name=0 );
    ~KatePrintTextSettings(){};

    void getOptions(QMap<QString,QString>& opts, bool incldef = false);
    void setOptions(const QMap<QString,QString>& opts);
    
    /* call if view has a selection, enables the seelction checkbox according to the arg */
    void enableSelection( bool );
  
  private:
    QCheckBox *cbSelection, *cbLineNumbers, *cbGuide;
};
//END Text Settings

//BEGIN Header/Footer
/*
  Header & Footer page:
  - enable header/footer
  - header/footer props
    o formats
    0 colors
*/

class KatePrintHeaderFooter : public KPrintDialogPage
{
  Q_OBJECT
  public:
    KatePrintHeaderFooter( KPrinter *printer, QWidget *parent=0, const char *name=0 );
    ~KatePrintHeaderFooter(){};

    void getOptions(QMap<QString,QString>& opts, bool incldef = false);
    void setOptions(const QMap<QString,QString>& opts);

  public slots:  
    void setHFFont();
        
  private:
    QCheckBox *cbEnableHeader, *cbEnableFooter;
    QLabel *lFontPreview;
    QString strFont;
    QGroupBox *gbHeader, *gbFooter;
    QLineEdit *leHeaderLeft, *leHeaderCenter, *leHeaderRight;
    KColorButton *kcbtnHeaderFg, *kcbtnHeaderBg;
    QCheckBox *cbHeaderEnableBgColor;
    QLineEdit *leFooterLeft, *leFooterCenter, *leFooterRight;
    KColorButton *kcbtnFooterFg, *kcbtnFooterBg;
    QCheckBox *cbFooterEnableBgColor;    
};

//END Header/Footer

//BEGIN Layout
/*
  Layout page:
  - Use Box
  - Box properties
    o Width
    o Margin
    o Color
*/
class KatePrintLayout : public KPrintDialogPage
{
  Q_OBJECT
  public:
    KatePrintLayout( KPrinter *printer, QWidget *parent=0, const char *name=0 );
    ~KatePrintLayout(){};

    void getOptions(QMap<QString,QString>& opts, bool incldef = false);
    void setOptions(const QMap<QString,QString>& opts);
  
  private:
    QCheckBox *cbEnableBox, *cbDrawBackground;
    QGroupBox *gbBoxProps;
    QSpinBox *sbBoxWidth, *sbBoxMargin;
    KColorButton* kcbtnBoxColor;
};
//END Layout

#endif
