/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001-2002 Michael Goffioul <goffioul@imec.be>
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

// $Id:$
 
#include "kateprintsettings.h"

#include <kcolorbutton.h>
#include <kdialog.h> // for spacingHint()
#include <kfontdialog.h>
#include <klocale.h>

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qwhatsthis.h>

//BEGIN KatePrintTextSettings
KatePrintTextSettings::KatePrintTextSettings( KPrinter */*printer*/, QWidget *parent, const char *name )
  : KPrintDialogPage( parent, name )
{
  setTitle( i18n("Te&xt Settings") );
  
  QVBoxLayout *lo = new QVBoxLayout ( this );
  lo->setSpacing( KDialog::spacingHint() );
  
  cbSelection = new QCheckBox( i18n("Print &Selected Text Only"), this );
  lo->addWidget( cbSelection );
  
  cbLineNumbers = new QCheckBox( i18n("Print &Line Numbers"), this );
  lo->addWidget( cbLineNumbers );
  
  cbGuide = new QCheckBox( i18n("Print Syntax &Guide"), this );
  lo->addWidget( cbGuide ); 

  lo->addStretch( 1 );

  // set defaults - nothing to do :-)  
  
  // whatsthis
  QWhatsThis::add( cbSelection, i18n(
        "<p>This option is only available if some text is selected in the document.</p>"
        "<p>If available and enabled, only the selected text is printed.</p>") );
  QWhatsThis::add( cbLineNumbers, i18n(
        "<p>If enabled, line numbers will be printed on the left side of the page(s).</p>") );
  QWhatsThis::add( cbGuide, i18n(
        "<p>Print a box displaying typographical conventions for the document type, as "
        "defined by the syntax highlightning beeing used.") );
}

void KatePrintTextSettings::getOptions( QMap<QString,QString>& opts, bool )
{
  opts["app-kate-printselection"] = cbSelection->isChecked() ? "true" : "false";
  opts["app-kate-printlinenumbers"] = cbLineNumbers->isChecked() ? "true" : "false";
  opts["app-kate-printguide"] = cbGuide->isChecked() ? "true" : "false" ;
}

void KatePrintTextSettings::setOptions( const QMap<QString,QString>& opts )
{
  QString v;
  v = opts["app-kate-printselection"];
  if ( ! v.isEmpty() )
    cbSelection->setChecked( v == "true" );
  v = opts["app-kate-printlinenumbers"];
  if ( ! v.isEmpty() )
    cbLineNumbers->setChecked( v == "true" );
  v = opts["app-kate-printguide"];
  if ( ! v.isEmpty() )
    cbGuide->setChecked( v == "true" );
}

void KatePrintTextSettings::enableSelection( bool enable )
{
  cbSelection->setEnabled( enable );
}

//END KatePrintTextSettings

