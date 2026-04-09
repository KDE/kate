/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "exportwizard.h"
#include "dataoutputwidget.h"
#include "katesqlconstants.h"

#include <KLineEdit>
#include <KLocalizedString>
#include <KUrlRequester>

#include <QCheckBox>
#include <QDir>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>

// BEGIN ExportWizard
ExportWizard::ExportWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(i18nc("@title:window", "Export Wizard"));

    addPage(new ExportOutputPage(this));
    addPage(new ExportFormatPage(this));
}

ExportWizard::~ExportWizard() = default;
// END ExportWizard

// BEGIN ExportOutputPage
ExportOutputPage::ExportOutputPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18nc("@title Wizard page title", "Output Target"));
    setSubTitle(i18nc("@title Wizard page subtitle", "Select the output target."));

    auto *layout = new QVBoxLayout();

    documentRadioButton = new QRadioButton(i18nc("@option:radio Output target", "Current document"), this);
    clipboardRadioButton = new QRadioButton(i18nc("@option:radio Output target", "Clipboard"), this);
    fileRadioButton = new QRadioButton(i18nc("@option:radio Output target", "File"), this);

    auto *fileLayout = new QHBoxLayout();
    fileLayout->setContentsMargins(20, 0, 0, 0);

    fileUrl = new KUrlRequester(this);
    fileUrl->setMode(KFile::File);
    fileUrl->setNameFilters({i18n("Comma Separated Values") + QLatin1String(" (*.csv)"), i18n("All files") + QLatin1String(" (*)")});

    fileLayout->addWidget(fileUrl);

    layout->addWidget(documentRadioButton);
    layout->addWidget(clipboardRadioButton);
    layout->addWidget(fileRadioButton);
    layout->addLayout(fileLayout);

    setLayout(layout);

    registerField(KateSQLConstants::Export::Fields::OutputDocument, documentRadioButton);
    registerField(KateSQLConstants::Export::Fields::OutputClipboard, clipboardRadioButton);
    registerField(KateSQLConstants::Export::Fields::OutputFile, fileRadioButton);
    registerField(KateSQLConstants::Export::Fields::OutputFileUrl, fileUrl, "text");

    connect(fileRadioButton, &QRadioButton::toggled, fileUrl, &KUrlRequester::setEnabled);
}

void ExportOutputPage::initializePage()
{
    documentRadioButton->setChecked(true);
    fileUrl->setEnabled(false);
}

bool ExportOutputPage::validatePage()
{
    if (fileRadioButton->isChecked() && fileUrl->text().isEmpty()) {
        fileUrl->setFocus();
        return false;
    }

    /// TODO: check url validity

    return true;
}
// END ExportOutputPage

