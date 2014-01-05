/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2002-2010 Anders Lund <anders@alweb.dk>
 *
 *  Rewritten based on code of Copyright (c) 2002 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "printconfigwidgets.h"

#include "kateglobal.h"
#include "kateschema.h"

#include <KColorButton>
#include <KComboBox>
#include <KConfigGroup>
#include <KLineEdit>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QBoxLayout>
#include <QCheckBox>
#include <QFontDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QSpinBox>

using namespace KatePrinter;

//BEGIN KatePrintTextSettings
KatePrintTextSettings::KatePrintTextSettings( QWidget *parent )
  : QWidget( parent )
{
  setWindowTitle( i18n("Te&xt Settings") );

  QVBoxLayout *lo = new QVBoxLayout ( this );

  cbLineNumbers = new QCheckBox( i18n("Print line &numbers"), this );
  lo->addWidget( cbLineNumbers );

  cbGuide = new QCheckBox( i18n("Print &legend"), this );
  lo->addWidget( cbGuide );

  lo->addStretch( 1 );

  // set defaults - nothing to do :-)

  // whatsthis
  cbLineNumbers->setWhatsThis(i18n(
        "<p>If enabled, line numbers will be printed on the left side of the page(s).</p>") );
  cbGuide->setWhatsThis(i18n(
        "<p>Print a box displaying typographical conventions for the document type, as "
        "defined by the syntax highlighting being used.</p>") );

  readSettings();
}

KatePrintTextSettings::~KatePrintTextSettings()
{
  writeSettings();
}

bool KatePrintTextSettings::printLineNumbers()
{
  return cbLineNumbers->isChecked();
}

bool KatePrintTextSettings::printGuide()
{
  return cbGuide->isChecked();
}

void KatePrintTextSettings::readSettings()
{
  KSharedConfigPtr config = KSharedConfig::openConfig();
  KConfigGroup printGroup( config, "Kate Print Settings" );

  KConfigGroup textGroup( &printGroup, "Text" );
  bool isLineNumbersChecked = textGroup.readEntry( "LineNumbers", false );
  cbLineNumbers->setChecked( isLineNumbersChecked );

  bool isLegendChecked = textGroup.readEntry( "Legend", false );
  cbGuide->setChecked( isLegendChecked );
}

void KatePrintTextSettings::writeSettings()
{
  KSharedConfigPtr config = KSharedConfig::openConfig();
  KConfigGroup printGroup( config, "Kate Print Settings" );

  KConfigGroup textGroup( &printGroup, "Text" );
  textGroup.writeEntry( "LineNumbers", printLineNumbers() );
  textGroup.writeEntry( "Legend", printGuide() );

  config->sync();
}

//END KatePrintTextSettings