//BEGIN KatePrintHeaderFooter
KatePrintHeaderFooter::KatePrintHeaderFooter( KPrinter */*printer*/, QWidget *parent, const char *name )
  : KPrintDialogPage( parent, name )
{
  setTitle( i18n("Hea&der && Footer") );
  
  QVBoxLayout *lo = new QVBoxLayout ( this );
  uint sp = KDialog::spacingHint();
  lo->setSpacing( sp );
  
  // enable
  QHBoxLayout *lo1 = new QHBoxLayout ( lo );
  cbEnableHeader = new QCheckBox( i18n("Pr&int Header"), this );
  lo1->addWidget( cbEnableHeader );
  cbEnableFooter = new QCheckBox( i18n("Pri&nt Footer"), this );
  lo1->addWidget( cbEnableFooter );
  
  // font
  QHBoxLayout *lo2 = new QHBoxLayout( lo );
  lo2->addWidget( new QLabel( i18n("Header/Footer Font:"), this ) );
  lFontPreview = new QLabel( this );
  lFontPreview->setFrameStyle( QFrame::Panel|QFrame::Sunken );
  lo2->addWidget( lFontPreview );
  lo2->setStretchFactor( lFontPreview, 1 );
  QPushButton *btnChooseFont = new QPushButton( i18n("Choo&se Font..."), this );
  lo2->addWidget( btnChooseFont );
  connect( btnChooseFont, SIGNAL(clicked()), this, SLOT(setHFFont()) );
  // header
  gbHeader = new QGroupBox( 2, Qt::Horizontal, i18n("Header Properties"), this );
  lo->addWidget( gbHeader );
  
  QLabel *lHeaderFormat = new QLabel( i18n("&Format:"), gbHeader );
  QHBox *hbHeaderFormat = new QHBox( gbHeader );
  hbHeaderFormat->setSpacing( sp );
  leHeaderLeft = new QLineEdit( hbHeaderFormat );
  leHeaderCenter = new QLineEdit( hbHeaderFormat );
  leHeaderRight = new QLineEdit( hbHeaderFormat );
  lHeaderFormat->setBuddy( leHeaderLeft );
  new QLabel( i18n("Colors:"), gbHeader );
  QHBox *hbHeaderColors = new QHBox( gbHeader );
  hbHeaderColors->setSpacing( sp );
  QLabel *lHeaderFgCol = new QLabel( i18n("Foreground:"), hbHeaderColors );
  kcbtnHeaderFg = new KColorButton( hbHeaderColors );
  lHeaderFgCol->setBuddy( kcbtnHeaderFg ); 
  cbHeaderEnableBgColor = new QCheckBox( i18n("Bac&kground"), hbHeaderColors );
  kcbtnHeaderBg = new KColorButton( hbHeaderColors );
  
  gbFooter = new QGroupBox( 2, Qt::Horizontal, i18n("Footer Properties"), this );
  lo->addWidget( gbFooter );

  // footer  
  QLabel *lFooterFormat = new QLabel( i18n("For&mat:"), gbFooter );
  QHBox *hbFooterFormat = new QHBox( gbFooter );
  hbFooterFormat->setSpacing( sp );
  leFooterLeft = new QLineEdit( hbFooterFormat );
  leFooterCenter = new QLineEdit( hbFooterFormat );
  leFooterRight = new QLineEdit( hbFooterFormat );
  lFooterFormat->setBuddy( leFooterLeft );
  
  new QLabel( i18n("Colors:"), gbFooter );
  QHBox *hbFooterColors = new QHBox( gbFooter );
  hbFooterColors->setSpacing( sp );
  QLabel *lFooterBgCol = new QLabel( i18n("Foreground:"), hbFooterColors );
  kcbtnFooterFg = new KColorButton( hbFooterColors );
  lFooterBgCol->setBuddy( kcbtnFooterFg );
  cbFooterEnableBgColor = new QCheckBox( i18n("&Background"), hbFooterColors );
  kcbtnFooterBg = new KColorButton( hbFooterColors );

  lo->addStretch( 1 );
    
  // user friendly 
  connect( cbEnableHeader, SIGNAL(toggled(bool)), gbHeader, SLOT(setEnabled(bool)) );
  connect( cbEnableFooter, SIGNAL(toggled(bool)), gbFooter, SLOT(setEnabled(bool)) );
  connect( cbHeaderEnableBgColor, SIGNAL(toggled(bool)), kcbtnHeaderBg, SLOT(setEnabled(bool)) );
  connect( cbFooterEnableBgColor, SIGNAL(toggled(bool)), kcbtnFooterBg, SLOT(setEnabled(bool)) );
  
  // set defaults
  cbEnableHeader->setChecked( true );
  leHeaderLeft->setText( "%y" );
  leHeaderCenter->setText( "%f" );
  leHeaderRight->setText( "%p" );
  kcbtnHeaderFg->setColor( QColor("black") );
  cbHeaderEnableBgColor->setChecked( true );
  kcbtnHeaderBg->setColor( QColor("lightgrey") );

  cbEnableFooter->setChecked( true );
  leFooterRight->setText( "%U" );
  kcbtnFooterFg->setColor( QColor("black") );
  cbFooterEnableBgColor->setChecked( true );
  kcbtnFooterBg->setColor( QColor("lightgrey") );
    
  // whatsthis
  QString  s = i18n("<p>Format of the page header. The following tags are supported:</p>");
  QString s1 = i18n(
      "<ul><li><tt>%u</tt> : current user name</li>"
      "<li><tt>%d</tt> : complete date/time in short format</li>"
      "<li><tt>%D</tt> : complete date/time in long format</li>"
      "<li><tt>%h</tt> : current time</li>"
      "<li><tt>%y</tt> : current date in short format</li>"
      "<li><tt>%Y</tt> : current date in long format</li>"
      "<li><tt>%f</tt> : file name</li>"
      "<li><tt>%U</tt> : full URL of the document</li>"
      "<li><tt>%p</tt> : page number</li>"
      "</ul><br>"
      "<u>Note:</u> Do <b>not</b> use the '|' (vertical bar) character.");
  QWhatsThis::add(leHeaderRight, s + s1 );
  QWhatsThis::add(leHeaderCenter, s + s1 );
  QWhatsThis::add(leHeaderLeft, s + s1 );
  s = i18n("<p>Format of the page footer. The following tags are supported:</p>");
  QWhatsThis::add(leFooterRight, s + s1 );
  QWhatsThis::add(leFooterCenter, s + s1 );
  QWhatsThis::add(leFooterLeft, s + s1 );


}