// BEGIN ExportFormatPage
ExportFormatPage::ExportFormatPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18nc("@title Wizard page title", "Fields Format"));
    setSubTitle(i18nc("@title Wizard page subtitle", "Select fields format.\nClick on \"Finish\" button to export data."));

    auto *layout = new QVBoxLayout();

    auto *headersGroupBox = new QGroupBox(i18nc("@title:group", "Headers"), this);
    auto *headersLayout = new QVBoxLayout();

    exportColumnNamesCheckBox = new QCheckBox(i18nc("@option:check", "Export column names"), headersGroupBox);
    exportLineNumbersCheckBox = new QCheckBox(i18nc("@option:check", "Export line numbers"), headersGroupBox);

    headersLayout->addWidget(exportColumnNamesCheckBox);
    headersLayout->addWidget(exportLineNumbersCheckBox);

    headersGroupBox->setLayout(headersLayout);

    auto *quoteGroupBox = new QGroupBox(i18nc("@title:group", "Quotes"), this);
    auto *quoteLayout = new QGridLayout();

    quoteStringsCheckBox = new QCheckBox(i18nc("@option:check", "Quote strings"), quoteGroupBox);
    quoteNumbersCheckBox = new QCheckBox(i18nc("@option:check", "Quote numbers"), quoteGroupBox);
    quoteStringsLine = new KLineEdit(quoteGroupBox);
    quoteNumbersLine = new KLineEdit(quoteGroupBox);

    quoteStringsLine->setMaxLength(1);
    quoteNumbersLine->setMaxLength(1);

    quoteLayout->addWidget(quoteStringsCheckBox, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    quoteLayout->addWidget(new QLabel(i18nc("@label:textbox", "Character:")), 0, 1, Qt::AlignRight | Qt::AlignVCenter);
    quoteLayout->addWidget(quoteStringsLine, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
    quoteLayout->addWidget(quoteNumbersCheckBox, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
    quoteLayout->addWidget(new QLabel(i18nc("@label:textbox", "Character:")), 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    quoteLayout->addWidget(quoteNumbersLine, 1, 2, Qt::AlignRight | Qt::AlignVCenter);
    quoteLayout->setColumnStretch(1, 1);

    quoteGroupBox->setLayout(quoteLayout);

    auto *delimitersGroupBox = new QGroupBox(i18nc("@title:group", "Delimiters"), this);
    auto *delimitersLayout = new QFormLayout();

    fieldDelimiterLine = new KLineEdit(delimitersGroupBox);
    fieldDelimiterLine->setMaxLength(3);

    delimitersLayout->addRow(i18nc("@label:textbox", "Field delimiter:"), fieldDelimiterLine);

    delimitersGroupBox->setLayout(delimitersLayout);

    layout->addWidget(headersGroupBox);
    layout->addWidget(quoteGroupBox);
    layout->addWidget(delimitersGroupBox);

    setLayout(layout);

    registerField(KateSQLConstants::Export::Fields::ExportColumnNames, exportColumnNamesCheckBox);
    registerField(KateSQLConstants::Export::Fields::ExportLineNumbers, exportLineNumbersCheckBox);
    registerField(KateSQLConstants::Export::Fields::CheckQuoteStrings, quoteStringsCheckBox);
    registerField(KateSQLConstants::Export::Fields::CheckQuoteNumbers, quoteNumbersCheckBox);
    registerField(KateSQLConstants::Export::Fields::QuoteStringsChar, quoteStringsLine);
    registerField(KateSQLConstants::Export::Fields::QuoteNumbersChar, quoteNumbersLine);
    registerField(KateSQLConstants::Export::Fields::FieldDelimiter, fieldDelimiterLine);

    connect(quoteStringsCheckBox, &QCheckBox::toggled, quoteStringsLine, &KLineEdit::setEnabled);
    connect(quoteNumbersCheckBox, &QCheckBox::toggled, quoteNumbersLine, &KLineEdit::setEnabled);
}

void ExportFormatPage::initializePage()
{
    exportColumnNamesCheckBox->setChecked(KateSQLConstants::Export::DefaultValues::IsExportingColumnNames);
    exportLineNumbersCheckBox->setChecked(KateSQLConstants::Export::DefaultValues::IsExportingLineNumbers);
    quoteStringsCheckBox->setChecked(KateSQLConstants::Export::DefaultValues::IsQuotingStrings);
    quoteNumbersCheckBox->setChecked(KateSQLConstants::Export::DefaultValues::IsQuotingNumbers);
    quoteStringsLine->setEnabled(KateSQLConstants::Export::DefaultValues::IsQuotingStrings);
    quoteNumbersLine->setEnabled(KateSQLConstants::Export::DefaultValues::IsQuotingNumbers);

    quoteStringsLine->setText(KateSQLConstants::Export::DefaultValues::QuoteStringCharForWizard);
    quoteNumbersLine->setText(KateSQLConstants::Export::DefaultValues::QuoteNumbersCharForWizard);
    fieldDelimiterLine->setText(KateSQLConstants::Export::DefaultValues::FieldDelimiterForWizard);
}

bool ExportFormatPage::validatePage()
{
    if ((quoteStringsCheckBox->isChecked() && quoteStringsLine->text().isEmpty())
        || (quoteNumbersCheckBox->isChecked() && quoteNumbersLine->text().isEmpty())) {
        return false;
    }

    if (fieldDelimiterLine->text().isEmpty()) {
        return false;
    }

    return true;
}
// END ExportFormatPage

#include "moc_exportwizard.cpp"