//BEGIN KatePrintHeaderFooter
KatePrintHeaderFooter::KatePrintHeaderFooter( QWidget *parent )
  : QWidget( parent )
{
  setWindowTitle( i18n("Hea&der && Footer") );

  QVBoxLayout *lo = new QVBoxLayout ( this );

  // enable
  QHBoxLayout *lo1 = new QHBoxLayout ();
  lo->addLayout( lo1 );
  cbEnableHeader = new QCheckBox( i18n("Pr&int header"), this );
  lo1->addWidget( cbEnableHeader );
  cbEnableFooter = new QCheckBox( i18n("Pri&nt footer"), this );
  lo1->addWidget( cbEnableFooter );

  // font
  QHBoxLayout *lo2 = new QHBoxLayout();
  lo->addLayout( lo2 );
  lo2->addWidget( new QLabel( i18n("Header/footer font:"), this ) );
  lFontPreview = new QLabel( this );
  lFontPreview->setFrameStyle( QFrame::Panel|QFrame::Sunken );
  lo2->addWidget( lFontPreview );
  lo2->setStretchFactor( lFontPreview, 1 );
  QPushButton *btnChooseFont = new QPushButton( i18n("Choo&se Font..."), this );
  lo2->addWidget( btnChooseFont );
  connect( btnChooseFont, SIGNAL(clicked()), this, SLOT(setHFFont()) );

  // header
  gbHeader = new QGroupBox( this );
  gbHeader->setTitle(i18n("Header Properties"));
  QGridLayout* grid = new QGridLayout(gbHeader);
  lo->addWidget( gbHeader );

  QLabel *lHeaderFormat = new QLabel( i18n("&Format:"), gbHeader );
  grid->addWidget(lHeaderFormat, 0, 0);

  QFrame *hbHeaderFormat = new QFrame( gbHeader );
  QHBoxLayout *layoutFormat = new QHBoxLayout( hbHeaderFormat );
  grid->addWidget(hbHeaderFormat, 0, 1);

  leHeaderLeft = new KLineEdit( hbHeaderFormat );
  layoutFormat->addWidget(leHeaderLeft);
  leHeaderCenter = new KLineEdit( hbHeaderFormat );
  layoutFormat->addWidget(leHeaderCenter);
  leHeaderRight = new KLineEdit( hbHeaderFormat );
  lHeaderFormat->setBuddy( leHeaderLeft );
  layoutFormat->addWidget(leHeaderRight);

  leHeaderLeft->setContextMenuPolicy(Qt::CustomContextMenu);
  leHeaderCenter->setContextMenuPolicy(Qt::CustomContextMenu);
  leHeaderRight->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(leHeaderLeft, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
  connect(leHeaderCenter, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
  connect(leHeaderRight, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

  grid->addWidget(new QLabel( i18n("Colors:"), gbHeader ), 1, 0);

  QFrame *hbHeaderColors = new QFrame( gbHeader );
  QHBoxLayout *layoutColors = new QHBoxLayout( hbHeaderColors );
  layoutColors->setSpacing( -1 );
  grid->addWidget(hbHeaderColors, 1, 1);

  QLabel *lHeaderFgCol = new QLabel( i18n("Foreground:"), hbHeaderColors );
  layoutColors->addWidget(lHeaderFgCol);
  kcbtnHeaderFg = new KColorButton( hbHeaderColors );
  layoutColors->addWidget(kcbtnHeaderFg);
  lHeaderFgCol->setBuddy( kcbtnHeaderFg );
  cbHeaderEnableBgColor = new QCheckBox( i18n("Bac&kground"), hbHeaderColors );
  layoutColors->addWidget(cbHeaderEnableBgColor);
  kcbtnHeaderBg = new KColorButton( hbHeaderColors );
  layoutColors->addWidget(kcbtnHeaderBg);

  gbFooter = new QGroupBox( this );
  gbFooter->setTitle(i18n("Footer Properties"));
  grid = new QGridLayout(gbFooter);
  lo->addWidget( gbFooter );

  // footer
  QLabel *lFooterFormat = new QLabel( i18n("For&mat:"), gbFooter );
  grid->addWidget(lFooterFormat, 0, 0);

  QFrame *hbFooterFormat = new QFrame( gbFooter );
  layoutFormat = new QHBoxLayout( hbFooterFormat );
  layoutFormat->setSpacing( -1 );
  grid->addWidget(hbFooterFormat, 0, 1);

  leFooterLeft = new KLineEdit( hbFooterFormat );
  layoutFormat->addWidget(leFooterLeft);
  leFooterCenter = new KLineEdit( hbFooterFormat );
  layoutFormat->addWidget(leFooterCenter);
  leFooterRight = new KLineEdit( hbFooterFormat );
  layoutFormat->addWidget(leFooterRight);
  lFooterFormat->setBuddy( leFooterLeft );

  leFooterLeft->setContextMenuPolicy(Qt::CustomContextMenu);
  leFooterCenter->setContextMenuPolicy(Qt::CustomContextMenu);
  leFooterRight->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(leFooterLeft, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
  connect(leFooterCenter, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
  connect(leFooterRight, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

  grid->addWidget(new QLabel( i18n("Colors:"), gbFooter ), 1, 0);

  QFrame *hbFooterColors = new QFrame( gbFooter );
  layoutColors = new QHBoxLayout( hbFooterColors );
  layoutColors->setSpacing( -1 );
  grid->addWidget(hbFooterColors, 1, 1);

  QLabel *lFooterBgCol = new QLabel( i18n("Foreground:"), hbFooterColors );
  layoutColors->addWidget(lFooterBgCol);
  kcbtnFooterFg = new KColorButton( hbFooterColors );
  layoutColors->addWidget(kcbtnFooterFg);
  lFooterBgCol->setBuddy( kcbtnFooterFg );
  cbFooterEnableBgColor = new QCheckBox( i18n("&Background"), hbFooterColors );
  layoutColors->addWidget(cbFooterEnableBgColor);
  kcbtnFooterBg = new KColorButton( hbFooterColors );
  layoutColors->addWidget(kcbtnFooterBg);

  lo->addStretch( 1 );

  // user friendly
  connect( cbEnableHeader, SIGNAL(toggled(bool)), gbHeader, SLOT(setEnabled(bool)) );
  connect( cbEnableFooter, SIGNAL(toggled(bool)), gbFooter, SLOT(setEnabled(bool)) );
  connect( cbHeaderEnableBgColor, SIGNAL(toggled(bool)), kcbtnHeaderBg, SLOT(setEnabled(bool)) );
  connect( cbFooterEnableBgColor, SIGNAL(toggled(bool)), kcbtnFooterBg, SLOT(setEnabled(bool)) );

  // set defaults
  cbEnableHeader->setChecked( true );
  leHeaderLeft->setText( QLatin1String("%y") );
  leHeaderCenter->setText( QLatin1String("%f") );
  leHeaderRight->setText( QLatin1String("%p") );
  kcbtnHeaderFg->setColor( Qt::black );
  cbHeaderEnableBgColor->setChecked( false );
  kcbtnHeaderBg->setColor( Qt::lightGray );

  cbEnableFooter->setChecked( true );
  leFooterRight->setText( QLatin1String("%U") );
  kcbtnFooterFg->setColor( Qt::black );
  cbFooterEnableBgColor->setChecked( false );
  kcbtnFooterBg->setColor( Qt::lightGray );

  // whatsthis
  QString  s = i18n("<p>Format of the page header. The following tags are supported:</p>");
  QString s1 = i18n(
      "<ul><li><tt>%u</tt>: current user name</li>"
      "<li><tt>%d</tt>: complete date/time in short format</li>"
      "<li><tt>%D</tt>: complete date/time in long format</li>"
      "<li><tt>%h</tt>: current time</li>"
      "<li><tt>%y</tt>: current date in short format</li>"
      "<li><tt>%Y</tt>: current date in long format</li>"
      "<li><tt>%f</tt>: file name</li>"
      "<li><tt>%U</tt>: full URL of the document</li>"
      "<li><tt>%p</tt>: page number</li>"
      "<li><tt>%P</tt>: total amount of pages</li>"
      "</ul><br />");
  leHeaderRight->setWhatsThis(s + s1 );
  leHeaderCenter->setWhatsThis(s + s1 );
  leHeaderLeft->setWhatsThis(s + s1 );
  s = i18n("<p>Format of the page footer. The following tags are supported:</p>");
  leFooterRight->setWhatsThis(s + s1 );
  leFooterCenter->setWhatsThis(s + s1 );
  leFooterLeft->setWhatsThis(s + s1 );

  readSettings();
}

KatePrintHeaderFooter::~KatePrintHeaderFooter()
{
  writeSettings();
}

QFont KatePrintHeaderFooter::font()
{
    return lFontPreview->font();
}

bool KatePrintHeaderFooter::useHeader()
{
  return cbEnableHeader->isChecked();
}

QStringList KatePrintHeaderFooter::headerFormat()
{
  QStringList l;
  l << leHeaderLeft->text() << leHeaderCenter->text() << leHeaderRight->text();
  return l;
}

QColor KatePrintHeaderFooter::headerForeground()
{
  return kcbtnHeaderFg->color();
}

QColor KatePrintHeaderFooter::headerBackground()
{
  return kcbtnHeaderBg->color();
}

bool KatePrintHeaderFooter::useHeaderBackground()
{
  return cbHeaderEnableBgColor->isChecked();
}

bool KatePrintHeaderFooter::useFooter()
{
  return cbEnableFooter->isChecked();
}

QStringList KatePrintHeaderFooter::footerFormat()
{
  QStringList l;
  l<< leFooterLeft->text() << leFooterCenter->text() << leFooterRight->text();
  return l;
}

QColor KatePrintHeaderFooter::footerForeground()
{
  return kcbtnFooterFg->color();
}

QColor KatePrintHeaderFooter::footerBackground()
{
  return kcbtnFooterBg->color();
}

bool KatePrintHeaderFooter::useFooterBackground()
{
  return cbFooterEnableBgColor->isChecked();
}

void KatePrintHeaderFooter::setHFFont()
{
  bool ok;
  QFont selectedFont = QFontDialog::getFont(&ok, lFontPreview->font(), this);

  if (ok) {
    lFontPreview->setFont(selectedFont);
    QString text = i18n("%1, %pt");
    lFontPreview->setText(text.arg(selectedFont.family()).arg(selectedFont.pointSize()));
  }
}

void KatePrintHeaderFooter::showContextMenu(const QPoint& pos)
{
  QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
  if (!lineEdit) {
    return;
  }

  QMenu* const contextMenu = lineEdit->createStandardContextMenu();
  if (contextMenu == NULL) {
    return;
  }
  contextMenu->addSeparator();

  // create original context menu
  QMenu* menu = contextMenu->addMenu(i18n("Add Placeholder..."));
  menu->setIcon(QIcon::fromTheme(QLatin1String("list-add")));
  QAction* a = menu->addAction(i18n("Current User Name") + QLatin1String("\t%u"));
  a->setData(QLatin1String("%u"));
  a = menu->addAction(i18n("Complete Date/Time (short format)") + QLatin1String("\t%d"));
  a->setData(QLatin1String("%d"));
  a = menu->addAction(i18n("Complete Date/Time (long format)") + QLatin1String("\t%D"));
  a->setData(QLatin1String("%D"));
  a = menu->addAction(i18n("Current Time") + QLatin1String("\t%h"));
  a->setData(QLatin1String("%h"));
  a = menu->addAction(i18n("Current Date (short format)") + QLatin1String("\t%y"));
  a->setData(QLatin1String("%y"));
  a = menu->addAction(i18n("Current Date (long format)") + QLatin1String("\t%Y"));
  a->setData(QLatin1String("%Y"));
  a = menu->addAction(i18n("File Name") + QLatin1String("\t%f"));
  a->setData(QLatin1String("%f"));
  a = menu->addAction(i18n("Full document URL") + QLatin1String("\t%U"));
  a->setData(QLatin1String("%U"));
  a = menu->addAction(i18n("Page Number") + QLatin1String("\t%p"));
  a->setData(QLatin1String("%p"));
  a = menu->addAction(i18n("Total Amount of Pages") + QLatin1String("\t%P"));
  a->setData(QLatin1String("%P"));

  QAction* const result = contextMenu->exec(lineEdit->mapToGlobal(pos));
  if (result) {
    QString placeHolder = result->data().toString();
    if (!placeHolder.isEmpty()) {
      lineEdit->insert(placeHolder);
    }
  }
}

void KatePrintHeaderFooter::readSettings()
{
  KSharedConfigPtr config = KSharedConfig::openConfig();
  KConfigGroup printGroup( config, "Kate Print Settings" );

  // Header
  KConfigGroup headerFooterGroup( &printGroup, "HeaderFooter" );
  bool isHeaderEnabledChecked = headerFooterGroup.readEntry( "HeaderEnabled", true );
  cbEnableHeader->setChecked( isHeaderEnabledChecked );

  QString headerFormatLeft = headerFooterGroup.readEntry( "HeaderFormatLeft", "%y" );
  leHeaderLeft->setText( headerFormatLeft );

  QString headerFormatCenter = headerFooterGroup.readEntry( "HeaderFormatCenter", "%f" );
  leHeaderCenter->setText( headerFormatCenter );

  QString headerFormatRight = headerFooterGroup.readEntry( "HeaderFormatRight", "%p" );
  leHeaderRight->setText( headerFormatRight );

  QColor headerForeground = headerFooterGroup.readEntry( "HeaderForeground", QColor("black") );
  kcbtnHeaderFg->setColor( headerForeground );

  bool isHeaderBackgroundChecked = headerFooterGroup.readEntry( "HeaderBackgroundEnabled", false );
  cbHeaderEnableBgColor->setChecked( isHeaderBackgroundChecked );

  QColor headerBackground = headerFooterGroup.readEntry( "HeaderBackground", QColor("lightgrey") );
  kcbtnHeaderBg->setColor( headerBackground );

  // Footer
  bool isFooterEnabledChecked = headerFooterGroup.readEntry( "FooterEnabled", true );
  cbEnableFooter->setChecked( isFooterEnabledChecked );

  QString footerFormatLeft = headerFooterGroup.readEntry( "FooterFormatLeft", QString() );
  leFooterLeft->setText( footerFormatLeft );

  QString footerFormatCenter = headerFooterGroup.readEntry( "FooterFormatCenter", QString() );
  leFooterCenter->setText( footerFormatCenter );

  QString footerFormatRight = headerFooterGroup.readEntry( "FooterFormatRight", "%U" );
  leFooterRight->setText( footerFormatRight );

  QColor footerForeground = headerFooterGroup.readEntry( "FooterForeground", QColor("black") );
  kcbtnFooterFg->setColor( footerForeground );

  bool isFooterBackgroundChecked = headerFooterGroup.readEntry( "FooterBackgroundEnabled", false );
  cbFooterEnableBgColor->setChecked( isFooterBackgroundChecked );

  QColor footerBackground = headerFooterGroup.readEntry( "FooterBackground", QColor("lightgrey") );
  kcbtnFooterBg->setColor( footerBackground );

  // Font
  QFont headerFooterFont = headerFooterGroup.readEntry( "HeaderFooterFont", QFont() );
  lFontPreview->setFont( headerFooterFont );
  lFontPreview->setText( QString(headerFooterFont.family() + QLatin1String(", %1pt")).arg( headerFooterFont.pointSize() ) );
}

void KatePrintHeaderFooter::writeSettings()
{
  KSharedConfigPtr config = KSharedConfig::openConfig();
  KConfigGroup printGroup( config, "Kate Print Settings" );

  // Header
  KConfigGroup headerFooterGroup( &printGroup, "HeaderFooter" );
  headerFooterGroup.writeEntry( "HeaderEnabled", useHeader() );

  QStringList format = headerFormat();
  headerFooterGroup.writeEntry( "HeaderFormatLeft", format[0] );
  headerFooterGroup.writeEntry( "HeaderFormatCenter", format[1] );
  headerFooterGroup.writeEntry( "HeaderFormatRight", format[2] );
  headerFooterGroup.writeEntry( "HeaderForeground", headerForeground() );
  headerFooterGroup.writeEntry( "HeaderBackgroundEnabled", useHeaderBackground() );
  headerFooterGroup.writeEntry( "HeaderBackground", headerBackground() );

  // Footer
  headerFooterGroup.writeEntry( "FooterEnabled", useFooter() );

  format = footerFormat();
  headerFooterGroup.writeEntry( "FooterFormatLeft", format[0] );
  headerFooterGroup.writeEntry( "FooterFormatCenter", format[1] );
  headerFooterGroup.writeEntry( "FooterFormatRight", format[2] );
  headerFooterGroup.writeEntry( "FooterForeground", footerForeground() );
  headerFooterGroup.writeEntry( "FooterBackgroundEnabled", useFooterBackground() );
  headerFooterGroup.writeEntry( "FooterBackground", footerBackground() );

  // Font
  headerFooterGroup.writeEntry( "HeaderFooterFont", font() );

  config->sync();
}

//END KatePrintHeaderFooter

//BEGIN KatePrintLayout

KatePrintLayout::KatePrintLayout( QWidget *parent)
  : QWidget( parent )
{
  setWindowTitle( i18n("L&ayout") );

  QVBoxLayout *lo = new QVBoxLayout ( this );

  QHBoxLayout *hb = new QHBoxLayout();
  lo->addLayout( hb );
  QLabel *lSchema = new QLabel( i18n("&Schema:"), this );
  hb->addWidget(lSchema);
  cmbSchema = new KComboBox( this );
  hb->addWidget(cmbSchema);
  cmbSchema->setEditable( false );
  lSchema->setBuddy( cmbSchema );

  cbDrawBackground = new QCheckBox( i18n("Draw bac&kground color"), this );
  lo->addWidget( cbDrawBackground );

  cbEnableBox = new QCheckBox( i18n("Draw &boxes"), this );
  lo->addWidget( cbEnableBox );

  gbBoxProps = new QGroupBox( this );
  gbBoxProps->setTitle(i18n("Box Properties"));
  QGridLayout* grid = new QGridLayout(gbBoxProps);
  lo->addWidget( gbBoxProps );

  QLabel *lBoxWidth = new QLabel( i18n("W&idth:"), gbBoxProps );
  grid->addWidget(lBoxWidth, 0, 0);
  sbBoxWidth = new QSpinBox( gbBoxProps );
  sbBoxWidth->setRange( 1, 100 );
  sbBoxWidth->setSingleStep( 1 );
  grid->addWidget(sbBoxWidth, 0, 1);
  lBoxWidth->setBuddy( sbBoxWidth );

  QLabel *lBoxMargin = new QLabel( i18n("&Margin:"), gbBoxProps );
  grid->addWidget(lBoxMargin, 1, 0);
  sbBoxMargin = new QSpinBox( gbBoxProps );
  sbBoxMargin->setRange( 0, 100 );
  sbBoxMargin->setSingleStep( 1 );
  grid->addWidget(sbBoxMargin, 1, 1);
  lBoxMargin->setBuddy( sbBoxMargin );

  QLabel *lBoxColor = new QLabel( i18n("Co&lor:"), gbBoxProps );
  grid->addWidget(lBoxColor, 2, 0);
  kcbtnBoxColor = new KColorButton( gbBoxProps );
  grid->addWidget(kcbtnBoxColor, 2, 1);
  lBoxColor->setBuddy( kcbtnBoxColor );

  connect( cbEnableBox, SIGNAL(toggled(bool)), gbBoxProps, SLOT(setEnabled(bool)) );

  lo->addStretch( 1 );
  // set defaults:
  sbBoxMargin->setValue( 6 );
  gbBoxProps->setEnabled( false );

  Q_FOREACH (const KateSchema &schema, KateGlobal::self()->schemaManager()->list())
    cmbSchema->addItem (schema.translatedName(), QVariant (schema.rawName));

  // default is printing, MUST BE THERE
  cmbSchema->setCurrentIndex (cmbSchema->findData (QVariant(QString::fromLatin1("Printing"))));

  // whatsthis
  cmbSchema->setWhatsThis(i18n(
        "Select the color scheme to use for the print." ) );
  cbDrawBackground->setWhatsThis(i18n(
        "<p>If enabled, the background color of the editor will be used.</p>"
        "<p>This may be useful if your color scheme is designed for a dark background.</p>") );
  cbEnableBox->setWhatsThis(i18n(
        "<p>If enabled, a box as defined in the properties below will be drawn "
        "around the contents of each page. The Header and Footer will be separated "
        "from the contents with a line as well.</p>") );
  sbBoxWidth->setWhatsThis(i18n(
        "The width of the box outline" ) );
  sbBoxMargin->setWhatsThis(i18n(
        "The margin inside boxes, in pixels") );
  kcbtnBoxColor->setWhatsThis(i18n(
        "The line color to use for boxes") );

  readSettings();
}

KatePrintLayout::~KatePrintLayout()
{
  writeSettings();
}

QString KatePrintLayout::colorScheme()
{
  return cmbSchema->itemData(cmbSchema->currentIndex()).toString();
}

bool KatePrintLayout::useBackground()
{
  return cbDrawBackground->isChecked();
}

bool KatePrintLayout::useBox()
{
  return cbEnableBox->isChecked();
}

int KatePrintLayout::boxWidth()
{
  return sbBoxWidth->value();
}

int KatePrintLayout::boxMargin()
{
  return sbBoxMargin->value();
}

QColor KatePrintLayout::boxColor()
{
  return kcbtnBoxColor->color();
}

void KatePrintLayout::readSettings()
{
  KSharedConfigPtr config = KSharedConfig::openConfig();
  KConfigGroup printGroup(config, "Kate Print Settings");

  KConfigGroup layoutGroup(&printGroup, "Layout");

  // get color schema back
  QString colorScheme = layoutGroup.readEntry( "ColorScheme", "Printing" );
  int index = cmbSchema->findData (QVariant (colorScheme));
  if (index != -1)
    cmbSchema->setCurrentIndex ( index );

  bool isBackgroundChecked = layoutGroup.readEntry( "BackgroundColorEnabled", false );
  cbDrawBackground->setChecked( isBackgroundChecked );

  bool isBoxChecked = layoutGroup.readEntry( "BoxEnabled", false );
  cbEnableBox->setChecked( isBoxChecked );

  int boxWidth = layoutGroup.readEntry( "BoxWidth", 1 );
  sbBoxWidth->setValue( boxWidth );

  int boxMargin = layoutGroup.readEntry( "BoxMargin", 6 );
  sbBoxMargin->setValue( boxMargin );

  QColor boxColor = layoutGroup.readEntry( "BoxColor", QColor() );
  kcbtnBoxColor->setColor( boxColor );
}

void KatePrintLayout::writeSettings()
{
  KSharedConfigPtr config = KSharedConfig::openConfig();
  KConfigGroup printGroup(config, "Kate Print Settings");

  KConfigGroup layoutGroup(&printGroup, "Layout");
  layoutGroup.writeEntry( "ColorScheme", colorScheme() );
  layoutGroup.writeEntry( "BackgroundColorEnabled", useBackground() );
  layoutGroup.writeEntry( "BoxEnabled", useBox() );
  layoutGroup.writeEntry( "BoxWidth", boxWidth() );
  layoutGroup.writeEntry( "BoxMargin", boxMargin() );
  layoutGroup.writeEntry( "BoxColor", boxColor() );

  config->sync();
}

//END KatePrintLayout

#include "printconfigwidgets.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