void KatePrintHeaderFooter::getOptions(QMap<QString,QString>& opts, bool )
{
  opts["app-kate-hffont"] = strFont;
  
  opts["app-kate-useheader"] = (cbEnableHeader->isChecked() ? "true" : "false");
  opts["app-kate-headerfg"] = kcbtnHeaderFg->color().name();
  opts["app-kate-headerusebg"] = (cbHeaderEnableBgColor->isChecked() ? "true" : "false");
  opts["app-kate-headerbg"] = kcbtnHeaderBg->color().name();
  opts["app-kate-headerformat"] = leHeaderLeft->text() + "|" + leHeaderCenter->text() + "|" + leHeaderRight->text();

  opts["app-kate-usefooter"] = (cbEnableFooter->isChecked() ? "true" : "false");
  opts["app-kate-footerfg"] = kcbtnFooterFg->color().name();
  opts["app-kate-footerusebg"] = (cbFooterEnableBgColor->isChecked() ? "true" : "false");
  opts["app-kate-footerbg"] = kcbtnFooterBg->color().name();
  opts["app-kate-footerformat"] = leFooterLeft->text() + "|" + leFooterCenter->text() + "|" + leFooterRight->text();
}

void KatePrintHeaderFooter::setOptions( const QMap<QString,QString>& opts )
{
  QString v;
  v = opts["app-kate-hffont"];
  strFont = v;
  QFont f = font();
  if ( ! v.isEmpty() )
  {
    f.fromString( strFont );
    lFontPreview->setFont( f );
  }
  lFontPreview->setText( (f.family() + ", %1pt").arg( f.pointSize() ) );
  
  v = opts["app-kate-useheader"];
  if ( ! v.isEmpty() ) 
    cbEnableHeader->setChecked( v == "true" ); 
  v = opts["app-kate-headerfg"];
  if ( ! v.isEmpty() ) 
    kcbtnHeaderFg->setColor( QColor( v ) );
  v = opts["app-kate-headerusebg"];
  if ( ! v.isEmpty() ) 
    cbHeaderEnableBgColor->setChecked( v == "true" );
  v = opts["app-kate-headerbg"];
  if ( ! v.isEmpty() ) 
    kcbtnHeaderBg->setColor( QColor( v ) );

  QStringList tags = QStringList::split('|', opts["app-kate-headerformat"], "true");
  if (tags.count() == 3)
  {
    leHeaderLeft->setText(tags[0]);
    leHeaderCenter->setText(tags[1]);
    leHeaderRight->setText(tags[2]);
  }
  
  v = opts["app-kate-usefooter"];
  if ( ! v.isEmpty() ) 
    cbEnableFooter->setChecked( v == "true" ); 
  v = opts["app-kate-footerfg"];
  if ( ! v.isEmpty() ) 
    kcbtnFooterFg->setColor( QColor( v ) );
  v = opts["app-kate-footerusebg"];
  if ( ! v.isEmpty() ) 
    cbFooterEnableBgColor->setChecked( v == "true" );
  v = opts["app-kate-footerbg"];
  if ( ! v.isEmpty() ) 
    kcbtnFooterBg->setColor( QColor( v ) );
  
  tags = QStringList::split('|', opts["app-kate-footerformat"], "true");
  if (tags.count() == 3)
  {
    leFooterLeft->setText(tags[0]);
    leFooterCenter->setText(tags[1]);
    leFooterRight->setText(tags[2]);
  }
}

void KatePrintHeaderFooter::setHFFont()
{
  QFont fnt( lFontPreview->font() );
  // display a font dialog
  if ( KFontDialog::getFont( fnt, false, this ) == KFontDialog::Accepted )
  {
    // change strFont
    strFont = fnt.toString();
    // set preview
    lFontPreview->setFont( fnt );
    lFontPreview->setText( (fnt.family() + ", %1pt").arg( fnt.pointSize() ) );
  }
}

