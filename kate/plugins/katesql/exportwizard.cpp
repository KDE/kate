/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

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

#include "exportwizard.h"
#include "dataoutputwidget.h"

#include <klocale.h>
#include <kurlrequester.h>
#include <klineedit.h>

#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qformlayout.h>
#include <qgridlayout.h>

//BEGIN ExportWizard
ExportWizard::ExportWizard(QWidget *parent)
: QWizard(parent)
{
  setWindowTitle(i18n("Export Wizard"));

  addPage(new ExportOutputPage(this));
  addPage(new ExportFormatPage(this));
}


ExportWizard::~ExportWizard()
{
}
//END ExportWizard


//BEGIN ExportOutputPage
ExportOutputPage::ExportOutputPage(QWidget *parent)
: QWizardPage(parent)
{
  setTitle(i18n("Output Target"));
  setSubTitle(i18n("Select the output target."));

  QVBoxLayout *layout = new QVBoxLayout();

  clipboardRadioButton = new QRadioButton(i18n("Clipboard"), this);
  fileRadioButton = new QRadioButton(i18n("File"), this);

  QHBoxLayout *fileLayout = new QHBoxLayout();
  fileLayout->setContentsMargins(20, 0, 0, 0);

  fileUrl = new KUrlRequester(this);
  fileUrl->setMode(KFile::File);
  fileUrl->setFilter("*.csv|Comma Separated Values\n*|All files");

  fileLayout->addWidget(fileUrl);

  layout->addWidget(clipboardRadioButton);
  layout->addWidget(fileRadioButton);
  layout->addLayout(fileLayout);

  setLayout(layout);

  registerField("outClipboard", clipboardRadioButton);
  registerField("outFile", fileRadioButton);
  registerField("outFileUrl", fileUrl, "text");

  connect(fileRadioButton, SIGNAL(toggled(bool)), fileUrl, SLOT(setEnabled(bool)));
}


void ExportOutputPage::initializePage()
{
  clipboardRadioButton->setChecked(true);
  fileUrl->setEnabled(false);
}


bool ExportOutputPage::validatePage()
{
  if (fileRadioButton->isChecked() && fileUrl->text().isEmpty())
  {
    fileUrl->setFocus();
    return false;
  }

  /// TODO: check url validity

  return true;
}
//END ExportOutputPage


//BEGIN ExportFormatPage
ExportFormatPage::ExportFormatPage(QWidget *parent)
: QWizardPage(parent)
{
  setTitle(i18n("Fields Format"));
  setSubTitle(i18n("Select fields format.\nClick on \"Finish\" button to export data."));

  QVBoxLayout *layout = new QVBoxLayout();

  QGroupBox *headersGroupBox = new QGroupBox(i18n("Headers"), this);
  QVBoxLayout *headersLayout = new QVBoxLayout();

  exportColumnNamesCheckBox = new QCheckBox(i18n("Export column names"), headersGroupBox);
  exportLineNumbersCheckBox = new QCheckBox(i18n("Export line numbers"), headersGroupBox);

  headersLayout->addWidget(exportColumnNamesCheckBox);
  headersLayout->addWidget(exportLineNumbersCheckBox);

  headersGroupBox->setLayout(headersLayout);

  QGroupBox *quoteGroupBox = new QGroupBox(i18n("Quotes"), this);
  QGridLayout *quoteLayout = new QGridLayout();

  quoteStringsCheckBox = new QCheckBox(i18n("Quote strings"), quoteGroupBox);
  quoteNumbersCheckBox = new QCheckBox(i18n("Quote numbers"), quoteGroupBox);
  quoteStringsLine = new KLineEdit(quoteGroupBox);
  quoteNumbersLine = new KLineEdit(quoteGroupBox);

  quoteStringsLine->setMaxLength(1);
  quoteNumbersLine->setMaxLength(1);

  quoteLayout->addWidget(quoteStringsCheckBox, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
  quoteLayout->addWidget(new QLabel(i18n("Character:")), 0, 1, Qt::AlignRight | Qt::AlignVCenter);
  quoteLayout->addWidget(quoteStringsLine, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
  quoteLayout->addWidget(quoteNumbersCheckBox, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
  quoteLayout->addWidget(new QLabel(i18n("Character:")), 1, 1, Qt::AlignRight | Qt::AlignVCenter);
  quoteLayout->addWidget(quoteNumbersLine, 1, 2, Qt::AlignRight | Qt::AlignVCenter);
  quoteLayout->setColumnStretch(1, 1);

  quoteGroupBox->setLayout(quoteLayout);

  QGroupBox *delimitersGroupBox = new QGroupBox(i18n("Delimiters"), this);
  QFormLayout *delimitersLayout = new QFormLayout();

  fieldDelimiterLine = new KLineEdit(delimitersGroupBox);
  fieldDelimiterLine->setMaxLength(3);

  delimitersLayout->addRow(i18n("Field delimiter:"), fieldDelimiterLine);

  delimitersGroupBox->setLayout(delimitersLayout);

  layout->addWidget(headersGroupBox);
  layout->addWidget(quoteGroupBox);
  layout->addWidget(delimitersGroupBox);

  setLayout(layout);

  registerField("exportColumnNames", exportColumnNamesCheckBox);
  registerField("exportLineNumbers", exportLineNumbersCheckBox);
  registerField("checkQuoteStrings", quoteStringsCheckBox);
  registerField("checkQuoteNumbers", quoteNumbersCheckBox);
  registerField("quoteStringsChar", quoteStringsLine);
  registerField("quoteNumbersChar", quoteNumbersLine);
  registerField("fieldDelimiter*", fieldDelimiterLine);

  connect(quoteStringsCheckBox, SIGNAL(toggled(bool)), quoteStringsLine, SLOT(setEnabled(bool)));
  connect(quoteNumbersCheckBox, SIGNAL(toggled(bool)), quoteNumbersLine, SLOT(setEnabled(bool)));
}


void ExportFormatPage::initializePage()
{
  exportColumnNamesCheckBox->setChecked(true);
  exportLineNumbersCheckBox->setChecked(false);
  quoteStringsCheckBox->setChecked(false);
  quoteNumbersCheckBox->setChecked(false);
  quoteStringsLine->setEnabled(false);
  quoteNumbersLine->setEnabled(false);

  quoteStringsLine->setText("\"");
  quoteNumbersLine->setText("\"");
  fieldDelimiterLine->setText("\\t");
}


bool ExportFormatPage::validatePage()
{
  if ((quoteStringsCheckBox->isChecked() && quoteStringsLine->text().isEmpty())
  ||  (quoteNumbersCheckBox->isChecked() && quoteNumbersLine->text().isEmpty()))
    return false;

  if (fieldDelimiterLine->text().isEmpty())
    return false;

  return true;
}
//END ExportFormatPage