//END KatePrintHeaderFooter

//BEGIN KatePrintLayout

KatePrintLayout::KatePrintLayout( KPrinter */*printer*/, QWidget *parent, const char *name )
  : KPrintDialogPage( parent, name )
{
  setTitle( i18n("L&ayout") );
  
  QVBoxLayout *lo = new QVBoxLayout ( this );
  lo->setSpacing( KDialog::spacingHint() );
  
  cbDrawBackground = new QCheckBox( i18n("Draw Bac&kground Color"), this );
  lo->addWidget( cbDrawBackground );
  
  cbEnableBox = new QCheckBox( i18n("Draw &Boxes"), this );
  lo->addWidget( cbEnableBox );
  
  gbBoxProps = new QGroupBox( 2, Qt::Horizontal, i18n("Box Properties"), this );
  lo->addWidget( gbBoxProps );
  
  QLabel *lBoxWidth = new QLabel( i18n("W&idth"), gbBoxProps );
  sbBoxWidth = new QSpinBox( 1, 100, 1, gbBoxProps );
  lBoxWidth->setBuddy( sbBoxWidth );
  
  QLabel *lBoxMargin = new QLabel( i18n("&Margin"), gbBoxProps );
  sbBoxMargin = new QSpinBox( 0, 100, 1, gbBoxProps );
  lBoxMargin->setBuddy( sbBoxMargin );
  
  QLabel *lBoxColor = new QLabel( i18n("Co&lor"), gbBoxProps );
  kcbtnBoxColor = new KColorButton( gbBoxProps );
  lBoxColor->setBuddy( kcbtnBoxColor );
  
  connect( cbEnableBox, SIGNAL(toggled(bool)), gbBoxProps, SLOT(setEnabled(bool)) );
  
  lo->addStretch( 1 );
  // set defaults:
  sbBoxMargin->setValue( 6 );
  gbBoxProps->setEnabled( false );
  
  // whatsthis
  QWhatsThis::add( cbDrawBackground, i18n(
        "<p>If enabled, the background color of the editor will be used.</p>"
        "<p>This may be usefull if your color scheme is designed for a dark background, "
        "but may mean a huge waste of bleach.</p>") );
  QWhatsThis::add( cbEnableBox, i18n(
        "<p>If enabled, a box as defined in the properties below will be drawn "
        "around the contents of each page. Header and Footer will be weparated "
        "from contents with a line as well</p>") );
  QWhatsThis::add( sbBoxWidth, i18n(
        "The width of the box outline" ) );
  QWhatsThis::add( sbBoxMargin, i18n(
        "The margin inside boxes in pixels") );
  QWhatsThis::add( kcbtnBoxColor, i18n(
        "The line color to use for boxes") );
}

void KatePrintLayout::getOptions(QMap<QString,QString>& opts, bool )
{
  opts["app-kate-usebackground"] = cbDrawBackground->isChecked() ? "true" : "false";
  opts["app-kate-usebox"] = cbEnableBox->isChecked() ? "true" : "false";
  opts["app-kate-boxwidth"] = sbBoxWidth->cleanText();
  opts["app-kate-boxmargin"] = sbBoxMargin->cleanText();
  opts["app-kate-boxcolor"] = kcbtnBoxColor->color().name();
}

void KatePrintLayout::setOptions( const QMap<QString,QString>& opts )
{
  QString v;
  v = opts["app-kate-usebackground"];
  if ( ! v.isEmpty() )
    cbDrawBackground->setChecked( v == "true" );
  v = opts["app-kate-usebox"];
  if ( ! v.isEmpty() )
    cbEnableBox->setChecked( v == "true" );
  v = opts["app-kate-boxwidth"];
  if ( ! v.isEmpty() )
    sbBoxWidth->setValue( v.toInt() );
  v = opts["app-kate-boxmargin"];
  if ( ! v.isEmpty() )
    sbBoxMargin->setValue( v.toInt() );
  v = opts["app-kate-boxcolor"];
  if ( ! v.isEmpty() )
    kcbtnBoxColor->setColor( QColor( v ) );
}
//END KatePrintLayout

#include "kateprintsettings.moc"
